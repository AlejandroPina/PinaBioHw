// PinaBio v1.0 — firmware v3
// Seeed XIAO ESP32-S3
// Placa: GSR + PPG + temp + ECG (AD8232) + 2 bandas resistivas
// HRV lo calcula el teléfono. USB solo para programar; sesión = BLE, sin PC.
// Seeed XIAO ESP32-S3  |  Arduino-ESP32
//
// La placa NO calcula HRV. Solo detecta latidos y manda los tiempos (como un Polar).
// El móvil hace RMSSD, SDNN, LF/HF y la respiración.
//
// Pines (zócalo de la PCB Mini):
//   I2C SDA=D4 (GPIO5)  SCL=D5 (GPIO6)
//   Batería A0 (GPIO1) divisor 1/2 en el XIAO
//   ~SLEEP D3 (GPIO4) INPUT_PULLUP; LOW = SW1 pide apagar analógico
//
// I2C: ADS1115 0x49  MAX30102 0x57  MAX30205 0x48
// BLE: JSON propio + Heart Rate Service 0x180D (HR + RR, estilo Polar H10)
//
// Librerías: Adafruit ADS1X15, SparkFun MAX3010x
// No es un dispositivo médico. Electrodos fuera si USB está enchufado.

#include <math.h>
#include <Wire.h>
#include <Adafruit_ADS1X15.h>
#include <MAX30105.h>
#include "heartRate.h"

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#include "driver/rtc_io.h"

// 1 = la XIAO anuncia BLE y manda JSON v3 SIN ADS/MAX (para probar Android ahora).
// 0 = placa PinaBio real.
#ifndef DEMO_BLE
#define DEMO_BLE 0
#endif

static const int PIN_VBAT = A0;
static const int PIN_SLEEP = D3;
static const int PIN_LOP = D8;
static const int PIN_LON = D9;

static const uint8_t ADDR_ADS = 0x49;
static const uint8_t ADDR_PPG = 0x57;
static const uint8_t ADDR_TEMP = 0x48;

static const float V_REF = 0.50f;
static const float R_SERIES = 100000.0f;
static const float R_BAND = 47000.0f;
static const uint32_t IR_FINGER_MIN = 20000;
static const uint32_t JSON_PERIOD_MS = 200;
static const uint32_t BEAT_STALE_MS = 2500;
static const int IBI_MIN_MS = 300;
static const int IBI_MAX_MS = 1500;
static const int RR_Q = 8;

static const char *BLE_NAME = "PinaBiosensor";
static const char *UUID_SVC = "6b1d0001-5e8a-4c2f-9b3a-2c7f0e1a4d90";
static const char *UUID_JSON = "6b1d0002-5e8a-4c2f-9b3a-2c7f0e1a4d90";

#define OK_ADS 1u
#define OK_PPG 2u
#define OK_TEMP 4u
#define OK_FINGER 8u
#define OK_SHUTDOWN 16u

Adafruit_ADS1115 ads;
MAX30105 ppg;

BLECharacteristic *jsonChar = nullptr;
BLECharacteristic *hrChar = nullptr;
volatile bool sleepRequested = false;
bool bleConnected = false;

bool adsOk = false;
bool ppgOk = false;
bool tempOk = false;

uint32_t lastBeatMs = 0;
int hrBpm = 0;
float beatAvg = 0;
int pendingRr[RR_Q];
int pendingRrN = 0;

uint32_t lastIr = 0;
float lastGsrUs = 0;
float lastTempC = 0;
float lastBattV = 0;
float lastThoraxV = 0;
float lastAbdomenV = 0;
float lastEcgMv = 0;
uint8_t lastOk = 0;
uint8_t lastLo = 0;

static const int ECG_N = 8;
float ecgBuf[ECG_N];
int ecgN = 0;

class ServerCb : public BLEServerCallbacks {
  void onConnect(BLEServer *s) override {
    (void)s;
    bleConnected = true;
  }
  void onDisconnect(BLEServer *s) override {
    bleConnected = false;
    delay(80);
    s->startAdvertising();
  }
};

void IRAM_ATTR onSleepIsr() { sleepRequested = true; }

bool i2cPresent(uint8_t addr) {
  Wire.beginTransmission(addr);
  return Wire.endTransmission() == 0;
}

float readMax30205C() {
  Wire.beginTransmission(ADDR_TEMP);
  Wire.write(0x00);
  if (Wire.endTransmission() != 0) {
    tempOk = false;
    return 0;
  }
  if (Wire.requestFrom((int)ADDR_TEMP, 2) != 2) {
    tempOk = false;
    return 0;
  }
  int16_t raw = (int16_t)((Wire.read() << 8) | Wire.read());
  tempOk = true;
  return raw * 0.00390625f;
}

float gsrMicrosiemens(float vMid) {
  if (!adsOk) return 0;
  if (vMid <= 0.002f) return 0;
  if (vMid >= V_REF * 0.98f) return 0;
  float rSkin = R_SERIES * vMid / (V_REF - vMid);
  if (rSkin < 100.0f) return 0;
  return 1000000.0f / rSkin;
}

void queueRr(int ibi) {
  if (pendingRrN < RR_Q) pendingRr[pendingRrN++] = ibi;
}

void notifyPolarHr(int bpm, int rrMs) {
  if (!hrChar || !bleConnected) return;
  uint8_t pkt[4];
  pkt[0] = 0x10;
  int hr = bpm;
  if (hr < 0) hr = 0;
  if (hr > 255) hr = 255;
  pkt[1] = (uint8_t)hr;
  uint16_t rr = (uint16_t)((rrMs * 1024L) / 1000L);
  pkt[2] = (uint8_t)(rr & 0xFF);
  pkt[3] = (uint8_t)(rr >> 8);
  hrChar->setValue(pkt, 4);
  hrChar->notify();
}

void clearHr() {
  hrBpm = 0;
  beatAvg = 0;
}

void processIr(uint32_t ir) {
  lastIr = ir;
  if (ir < IR_FINGER_MIN) {
    if (millis() - lastBeatMs > BEAT_STALE_MS) clearHr();
    return;
  }
  if (checkForBeat((long)ir)) {
    uint32_t now = millis();
    int ibi = (int)(now - lastBeatMs);
    lastBeatMs = now;
    if (ibi >= IBI_MIN_MS && ibi <= IBI_MAX_MS) {
      queueRr(ibi);
      float inst = 60000.0f / (float)ibi;
      if (beatAvg < 1) beatAvg = inst;
      else beatAvg = 0.8f * beatAvg + 0.2f * inst;
      int rounded = (int)(beatAvg + 0.5f);
      if (rounded < 40 || rounded > 180) rounded = (int)(inst + 0.5f);
      hrBpm = rounded;
      notifyPolarHr(hrBpm, ibi);
    }
  }
  if (millis() - lastBeatMs > BEAT_STALE_MS) clearHr();
}

uint8_t buildOk(bool shutting) {
  uint8_t ok = 0;
  if (adsOk) ok |= OK_ADS;
  if (ppgOk) ok |= OK_PPG;
  if (tempOk) ok |= OK_TEMP;
  if (lastIr >= IR_FINGER_MIN) ok |= OK_FINGER;
  if (shutting) ok |= OK_SHUTDOWN;
  return ok;
}

void fillRrArray(char *rr, size_t n) {
  size_t pos = 0;
  int wrote = snprintf(rr + pos, n - pos, "[");
  if (wrote < 0) return;
  pos += (size_t)wrote;
  for (int i = 0; i < pendingRrN && pos < n; i++) {
    wrote = snprintf(rr + pos, n - pos, "%s%d", i ? "," : "", pendingRr[i]);
    if (wrote < 0) break;
    pos += (size_t)wrote;
  }
  snprintf(rr + pos, n - pos, "]");
  pendingRrN = 0;
}

void fillEcgArray(char *dst, size_t n) {
  size_t pos = 0;
  int wrote = snprintf(dst + pos, n - pos, "[");
  if (wrote < 0) return;
  pos += (size_t)wrote;
  for (int i = 0; i < ecgN && pos < n; i++) {
    wrote = snprintf(dst + pos, n - pos, "%s%.0f", i ? "," : "", ecgBuf[i]);
    if (wrote < 0) break;
    pos += (size_t)wrote;
  }
  snprintf(dst + pos, n - pos, "]");
  ecgN = 0;
}

void fillJson(char *buf, size_t n, bool shutting) {
  lastOk = buildOk(shutting);
  lastLo = (uint8_t)((digitalRead(PIN_LOP) == HIGH || digitalRead(PIN_LON) == HIGH) ? 1 : 0);
  char rr[72];
  char ecg[96];
  fillRrArray(rr, sizeof(rr));
  fillEcgArray(ecg, sizeof(ecg));
  snprintf(buf, n,
           "{\"v\":3,\"ms\":%lu,\"gsr_uS\":%.2f,\"t_c\":%.2f,\"hr\":%d,"
           "\"rr_ms\":%s,\"ir\":%lu,\"batt_v\":%.2f,\"ok\":%u,"
           "\"lo\":%u,\"rt_v\":%.3f,\"ra_v\":%.3f,\"ecg_mv\":%s}",
           (unsigned long)millis(), lastGsrUs, lastTempC, hrBpm, rr,
           (unsigned long)lastIr, lastBattV, (unsigned)lastOk,
           (unsigned)lastLo, lastThoraxV, lastAbdomenV, ecg);
}

void publish(bool shutting) {
  char line[400];
  fillJson(line, sizeof(line), shutting);
  Serial.println(line);
  if (jsonChar && bleConnected) {
    jsonChar->setValue((uint8_t *)line, strlen(line));
    jsonChar->notify();
  }
}

void sampleSlowSensors() {
#if DEMO_BLE
  lastBattV = 3.90f;
  lastGsrUs = 6.5f + 1.2f * sinf(millis() / 1800.0f);
  lastThoraxV = 0.16f + 0.05f * sinf(millis() / 800.0f);
  lastAbdomenV = 0.14f + 0.04f * sinf(millis() / 800.0f);
  lastTempC = 33.1f;
  adsOk = true;
  tempOk = true;
  return;
#endif
  lastBattV = analogReadMilliVolts(PIN_VBAT) / 1000.0f * 2.0f;
  if (adsOk) {
    ads.setGain(GAIN_EIGHT);
    int16_t raw = ads.readADC_SingleEnded(1);
    lastGsrUs = gsrMicrosiemens(ads.computeVolts(raw));
    ads.setGain(GAIN_ONE);
    lastThoraxV = ads.computeVolts(ads.readADC_SingleEnded(2));
    lastAbdomenV = ads.computeVolts(ads.readADC_SingleEnded(3));
  } else {
    lastGsrUs = 0;
    lastThoraxV = 0;
    lastAbdomenV = 0;
    adsOk = i2cPresent(ADDR_ADS);
  }
  if (tempOk || i2cPresent(ADDR_TEMP)) lastTempC = readMax30205C();
  else lastTempC = 0;
}

void sampleEcg() {
#if DEMO_BLE
  lastEcgMv = 400.0f + 800.0f * ((millis() % 400) < 40);
  if (ecgN < ECG_N) ecgBuf[ecgN++] = lastEcgMv;
  return;
#endif
  if (!adsOk) return;
  ads.setGain(GAIN_ONE);
  float v = ads.computeVolts(ads.readADC_SingleEnded(0));
  lastEcgMv = v * 1000.0f;
  if (ecgN < ECG_N) ecgBuf[ecgN++] = lastEcgMv;
}

void enterDeepSleep() {
  publish(true);
  delay(30);
  BLEDevice::deinit(true);
  rtc_gpio_pullup_en(GPIO_NUM_4);
  rtc_gpio_pulldown_dis(GPIO_NUM_4);
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_4, 1);
  esp_deep_sleep_start();
}

void setupBle() {
  BLEDevice::init(BLE_NAME);
  BLEDevice::setMTU(247);
  BLEServer *server = BLEDevice::createServer();
  server->setCallbacks(new ServerCb());

  BLEService *svc = server->createService(UUID_SVC);
  jsonChar = svc->createCharacteristic(
      UUID_JSON, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
  jsonChar->addDescriptor(new BLE2902());
  char boot[400];
  fillJson(boot, sizeof(boot), false);
  jsonChar->setValue((uint8_t *)boot, strlen(boot));
  svc->start();

  BLEService *hrSvc = server->createService(BLEUUID((uint16_t)0x180D));
  hrChar = hrSvc->createCharacteristic(
      BLEUUID((uint16_t)0x2A37),
      BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
  hrChar->addDescriptor(new BLE2902());
  uint8_t emptyHr[2] = {0x00, 0x00};
  hrChar->setValue(emptyHr, 2);
  BLECharacteristic *loc = hrSvc->createCharacteristic(
      BLEUUID((uint16_t)0x2A38), BLECharacteristic::PROPERTY_READ);
  uint8_t finger = 3;
  loc->setValue(&finger, 1);
  hrSvc->start();

  BLEAdvertising *adv = BLEDevice::getAdvertising();
  adv->addServiceUUID(UUID_SVC);
  adv->addServiceUUID(BLEUUID((uint16_t)0x180D));
  adv->setScanResponse(true);
  adv->setMinPreferred(0x06);
  BLEDevice::startAdvertising();
}

void setup() {
  pinMode(PIN_SLEEP, INPUT_PULLUP);
  pinMode(PIN_LOP, INPUT);
  pinMode(PIN_LON, INPUT);
  analogReadResolution(12);
  Serial.begin(115200);
  delay(200);

  Wire.begin();
  Wire.setClock(400000);

#if DEMO_BLE
  adsOk = true;
  ppgOk = true;
  tempOk = true;
  lastIr = 90000;
#else
  adsOk = ads.begin(ADDR_ADS);
  if (adsOk) {
    ads.setGain(GAIN_EIGHT);
    ads.setDataRate(RATE_ADS1115_860SPS);
  }

  ppgOk = ppg.begin(Wire, I2C_SPEED_FAST, ADDR_PPG);
  if (ppgOk) {
    ppg.setup(0x1F, 4, 2, 100, 411, 4096);
    ppg.setPulseAmplitudeRed(0x0A);
    ppg.setPulseAmplitudeIR(0x1F);
    ppg.setPulseAmplitudeGreen(0);
  }

  tempOk = i2cPresent(ADDR_TEMP);
#endif

  attachInterrupt(digitalPinToInterrupt(PIN_SLEEP), onSleepIsr, FALLING);

  sampleSlowSensors();
  setupBle();
#if DEMO_BLE
  Serial.println("PinaBio DEMO_BLE=1 (JSON v3 sin sensores)");
#else
  Serial.println("PinaBio v1.0 (JSON v3, BLE PinaBiosensor, sin USB en sesion)");
#endif
}

void loop() {
  static uint32_t lastJson = 0;
  static uint32_t sleepLowSince = 0;

  if (ppgOk) {
#if DEMO_BLE
    int fakeIbi = 800 + (int)(40.0f * sinf(millis() / 900.0f));
    static uint32_t lastFakeBeat = 0;
    if (millis() - lastFakeBeat >= (uint32_t)fakeIbi) {
      lastFakeBeat = millis();
      lastIr = 90000;
      queueRr(fakeIbi);
      hrBpm = 60000 / fakeIbi;
      notifyPolarHr(hrBpm, fakeIbi);
    }
#else
    processIr(ppg.getIR());
#endif
  } else if ((millis() % 1000) < 20) {
    ppgOk = i2cPresent(ADDR_PPG);
  }

  sampleEcg();

#if !DEMO_BLE
  bool pinLow = digitalRead(PIN_SLEEP) == LOW;
  if (sleepRequested || pinLow) {
    if (sleepLowSince == 0) sleepLowSince = millis();
    if (millis() - sleepLowSince > 40) enterDeepSleep();
  } else {
    sleepLowSince = 0;
    sleepRequested = false;
  }
#endif

  if (millis() - lastJson >= JSON_PERIOD_MS) {
    lastJson = millis();
    sampleSlowSensors();
    publish(false);
  }
}
