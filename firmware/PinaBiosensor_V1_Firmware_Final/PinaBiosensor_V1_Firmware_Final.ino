/*
 * PinaBiosensor V1 - engineered firmware
 * Target: Seeed Studio XIAO ESP32-S3
 *
 * Hardware fixed by fabricated V1 PCB:
 *   ADS1115 0x49  AIN0=ECG, AIN1=GSR, AIN2=thorax, AIN3=abdomen
 *   MAX30102 0x57 external module, RAW RED + IR
 *   MAX30205 0x48 temperature
 *   Battery D0/A0/GPIO1, 47k/47k divider on PCB
 *   Sleep D3/GPIO4 active LOW
 *   AD8232 leads-off D8/GPIO7 and D9/GPIO8
 *   I2C SDA D4/GPIO5, SCL D5/GPIO6
 *
 * Communication:
 *   BLE name remains exactly: PinaBiosensor
 *   Custom service UUID is kept for compatibility.
 *   ECG RAW and PPG RAW use independent binary notify characteristics.
 *   Telemetry is separate. Commands are bidirectional over BLE and USB.
 *   Wi-Fi is intentionally not used.
 *
 * IMPORTANT:
 *   This firmware is a testable engineering baseline, not a medical device.
 *   Target sample rates are configurable goals. STATUS/DIAG reports measured
 *   rates so the real PCBA can be characterized without changing code.
 */

#include <Arduino.h>
#include <Wire.h>
#include <MAX30105.h>
#include "heartRate.h"
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <Preferences.h>
#include "driver/rtc_io.h"
#include "esp_sleep.h"
#include "esp_timer.h"
#include <math.h>
#include <ctype.h>
#include <string.h>
#include <strings.h>
#include <string>

// ----------------------------------------------------------------------
// Hardware
// ----------------------------------------------------------------------
static constexpr uint8_t ADS_ADDR  = 0x49;
static constexpr uint8_t PPG_ADDR  = 0x57;
static constexpr uint8_t TEMP_ADDR = 0x48;

static constexpr int PIN_VBAT  = A0;  // D0 / GPIO1
static constexpr int PIN_SLEEP = D3;  // D3 / GPIO4
static constexpr int PIN_SDA   = D4;  // D4 / GPIO5
static constexpr int PIN_SCL   = D5;  // D5 / GPIO6
static constexpr int PIN_LOP   = D8;  // D8 / GPIO7
static constexpr int PIN_LON   = D9;  // D9 / GPIO8

// ----------------------------------------------------------------------
// BLE UUIDs
// ----------------------------------------------------------------------
static constexpr char BLE_NAME[]  = "PinaBiosensor";
static constexpr char UUID_SVC[]  = "6b1d0001-5e8a-4c2f-9b3a-2c7f0e1a4d90";
static constexpr char UUID_JSON[] = "6b1d0002-5e8a-4c2f-9b3a-2c7f0e1a4d90";
static constexpr char UUID_CMD[]  = "6b1d0003-5e8a-4c2f-9b3a-2c7f0e1a4d90";
static constexpr char UUID_ECG[]  = "6b1d0004-5e8a-4c2f-9b3a-2c7f0e1a4d90";
static constexpr char UUID_PPG[]  = "6b1d0005-5e8a-4c2f-9b3a-2c7f0e1a4d90";
static constexpr char UUID_TEL[]  = "6b1d0006-5e8a-4c2f-9b3a-2c7f0e1a4d90";
static constexpr char UUID_EVT[]  = "6b1d0007-5e8a-4c2f-9b3a-2c7f0e1a4d90";

// ----------------------------------------------------------------------
// Acquisition defaults
// ----------------------------------------------------------------------
static constexpr uint16_t ECG_RATE_DEFAULT    = 250;
static constexpr uint16_t ECG_RATE_MAX        = 500;
static constexpr uint16_t PPG_RATE_DEFAULT    = 200;
static constexpr uint16_t GSR_RATE_DEFAULT    = 10;
static constexpr uint16_t THORAX_RATE_DEFAULT = 20;
static constexpr uint16_t ABDOMEN_RATE_DEFAULT= 20;
static constexpr uint16_t ADC_BUDGET_SPS      = 650;

static constexpr float GSR_VREF_DEFAULT = 0.500f;
static constexpr float GSR_FIXED_R      = 100000.0f;
static constexpr float GSR_SERIES_R     = 1000.0f; // PCB R11

static constexpr uint32_t TELEMETRY_PERIOD_MS = 100;  // 10 Hz
static constexpr uint32_t DIAG_PERIOD_MS      = 2000;
static constexpr uint32_t SLEEP_HOLD_MS       = 40;
static constexpr uint32_t I2C_TIMEOUT_MS       = 20;
static constexpr uint32_t ADS_CONV_TIMEOUT_US  = 4000;
static constexpr uint32_t PPG_POLL_MS           = 3;

// ----------------------------------------------------------------------
// Binary protocol
// ----------------------------------------------------------------------
// Header (12 bytes):
//   0 magic 0xA5
//   1 type (1=ECG, 2=PPG, 3=TELEMETRY, 4=EVENT)
//   2 version = 1
//   3 flags
//   4..5 sequence LE
//   6..9 first timestamp, ms from boot LE
//   10..11 nominal/estimated sample period, us LE
//   12 sample count
// Payload follows. CRC16-CCITT is last 2 bytes.
// ECG sample: int16 raw, LE.
// PPG sample: uint24 IR + uint24 RED, LE (18-bit source packed into 24 bits).
static constexpr uint8_t PROTOCOL_VERSION = 1;
static constexpr uint8_t PACKET_MAGIC     = 0xA5;
static constexpr uint16_t DEFAULT_ATT_MTU  = 23;

// ----------------------------------------------------------------------
// Types and state
// ----------------------------------------------------------------------
enum AdcChan : uint8_t { CH_ECG = 0, CH_GSR = 1, CH_THORAX = 2, CH_ABDOMEN = 3, CH_NONE = 255 };

enum RunState : uint8_t { STATE_STOPPED = 0, STATE_RUNNING = 1 };

struct ESample {
  int16_t raw;
  uint32_t tsUs; // microseconds since session start, wraps after ~71.6 min
};

struct PSample {
  uint32_t ir;
  uint32_t red;
  uint32_t tsUs; // microseconds since session start, wraps after ~71.6 min
};

struct SlowSample {
  uint32_t tsUs;
  float gsrUs;
  float thoraxV;
  float abdomenV;
  float tempC;
  float battV;
  uint16_t hr;
  uint16_t rrMs;
  uint8_t lo;
  uint8_t adsOk;
  uint8_t ppgOk;
  uint8_t tempOk;
};

struct CmdMsg {
  char text[128];
};

enum GainMode : uint8_t { GAINMODE_ONE = 0, GAINMODE_EIGHT = 1 };

struct AdcSlot {
  uint64_t nextDueUs;
  AdcChan lastChan;
  GainMode lastGain;
};

// ----------------------------------------------------------------------
// Globals
// ----------------------------------------------------------------------
MAX30105 ppg;
Preferences prefs;

BLEServer *bleServer = nullptr;
BLECharacteristic *chJson = nullptr;
BLECharacteristic *chCmd = nullptr;
BLECharacteristic *chEcg = nullptr;
BLECharacteristic *chPpg = nullptr;
BLECharacteristic *chTel = nullptr;
BLECharacteristic *chEvt = nullptr;
BLECharacteristic *chHr = nullptr;

SemaphoreHandle_t i2cMutex = nullptr;
SemaphoreHandle_t bleMutex = nullptr;
QueueHandle_t cmdQueue = nullptr;

TaskHandle_t adcTaskHandle = nullptr;
TaskHandle_t ppgTaskHandle = nullptr;
TaskHandle_t slowTaskHandle = nullptr;
TaskHandle_t commsTaskHandle = nullptr;
TaskHandle_t sleepTaskHandle = nullptr;

portMUX_TYPE ecgMux = portMUX_INITIALIZER_UNLOCKED;
portMUX_TYPE ppgMux = portMUX_INITIALIZER_UNLOCKED;
portMUX_TYPE telMux = portMUX_INITIALIZER_UNLOCKED;

volatile bool bleConnected = false;
volatile bool sleepRequested = false;
volatile bool deepSleeping = false;

RunState runState = STATE_RUNNING;

bool enEcg = true;
bool enGsr = true;
bool enThorax = true;
bool enAbdomen = true;
bool enPpg = true;
bool enTemp = true;
bool enBattery = true;

bool streamEcg = true;
bool streamPpg = true;
bool streamTelemetry = true;
bool streamJson = false;
bool txUsb = true;
bool txBle = true;
bool usbEcgStream = false;
bool usbPpgStream = false;
bool autoEcgBoost = true;

uint16_t ecgRateCfg = ECG_RATE_DEFAULT;
uint16_t ppgRateCfg = PPG_RATE_DEFAULT;
uint16_t gsrRateCfg = GSR_RATE_DEFAULT;
uint16_t thoraxRateCfg = THORAX_RATE_DEFAULT;
uint16_t abdomenRateCfg = ABDOMEN_RATE_DEFAULT;
float gsrVref = GSR_VREF_DEFAULT;

bool adsOk = false;
bool ppgOk = false;
bool tempOk = false;

float lastEcgV = 0.0f;
float lastGsrUs = 0.0f;
float lastThoraxV = 0.0f;
float lastAbdomenV = 0.0f;
float lastTempC = 0.0f;
float lastBattV = 0.0f;
uint8_t lastLo = 0;

uint16_t ppgHr = 0;
uint16_t ppgRrMs = 0;
uint32_t lastBeatTsMs = 0;
uint16_t hrHist[4] = {0,0,0,0};
uint8_t hrHistCount = 0;

uint32_t adcSamples[4] = {0,0,0,0};
uint32_t adcErrors[4] = {0,0,0,0};
uint32_t adcMuxChanges = 0;
uint32_t adcConversions = 0;
uint32_t ppgSamples = 0;
uint32_t ecgPackets = 0;
uint32_t ppgPackets = 0;
uint32_t telPackets = 0;
uint32_t eventPackets = 0;
uint32_t ecgDrops = 0;
uint32_t ppgDrops = 0;
uint32_t cmdDrops = 0;
uint32_t bleNotifyAttempts = 0;
uint32_t bleNotifyRejected = 0;

uint16_t ecgSeq = 0;
uint16_t ppgSeq = 0;
uint16_t telSeq = 0;
uint16_t evtSeq = 0;

uint32_t sessionStartMs = 0;
uint64_t sessionStartUs = 0;
uint32_t lastTelemetryMs = 0;
uint32_t lastDiagMs = 0;
uint32_t configDirtySinceMs = 0;
bool configDirty = false;

// ----------------------------------------------------------------------
// Ring buffers
// ----------------------------------------------------------------------
static constexpr size_t ECG_RING_CAP = 2048;
static constexpr size_t PPG_RING_CAP = 1024;
static constexpr size_t TEL_RING_CAP = 64;

ESample ecgRing[ECG_RING_CAP];
size_t ecgHead = 0, ecgTail = 0;
PSample ppgRing[PPG_RING_CAP];
size_t ppgHead = 0, ppgTail = 0;
SlowSample telRing[TEL_RING_CAP];
size_t telHead = 0, telTail = 0;

// ----------------------------------------------------------------------
// Small helpers
// ----------------------------------------------------------------------
static uint32_t nowMs() {
  return (uint32_t)(esp_timer_get_time() / 1000ULL);
}

static uint32_t sessionUs32() {
  return (uint32_t)((uint64_t)esp_timer_get_time() - sessionStartUs);
}

static uint16_t clampU16(long v) {
  if (v < 0) return 0;
  if (v > 65535L) return 65535;
  return (uint16_t)v;
}

static uint32_t clampU32(long long v) {
  if (v < 0) return 0;
  if ((unsigned long long)v > 0xFFFFFFFFULL) return 0xFFFFFFFFUL;
  return (uint32_t)v;
}

static uint16_t crc16ccitt(const uint8_t *data, size_t len) {
  uint16_t crc = 0xFFFF;
  for (size_t i = 0; i < len; ++i) {
    crc ^= (uint16_t)data[i] << 8;
    for (uint8_t b = 0; b < 8; ++b) {
      crc = (crc & 0x8000U) ? (uint16_t)((crc << 1) ^ 0x1021U) : (uint16_t)(crc << 1);
    }
  }
  return crc;
}

static void putU16(uint8_t *p, uint16_t v) {
  p[0] = (uint8_t)(v & 0xFFU);
  p[1] = (uint8_t)(v >> 8);
}

static void putU32(uint8_t *p, uint32_t v) {
  p[0] = (uint8_t)(v & 0xFFU);
  p[1] = (uint8_t)((v >> 8) & 0xFFU);
  p[2] = (uint8_t)((v >> 16) & 0xFFU);
  p[3] = (uint8_t)((v >> 24) & 0xFFU);
}

static void putU24(uint8_t *p, uint32_t v) {
  p[0] = (uint8_t)(v & 0xFFU);
  p[1] = (uint8_t)((v >> 8) & 0xFFU);
  p[2] = (uint8_t)((v >> 16) & 0xFFU);
}

static float adsRawToVolts(int16_t raw, GainMode gain) {
  float fsr = 4.096f;
  switch (gain) {
    case GAINMODE_ONE:   fsr = 4.096f; break;
    case GAINMODE_EIGHT: fsr = 0.512f; break;
    default:             fsr = 4.096f; break;
  }
  return ((float)raw * fsr) / 32768.0f;
}

static float calcGsrUs(float vMid) {
  if (!isfinite(vMid)) return 0.0f;
  if (vMid <= 0.002f) return 0.0f;
  if (vMid >= (gsrVref - 0.002f)) return 0.0f;
  float total = GSR_FIXED_R * vMid / (gsrVref - vMid);
  float skin = total - GSR_SERIES_R;
  if (!isfinite(skin) || skin <= 100.0f) return 0.0f;
  return 1000000.0f / skin;
}

static uint16_t adcTotalBudget() {
  uint32_t total = 0;
  if (enEcg) {
    uint16_t ecgEff = ecgRateCfg;
    if (autoEcgBoost) {
      uint32_t slow = 0;
      if (enGsr) slow += gsrRateCfg;
      if (enThorax) slow += thoraxRateCfg;
      if (enAbdomen) slow += abdomenRateCfg;
      uint32_t headroom = (slow < ADC_BUDGET_SPS) ? (ADC_BUDGET_SPS - slow) : ECG_RATE_DEFAULT;
      if (headroom > ECG_RATE_MAX) headroom = ECG_RATE_MAX;
      if (headroom < ECG_RATE_DEFAULT) headroom = ECG_RATE_DEFAULT;
      ecgEff = (uint16_t)headroom;
    }
    total += ecgEff;
  }
  if (enGsr) total += gsrRateCfg;
  if (enThorax) total += thoraxRateCfg;
  if (enAbdomen) total += abdomenRateCfg;
  return (total > 65535U) ? 65535U : (uint16_t)total;
}

static uint16_t effectiveEcgRate() {
  if (!enEcg) return 0;
  if (!autoEcgBoost) return ecgRateCfg;
  uint32_t slow = 0;
  if (enGsr) slow += gsrRateCfg;
  if (enThorax) slow += thoraxRateCfg;
  if (enAbdomen) slow += abdomenRateCfg;
  uint32_t headroom = (slow < ADC_BUDGET_SPS) ? (ADC_BUDGET_SPS - slow) : ECG_RATE_DEFAULT;
  if (headroom < ECG_RATE_DEFAULT) headroom = ECG_RATE_DEFAULT;
  if (headroom > ECG_RATE_MAX) headroom = ECG_RATE_MAX;
  return (uint16_t)headroom;
}

static bool enabled(AdcChan c) {
  if (c == CH_ECG) return enEcg;
  if (c == CH_GSR) return enGsr;
  if (c == CH_THORAX) return enThorax;
  if (c == CH_ABDOMEN) return enAbdomen;
  return false;
}

static uint16_t targetRate(AdcChan c) {
  if (!enabled(c)) return 0;
  if (c == CH_ECG) return effectiveEcgRate();
  if (c == CH_GSR) return gsrRateCfg;
  if (c == CH_THORAX) return thoraxRateCfg;
  if (c == CH_ABDOMEN) return abdomenRateCfg;
  return 0;
}

static GainMode gainFor(AdcChan c) {
  return (c == CH_GSR) ? GAINMODE_EIGHT : GAINMODE_ONE;
}

// ----------------------------------------------------------------------
// Ring buffers
// ----------------------------------------------------------------------
static bool ecgPush(const ESample &s) {
  bool overwritten = false;
  portENTER_CRITICAL(&ecgMux);
  size_t next = (ecgHead + 1) % ECG_RING_CAP;
  if (next == ecgTail) {
    ecgTail = (ecgTail + 1) % ECG_RING_CAP;
    ecgDrops++;
    overwritten = true;
  }
  ecgRing[ecgHead] = s;
  ecgHead = next;
  portEXIT_CRITICAL(&ecgMux);
  return !overwritten;
}

static bool ecgPop(ESample &s) {
  bool ok = false;
  portENTER_CRITICAL(&ecgMux);
  if (ecgTail != ecgHead) {
    s = ecgRing[ecgTail];
    ecgTail = (ecgTail + 1) % ECG_RING_CAP;
    ok = true;
  }
  portEXIT_CRITICAL(&ecgMux);
  return ok;
}

static size_t ecgCount() {
  size_t n;
  portENTER_CRITICAL(&ecgMux);
  n = (ecgHead >= ecgTail) ? (ecgHead - ecgTail) : (ECG_RING_CAP - ecgTail + ecgHead);
  portEXIT_CRITICAL(&ecgMux);
  return n;
}

static bool ppgPush(const PSample &s) {
  bool overwritten = false;
  portENTER_CRITICAL(&ppgMux);
  size_t next = (ppgHead + 1) % PPG_RING_CAP;
  if (next == ppgTail) {
    ppgTail = (ppgTail + 1) % PPG_RING_CAP;
    ppgDrops++;
    overwritten = true;
  }
  ppgRing[ppgHead] = s;
  ppgHead = next;
  portEXIT_CRITICAL(&ppgMux);
  return !overwritten;
}

static bool ppgPop(PSample &s) {
  bool ok = false;
  portENTER_CRITICAL(&ppgMux);
  if (ppgTail != ppgHead) {
    s = ppgRing[ppgTail];
    ppgTail = (ppgTail + 1) % PPG_RING_CAP;
    ok = true;
  }
  portEXIT_CRITICAL(&ppgMux);
  return ok;
}

static size_t ppgCount() {
  size_t n;
  portENTER_CRITICAL(&ppgMux);
  n = (ppgHead >= ppgTail) ? (ppgHead - ppgTail) : (PPG_RING_CAP - ppgTail + ppgHead);
  portEXIT_CRITICAL(&ppgMux);
  return n;
}

static bool telPush(const SlowSample &s) {
  bool overwritten = false;
  portENTER_CRITICAL(&telMux);
  size_t next = (telHead + 1) % TEL_RING_CAP;
  if (next == telTail) {
    telTail = (telTail + 1) % TEL_RING_CAP;
    overwritten = true;
  }
  telRing[telHead] = s;
  telHead = next;
  portEXIT_CRITICAL(&telMux);
  return !overwritten;
}

static bool telPop(SlowSample &s) {
  bool ok = false;
  portENTER_CRITICAL(&telMux);
  if (telTail != telHead) {
    s = telRing[telTail];
    telTail = (telTail + 1) % TEL_RING_CAP;
    ok = true;
  }
  portEXIT_CRITICAL(&telMux);
  return ok;
}

static size_t telCount() {
  size_t n;
  portENTER_CRITICAL(&telMux);
  n = (telHead >= telTail) ? (telHead - telTail) : (TEL_RING_CAP - telTail + telHead);
  portEXIT_CRITICAL(&telMux);
  return n;
}

static void clearBuffers() {
  portENTER_CRITICAL(&ecgMux);
  ecgHead = ecgTail = 0;
  portEXIT_CRITICAL(&ecgMux);
  portENTER_CRITICAL(&ppgMux);
  ppgHead = ppgTail = 0;
  portEXIT_CRITICAL(&ppgMux);
  portENTER_CRITICAL(&telMux);
  telHead = telTail = 0;
  portEXIT_CRITICAL(&telMux);
}

// ----------------------------------------------------------------------
// Low-level ADS1115 single-shot engine
// This avoids the repeated comparator-threshold writes performed by the
// generic Adafruit startADCReading() helper on every conversion. The PCB's
// ALERT pin is not wired to the XIAO, so we poll OS in the config register.
// ----------------------------------------------------------------------
static constexpr uint16_t ADS_REG_CONVERSION = 0x00;
static constexpr uint16_t ADS_REG_CONFIG     = 0x01;
static constexpr uint16_t ADS_OS_START       = 0x8000;
static constexpr uint16_t ADS_MUX_BASE       = 0x4000;
static constexpr uint16_t ADS_PGA_ONE        = 0x0200;
static constexpr uint16_t ADS_PGA_EIGHT      = 0x0400;
static constexpr uint16_t ADS_MODE_SINGLE    = 0x0100;
static constexpr uint16_t ADS_DR_860         = 0x00E0;
static constexpr uint16_t ADS_CQUE_DISABLE   = 0x0003;

static bool adsWriteConfig(uint16_t config) {
  if (!i2cMutex || !xSemaphoreTake(i2cMutex, pdMS_TO_TICKS(I2C_TIMEOUT_MS))) return false;
  Wire.beginTransmission(ADS_ADDR);
  Wire.write((uint8_t)ADS_REG_CONFIG);
  Wire.write((uint8_t)(config >> 8));
  Wire.write((uint8_t)(config & 0xFF));
  uint8_t err = Wire.endTransmission(true);
  xSemaphoreGive(i2cMutex);
  return err == 0;
}

static bool adsReadReg16(uint8_t reg, uint16_t &value) {
  if (!i2cMutex || !xSemaphoreTake(i2cMutex, pdMS_TO_TICKS(I2C_TIMEOUT_MS))) return false;
  Wire.beginTransmission(ADS_ADDR);
  Wire.write(reg);
  uint8_t err = Wire.endTransmission(false);
  if (err != 0) {
    xSemaphoreGive(i2cMutex);
    return false;
  }
  uint8_t got = Wire.requestFrom((int)ADS_ADDR, 2, true);
  if (got != 2) {
    xSemaphoreGive(i2cMutex);
    return false;
  }
  value = ((uint16_t)Wire.read() << 8) | Wire.read();
  xSemaphoreGive(i2cMutex);
  return true;
}

static uint16_t adsMuxBits(AdcChan c) {
  return (uint16_t)(ADS_MUX_BASE + ((uint16_t)c << 12));
}

static uint16_t adsGainBits(GainMode g) {
  return (g == GAINMODE_EIGHT) ? ADS_PGA_EIGHT : ADS_PGA_ONE;
}

static bool adsStartAndRead(AdcChan c, int16_t &raw, uint32_t &tsUs) {
  GainMode gain = gainFor(c);
  uint16_t cfg = ADS_OS_START | adsMuxBits(c) | adsGainBits(gain) |
                 ADS_MODE_SINGLE | ADS_DR_860 | ADS_CQUE_DISABLE;

  if (!adsWriteConfig(cfg)) return false;

  uint32_t startUs = (uint32_t)esp_timer_get_time();
  while ((uint32_t)(esp_timer_get_time() - startUs) < ADS_CONV_TIMEOUT_US) {
    uint16_t cval = 0;
    if (!adsReadReg16(ADS_REG_CONFIG, cval)) return false;
    if (cval & ADS_OS_START) break;
    vTaskDelay(1);
  }

  uint16_t raw16 = 0;
  if (!adsReadReg16(ADS_REG_CONVERSION, raw16)) return false;
  raw = (int16_t)raw16;
  tsUs = sessionUs32();
  return true;
}

static bool probeI2C(uint8_t addr) {
  if (!i2cMutex || !xSemaphoreTake(i2cMutex, pdMS_TO_TICKS(I2C_TIMEOUT_MS))) return false;
  Wire.beginTransmission(addr);
  uint8_t err = Wire.endTransmission(true);
  xSemaphoreGive(i2cMutex);
  return err == 0;
}

// ----------------------------------------------------------------------
// ADS scheduler
// ----------------------------------------------------------------------
static AdcSlot adcSlots[4];
static volatile bool adcScheduleDirty = true;

static void rebuildAdcSchedule() {
  uint64_t t = (uint64_t)esp_timer_get_time();
  for (uint8_t i = 0; i < 4; ++i) {
    AdcChan c = (AdcChan)i;
    adcSlots[i].nextDueUs = enabled(c) ? t : UINT64_MAX;
    adcSlots[i].lastChan = CH_NONE;
    adcSlots[i].lastGain = GAINMODE_ONE;
  }
  adcScheduleDirty = false;
}

static AdcChan pickNextAdcChannel(uint64_t nowUs) {
  if (adcScheduleDirty) rebuildAdcSchedule();
  AdcChan best = CH_NONE;
  uint64_t bestDue = UINT64_MAX;
  for (uint8_t i = 0; i < 4; ++i) {
    AdcChan c = (AdcChan)i;
    if (!enabled(c)) continue;
    uint16_t rate = targetRate(c);
    if (rate == 0) continue;
    if (adcSlots[i].nextDueUs < bestDue) {
      bestDue = adcSlots[i].nextDueUs;
      best = c;
    }
  }
  if (best == CH_NONE) return CH_NONE;
  if (bestDue > nowUs) {
    uint64_t waitUs = bestDue - nowUs;
    if (waitUs > 1500) {
      vTaskDelay(pdMS_TO_TICKS((uint32_t)(waitUs / 1000ULL)));
    } else {
      delayMicroseconds((uint32_t)waitUs);
    }
  }
  return best;
}

static void processAdcSample(AdcChan c, int16_t raw, uint32_t tsUs) {
  const uint8_t idx = (uint8_t)c;
  const GainMode gain = gainFor(c);
  const float v = adsRawToVolts(raw, gain);
  adcSamples[idx]++;

  if (c == CH_ECG) {
    lastEcgV = v;
    ecgPush({raw, tsUs});
  } else if (c == CH_GSR) {
    lastGsrUs = calcGsrUs(v);
  } else if (c == CH_THORAX) {
    lastThoraxV = v;
  } else if (c == CH_ABDOMEN) {
    lastAbdomenV = v;
  }
}

static void adcTask(void *arg) {
  (void)arg;
  rebuildAdcSchedule();

  for (;;) {
    if (runState != STATE_RUNNING || !adsOk || deepSleeping) {
      vTaskDelay(pdMS_TO_TICKS(10));
      continue;
    }

    uint64_t now = (uint64_t)esp_timer_get_time();
    AdcChan c = pickNextAdcChannel(now);
    if (c == CH_NONE) {
      vTaskDelay(pdMS_TO_TICKS(5));
      continue;
    }

    const uint8_t idx = (uint8_t)c;
    const GainMode g = gainFor(c);
    if (adcSlots[idx].lastChan != c || adcSlots[idx].lastGain != g) {
      if (adcSlots[idx].lastChan != CH_NONE) adcMuxChanges++;
      adcSlots[idx].lastChan = c;
      adcSlots[idx].lastGain = g;
    }

    int16_t raw = 0;
    uint32_t ts = 0;
    if (adsStartAndRead(c, raw, ts)) {
      adcConversions++;
      processAdcSample(c, raw, ts);
    } else {
      adcErrors[idx]++;
    }

    const uint16_t rate = targetRate(c);
    if (rate == 0) {
      adcSlots[idx].nextDueUs = UINT64_MAX;
      continue;
    }
    const uint64_t periodUs = 1000000ULL / (uint64_t)rate;
    if (adcSlots[idx].nextDueUs == UINT64_MAX) adcSlots[idx].nextDueUs = now;
    adcSlots[idx].nextDueUs += periodUs;

    const uint64_t n2 = (uint64_t)esp_timer_get_time();
    if (adcSlots[idx].nextDueUs + 4ULL * periodUs < n2) {
      adcSlots[idx].nextDueUs = n2 + periodUs;
    }
  }
}

// ----------------------------------------------------------------------
// MAX30205
// ----------------------------------------------------------------------
static bool max30205Read(float &tempC) {
  if (!i2cMutex || !xSemaphoreTake(i2cMutex, pdMS_TO_TICKS(I2C_TIMEOUT_MS))) return false;
  Wire.beginTransmission(TEMP_ADDR);
  Wire.write(0x00);
  if (Wire.endTransmission(false) != 0) {
    xSemaphoreGive(i2cMutex);
    return false;
  }
  if (Wire.requestFrom((int)TEMP_ADDR, 2, true) != 2) {
    xSemaphoreGive(i2cMutex);
    return false;
  }
  uint16_t rawU = ((uint16_t)Wire.read() << 8) | Wire.read();
  xSemaphoreGive(i2cMutex);
  int16_t raw = (int16_t)rawU;
  tempC = ((float)raw) * 0.00390625f;
  return isfinite(tempC);
}

// ----------------------------------------------------------------------
// MAX30102 / PPG
// ----------------------------------------------------------------------
static bool configurePpg(uint16_t rate) {
  if (!ppgOk) return false;
  if (rate != 100 && rate != 200 && rate != 400) return false;
  if (!i2cMutex || !xSemaphoreTake(i2cMutex, pdMS_TO_TICKS(50))) return false;
  ppg.setup(0x1F, 1, 2, rate, 215, 4096);
  ppg.setPulseAmplitudeRed(0x0A);
  ppg.setPulseAmplitudeIR(0x1F);
  ppg.setPulseAmplitudeGreen(0);
  ppg.setFIFOAverage(1);
  ppg.enableFIFORollover();
  ppg.clearFIFO();
  xSemaphoreGive(i2cMutex);
  ppgRateCfg = rate;
  return true;
}

static void ppgHardware(bool on) {
  if (!ppgOk || !i2cMutex) return;
  if (!xSemaphoreTake(i2cMutex, pdMS_TO_TICKS(50))) return;
  if (on) {
    ppg.wakeUp();
  } else {
    ppg.shutDown();
  }
  xSemaphoreGive(i2cMutex);
}

static void processPpgBeat(uint32_t ir, uint32_t sampleTs) {
  if (ir < 20000UL) return;
  if (!checkForBeat((long)ir)) return;

  if (lastBeatTsMs != 0) {
    uint32_t ibi = sampleTs - lastBeatTsMs;
    if (ibi >= 300 && ibi <= 1500) {
      ppgRrMs = (uint16_t)ibi;
      uint16_t inst = (uint16_t)(60000UL / ibi);
      if (hrHistCount < 4) hrHist[hrHistCount++] = inst;
      else {
        hrHist[0] = hrHist[1];
        hrHist[1] = hrHist[2];
        hrHist[2] = hrHist[3];
        hrHist[3] = inst;
      }
      uint32_t sum = 0;
      for (uint8_t i = 0; i < hrHistCount; ++i) sum += hrHist[i];
      if (hrHistCount) ppgHr = (uint16_t)(sum / hrHistCount);
    }
  }
  lastBeatTsMs = sampleTs;
}

static void ppgTask(void *arg) {
  (void)arg;
  bool hwOn = false;
  uint32_t lastPoll = 0;

  for (;;) {
    bool want = (runState == STATE_RUNNING && enPpg && ppgOk && !deepSleeping);
    if (!want) {
      if (hwOn) {
        ppgHardware(false);
        hwOn = false;
      }
      vTaskDelay(pdMS_TO_TICKS(10));
      continue;
    }

    if (!hwOn) {
      ppgHardware(true);
      hwOn = true;
      lastPoll = nowMs();
      vTaskDelay(pdMS_TO_TICKS(5));
    }

    uint32_t n = nowMs();
    if ((uint32_t)(n - lastPoll) < PPG_POLL_MS) {
      vTaskDelay(pdMS_TO_TICKS(1));
      continue;
    }
    lastPoll = n;

    if (!i2cMutex || !xSemaphoreTake(i2cMutex, pdMS_TO_TICKS(I2C_TIMEOUT_MS))) {
      vTaskDelay(pdMS_TO_TICKS(1));
      continue;
    }

    uint16_t newSamples = ppg.check();
    uint8_t count = ppg.available();
    if (newSamples > 0 && count > 0) {
      uint32_t now = sessionUs32();
      uint32_t periodUs = 1000000UL / (uint32_t)ppgRateCfg;
      uint32_t oldest = now - (uint32_t)(count - 1U) * periodUs;

      for (uint8_t i = 0; i < count; ++i) {
        uint32_t ir = ppg.getFIFOIR();
        uint32_t red = ppg.getFIFORed();
        uint32_t ts = oldest + (uint32_t)i * periodUs;
        ppgPush({ir, red, ts});
        ppgSamples++;
        processPpgBeat(ir, ts / 1000UL);
        ppg.nextSample();
      }
    }
    xSemaphoreGive(i2cMutex);
  }
}

// ----------------------------------------------------------------------
// Slow telemetry task
// ----------------------------------------------------------------------
static void slowTask(void *arg) {
  (void)arg;
  uint32_t lastSample = 0;
  uint32_t lastTemp = 0;
  uint32_t lastBatt = 0;

  for (;;) {
    if (runState != STATE_RUNNING || deepSleeping) {
      vTaskDelay(pdMS_TO_TICKS(20));
      continue;
    }

    uint32_t n = nowMs();
    if ((uint32_t)(n - lastSample) >= TELEMETRY_PERIOD_MS) {
      lastSample = n;
      lastLo = (uint8_t)((digitalRead(PIN_LOP) == HIGH || digitalRead(PIN_LON) == HIGH) ? 1 : 0);

      if (enTemp && (uint32_t)(n - lastTemp) >= 500) {
        lastTemp = n;
        float t = 0.0f;
        if (max30205Read(t)) {
          lastTempC = t;
          tempOk = true;
        } else {
          tempOk = false;
        }
      }

      if (enBattery && (uint32_t)(n - lastBatt) >= 500) {
        lastBatt = n;
        uint32_t mv = analogReadMilliVolts(PIN_VBAT);
        lastBattV = (mv / 1000.0f) * 2.0f;
      }

      telPush({sessionUs32(), lastGsrUs, lastThoraxV, lastAbdomenV, lastTempC, lastBattV,
               ppgHr, ppgRrMs, lastLo, (uint8_t)adsOk, (uint8_t)ppgOk, (uint8_t)tempOk});
    }
    vTaskDelay(pdMS_TO_TICKS(5));
  }
}

// ----------------------------------------------------------------------
// BLE
// ----------------------------------------------------------------------
static uint16_t negotiatedMtu() {
  if (!bleServer || !bleConnected) return DEFAULT_ATT_MTU;
  uint16_t mtu = bleServer->getPeerMTU(bleServer->getConnId());
  if (mtu < DEFAULT_ATT_MTU) mtu = DEFAULT_ATT_MTU;
  return mtu;
}

static uint16_t bleValueMax() {
  uint16_t mtu = negotiatedMtu();
  return (mtu > 3) ? (uint16_t)(mtu - 3) : 20;
}

static bool bleNotify(BLECharacteristic *ch, const uint8_t *data, size_t len) {
  if (!txBle || !bleConnected || ch == nullptr) return false;
  uint16_t maxLen = bleValueMax();
  if (len > maxLen) {
    bleNotifyRejected++;
    return false;
  }
  if (!bleMutex || xSemaphoreTake(bleMutex, pdMS_TO_TICKS(10)) != pdTRUE) {
    bleNotifyRejected++;
    return false;
  }
  ch->setValue((uint8_t *)data, len);
  ch->notify();
  xSemaphoreGive(bleMutex);
  bleNotifyAttempts++;
  return true;
}

static void bleText(BLECharacteristic *ch, const char *txt) {
  if (!ch || !bleConnected || !txBle) return;
  size_t n = strlen(txt);
  uint16_t maxLen = bleValueMax();
  if (n > maxLen) n = maxLen;
  if (!bleMutex || xSemaphoreTake(bleMutex, pdMS_TO_TICKS(10)) != pdTRUE) return;
  ch->setValue((uint8_t *)txt, n);
  ch->notify();
  xSemaphoreGive(bleMutex);
}

static void sendResponse(const char *txt) {
  if (txUsb) Serial.println(txt);
  bleText(chCmd, txt);
}

static void sendEvent(const char *txt) {
  if (txUsb) {
    Serial.print("EV ");
    Serial.println(txt);
  }
  uint8_t frame[80];
  size_t textLen = strlen(txt);
  if (textLen > 50) textLen = 50;
  const size_t len = 12 + textLen + 2;
  frame[0] = PACKET_MAGIC;
  frame[1] = 4;
  frame[2] = PROTOCOL_VERSION;
  frame[3] = 0;
  putU16(&frame[4], evtSeq++);
  putU32(&frame[6], sessionUs32());
  putU16(&frame[10], 0);
  frame[12] = (uint8_t)textLen;
  memcpy(&frame[13], txt, textLen);
  uint16_t crc = crc16ccitt(frame, len - 2);
  putU16(&frame[len - 2], crc);
  if (bleNotify(chEvt, frame, len)) eventPackets++;
}

// ----------------------------------------------------------------------
// Frame builders
// ----------------------------------------------------------------------
static bool sendEcgFrame() {
  const bool wantBle = streamEcg && bleConnected && txBle;
  const bool wantUsb = streamEcg && txUsb && usbEcgStream;
  if (!streamEcg || (!wantBle && !wantUsb)) return false;
  const uint16_t maxLen = wantBle ? bleValueMax() : 252;
  if (maxLen < 14) return false;

  uint8_t maxSamples = (uint8_t)((maxLen - 14) / 2); // 12 hdr + 2 CRC
  if (maxSamples > 80) maxSamples = 80;
  if (maxSamples < 1) return false;
  if (ecgCount() < maxSamples) return false;

  ESample samples[80];
  for (uint8_t i = 0; i < maxSamples; ++i) {
    if (!ecgPop(samples[i])) return false;
  }

  uint8_t frame[256];
  frame[0] = PACKET_MAGIC;
  frame[1] = 1;
  frame[2] = PROTOCOL_VERSION;
  frame[3] = 0;
  putU16(&frame[4], ecgSeq++);
  putU32(&frame[6], samples[0].tsUs);

  uint32_t periodUs = 1000000UL / (uint32_t)((effectiveEcgRate() > 0) ? effectiveEcgRate() : ECG_RATE_DEFAULT);
  if (maxSamples > 1) {
    uint32_t dtUs = samples[maxSamples - 1].tsUs - samples[0].tsUs;
    if (dtUs > 0) {
      periodUs = (uint32_t)((uint64_t)dtUs / (uint64_t)(maxSamples - 1U));
    }
  }
  if (periodUs > 65535UL) periodUs = 65535UL;
  putU16(&frame[10], (uint16_t)periodUs);
  frame[12] = maxSamples;

  for (uint8_t i = 0; i < maxSamples; ++i) {
    putU16(&frame[13 + 2U * i], (uint16_t)samples[i].raw);
  }

  const size_t len = 13 + 2U * maxSamples + 2;
  uint16_t crc = crc16ccitt(frame, len - 2);
  putU16(&frame[len - 2], crc);
  bool delivered = false;
  if (wantBle) delivered = bleNotify(chEcg, frame, len) || delivered;
  if (wantUsb) {
    Serial.write(frame, len);
    delivered = true;
  }
  if (!delivered) return false;
  ecgPackets++;
  return true;
}

static bool sendPpgFrame() {
  const bool wantBle = streamPpg && bleConnected && txBle;
  const bool wantUsb = streamPpg && txUsb && usbPpgStream;
  if (!streamPpg || (!wantBle && !wantUsb)) return false;
  const uint16_t maxLen = wantBle ? bleValueMax() : 252;
  if (maxLen < 20) return false;

  uint8_t maxSamples = (uint8_t)((maxLen - 14) / 6); // 12 hdr + 2 CRC
  if (maxSamples > 30) maxSamples = 30;
  if (maxSamples < 1) return false;
  if (ppgCount() < maxSamples) return false;

  PSample samples[30];
  for (uint8_t i = 0; i < maxSamples; ++i) {
    if (!ppgPop(samples[i])) return false;
  }

  uint8_t frame[256];
  frame[0] = PACKET_MAGIC;
  frame[1] = 2;
  frame[2] = PROTOCOL_VERSION;
  frame[3] = 0;
  putU16(&frame[4], ppgSeq++);
  putU32(&frame[6], samples[0].tsUs);

  uint32_t periodUs = 1000000UL / (uint32_t)((ppgRateCfg > 0) ? ppgRateCfg : PPG_RATE_DEFAULT);
  if (maxSamples > 1) {
    uint32_t dtUs = samples[maxSamples - 1].tsUs - samples[0].tsUs;
    if (dtUs > 0) periodUs = (uint32_t)((uint64_t)dtUs / (uint64_t)(maxSamples - 1U));
  }
  if (periodUs > 65535UL) periodUs = 65535UL;
  putU16(&frame[10], (uint16_t)periodUs);
  frame[12] = maxSamples;

  for (uint8_t i = 0; i < maxSamples; ++i) {
    uint8_t *p = &frame[13 + 6U * i];
    putU24(p, samples[i].ir);
    putU24(p + 3, samples[i].red);
  }

  const size_t len = 13 + 6U * maxSamples + 2;
  uint16_t crc = crc16ccitt(frame, len - 2);
  putU16(&frame[len - 2], crc);
  bool delivered = false;
  if (wantBle) delivered = bleNotify(chPpg, frame, len) || delivered;
  if (wantUsb) {
    Serial.write(frame, len);
    delivered = true;
  }
  if (!delivered) return false;
  ppgPackets++;
  return true;
}

static bool sendTelemetryFrame(const SlowSample &s) {
  if (!streamTelemetry || !bleConnected || !txBle) return false;
  const size_t payloadLen = 26;
  const size_t len = 13 + payloadLen + 2;
  if (bleValueMax() < len) return false;

  uint8_t frame[64];
  frame[0] = PACKET_MAGIC;
  frame[1] = 3;
  frame[2] = PROTOCOL_VERSION;
  frame[3] = 0;
  putU16(&frame[4], telSeq++);
  putU32(&frame[6], s.tsUs);
  putU16(&frame[10], 0);
  frame[12] = 0;

  uint8_t *p = &frame[13];
  uint32_t g = clampU32((long long)(s.gsrUs * 1000.0f));
  putU32(p, g); p += 4;                    // uS x1000
  int t100 = (int)lrintf(s.tempC * 100.0f);
  putU16(p, (uint16_t)(int16_t)constrain(t100, -32768, 32767)); p += 2;
  putU16(p, clampU16((long)lrintf(s.battV * 1000.0f))); p += 2;
  putU16(p, clampU16((long)lrintf(s.thoraxV * 1000.0f))); p += 2;
  putU16(p, clampU16((long)lrintf(s.abdomenV * 1000.0f))); p += 2;
  putU16(p, s.hr); p += 2;
  putU16(p, s.rrMs); p += 2;
  uint16_t flags = 0;
  if (s.adsOk) flags |= 1U;
  if (s.ppgOk) flags |= 2U;
  if (s.tempOk) flags |= 4U;
  if (enEcg) flags |= 8U;
  if (enGsr) flags |= 16U;
  if (enThorax) flags |= 32U;
  if (enAbdomen) flags |= 64U;
  if (s.lo) flags |= 128U;
  putU16(p, flags); p += 2;
  putU16(p, (uint16_t)((ecgCount() > 65535U) ? 65535U : ecgCount())); p += 2;
  putU16(p, (uint16_t)((ppgCount() > 65535U) ? 65535U : ppgCount())); p += 2;
  putU16(p, (uint16_t)((telCount() > 65535U) ? 65535U : telCount())); p += 2;

  uint16_t crc = crc16ccitt(frame, len - 2);
  putU16(&frame[len - 2], crc);
  if (!bleNotify(chTel, frame, len)) return false;
  telPackets++;
  return true;
}

static void sendLegacyJson() {
  if (!streamJson) return;
  char json[320];
  snprintf(json, sizeof(json),
           "{\"v\":4,\"ms\":%lu,\"ecg_v\":%.4f,\"gsr_uS\":%.3f,\"rt_v\":%.4f,\"ra_v\":%.4f,\"t_c\":%.2f,\"batt_v\":%.3f,\"hr\":%u,\"rr_ms\":%u,\"lo\":%u,\"ads\":%u,\"ppg\":%u,\"ecg_q\":%u,\"ppg_q\":%u}",
           (unsigned long)nowMs(), lastEcgV, lastGsrUs, lastThoraxV, lastAbdomenV,
           lastTempC, lastBattV, ppgHr, ppgRrMs, lastLo,
           (unsigned)adsOk, (unsigned)ppgOk,
           (unsigned)ecgCount(), (unsigned)ppgCount());
  if (txUsb) Serial.println(json);
  bleText(chJson, json);
}

static void notifyHr() {
  static uint16_t lastHr = 0;
  static uint16_t lastRr = 0;
  if (!bleConnected || !txBle || !chHr || ppgHr == 0 || ppgRrMs == 0) return;
  if (ppgHr == lastHr && ppgRrMs == lastRr) return;

  // Heart Rate Measurement Flags: bit4 = RR-Interval present;
  // bit0 = 0 means UINT8 heart rate format.
  uint8_t pkt[4];
  pkt[0] = 0x10;
  pkt[1] = (uint8_t)((ppgHr > 255U) ? 255U : ppgHr);
  uint16_t rr1024 = (uint16_t)(((uint32_t)ppgRrMs * 1024UL) / 1000UL);
  pkt[2] = (uint8_t)(rr1024 & 0xFFU);
  pkt[3] = (uint8_t)(rr1024 >> 8);
  // Send exactly the four bytes defined by the selected HRS flags.
  chHr->setValue(pkt, 4);
  chHr->notify();
  lastHr = ppgHr;
  lastRr = ppgRrMs;
}

// ----------------------------------------------------------------------
// Configuration persistence
// ----------------------------------------------------------------------
static void markConfigDirty() {
  configDirty = true;
  configDirtySinceMs = nowMs();
}

static void saveConfig() {
  prefs.begin("pinabio", false);
  prefs.putBool("ecg", enEcg);
  prefs.putBool("gsr", enGsr);
  prefs.putBool("thorax", enThorax);
  prefs.putBool("abd", enAbdomen);
  prefs.putBool("ppg", enPpg);
  prefs.putBool("temp", enTemp);
  prefs.putBool("bat", enBattery);
  prefs.putUShort("ecgr", ecgRateCfg);
  prefs.putUShort("ppgr", ppgRateCfg);
  prefs.putUShort("gsrr", gsrRateCfg);
  prefs.putUShort("thr", thoraxRateCfg);
  prefs.putUShort("abr", abdomenRateCfg);
  prefs.putBool("auto", autoEcgBoost);
  prefs.putFloat("vref", gsrVref);
  prefs.end();
  configDirty = false;
}

static void loadConfig() {
  prefs.begin("pinabio", true);
  enEcg = prefs.getBool("ecg", true);
  enGsr = prefs.getBool("gsr", true);
  enThorax = prefs.getBool("thorax", true);
  enAbdomen = prefs.getBool("abd", true);
  enPpg = prefs.getBool("ppg", true);
  enTemp = prefs.getBool("temp", true);
  enBattery = prefs.getBool("bat", true);
  ecgRateCfg = prefs.getUShort("ecgr", ECG_RATE_DEFAULT);
  ppgRateCfg = prefs.getUShort("ppgr", PPG_RATE_DEFAULT);
  gsrRateCfg = prefs.getUShort("gsrr", GSR_RATE_DEFAULT);
  thoraxRateCfg = prefs.getUShort("thr", THORAX_RATE_DEFAULT);
  abdomenRateCfg = prefs.getUShort("abr", ABDOMEN_RATE_DEFAULT);
  autoEcgBoost = prefs.getBool("auto", true);
  gsrVref = prefs.getFloat("vref", GSR_VREF_DEFAULT);
  prefs.end();

  if (ecgRateCfg < 20 || ecgRateCfg > ECG_RATE_MAX) ecgRateCfg = ECG_RATE_DEFAULT;
  if (!(ppgRateCfg == 100 || ppgRateCfg == 200 || ppgRateCfg == 400)) ppgRateCfg = PPG_RATE_DEFAULT;
  if (gsrRateCfg < 1 || gsrRateCfg > 100) gsrRateCfg = GSR_RATE_DEFAULT;
  if (thoraxRateCfg < 1 || thoraxRateCfg > 100) thoraxRateCfg = THORAX_RATE_DEFAULT;
  if (abdomenRateCfg < 1 || abdomenRateCfg > 100) abdomenRateCfg = ABDOMEN_RATE_DEFAULT;
  if (gsrVref < 0.45f || gsrVref > 0.55f || !isfinite(gsrVref)) gsrVref = GSR_VREF_DEFAULT;
}

static void defaultsConfig() {
  enEcg = enGsr = enThorax = enAbdomen = enPpg = enTemp = enBattery = true;
  streamEcg = streamPpg = streamTelemetry = true;
  streamJson = false;
  txUsb = true;
  txBle = true;
  autoEcgBoost = true;
  ecgRateCfg = ECG_RATE_DEFAULT;
  ppgRateCfg = PPG_RATE_DEFAULT;
  gsrRateCfg = GSR_RATE_DEFAULT;
  thoraxRateCfg = THORAX_RATE_DEFAULT;
  abdomenRateCfg = ABDOMEN_RATE_DEFAULT;
  gsrVref = GSR_VREF_DEFAULT;
  markConfigDirty();
  adcScheduleDirty = true;
}

// ----------------------------------------------------------------------
// Command processing
// ----------------------------------------------------------------------
static void uppercaseInPlace(char *s) {
  for (; *s; ++s) *s = (char)toupper((unsigned char)*s);
}

static bool parseOnOff(const char *s, bool &dst) {
  if (!s) return false;
  if (!strcasecmp(s, "ON") || !strcmp(s, "1")) { dst = true; return true; }
  if (!strcasecmp(s, "OFF") || !strcmp(s, "0")) { dst = false; return true; }
  return false;
}

static void statusText(char *out, size_t n) {
  snprintf(out, n,
           "STATUS run=%u BLE=%u MTU=%u ECG=%u/%u GSR=%u TH=%u AB=%u PPG=%u TEMP=%u BAT=%u "
           "stream=%u/%u/%u JSON=%u USB=%u BLEtx=%u "
           "rates ECGcfg=%u ECGeff=%u PPG=%u GSR=%u TH=%u AB=%u "
           "q=%u/%u/%u drops=%lu/%lu errors=%lu,%lu,%lu,%lu adcSPS=%lu,%lu,%lu,%lu",
           (unsigned)runState, (unsigned)bleConnected, (unsigned)negotiatedMtu(),
           (unsigned)enEcg, (unsigned)ecgRateCfg, (unsigned)enGsr, (unsigned)enThorax,
           (unsigned)enAbdomen, (unsigned)enPpg, (unsigned)enTemp, (unsigned)enBattery,
           (unsigned)streamEcg, (unsigned)streamPpg, (unsigned)streamTelemetry,
           (unsigned)streamJson, (unsigned)txUsb, (unsigned)txBle,
           (unsigned)ecgRateCfg, (unsigned)effectiveEcgRate(), (unsigned)ppgRateCfg,
           (unsigned)gsrRateCfg, (unsigned)thoraxRateCfg, (unsigned)abdomenRateCfg,
           (unsigned)ecgCount(), (unsigned)ppgCount(), (unsigned)telCount(),
           (unsigned long)ecgDrops, (unsigned long)ppgDrops,
           (unsigned long)adcErrors[0], (unsigned long)adcErrors[1],
           (unsigned long)adcErrors[2], (unsigned long)adcErrors[3],
           (unsigned long)adcSamples[0], (unsigned long)adcSamples[1],
           (unsigned long)adcSamples[2], (unsigned long)adcSamples[3]);
}

static void handleCommand(char *cmd) {
  if (!cmd) return;
  while (*cmd == ' ' || *cmd == '\t' || *cmd == '\r' || *cmd == '\n') cmd++;
  if (*cmd == '\0') return;
  uppercaseInPlace(cmd);

  if (!strcmp(cmd, "HELP")) {
    sendResponse("OK COMMANDS: START STOP STATUS HELP DEFAULTS SAVE SLEEP; ECG/GSR/THORAX/ABDOMEN/PPG/TEMP/BAT ON|OFF; ECG_STREAM/PPG_STREAM/TELEM_STREAM/JSON/USB_ECG/USB_PPG/USB/BLE ON|OFF; SET ECG_RATE n; SET PPG_RATE 100|200|400; SET GSR_RATE n; SET THORAX_RATE n; SET ABDOMEN_RATE n; SET AUTO_ECG ON|OFF; SET VREF x.xxxx");
    return;
  }

  if (!strcmp(cmd, "STATUS")) {
    char s[600];
    statusText(s, sizeof(s));
    sendResponse(s);
    return;
  }

  if (!strcmp(cmd, "START")) {
    runState = STATE_RUNNING;
    clearBuffers();
    sessionStartMs = nowMs();
    sessionStartUs = (uint64_t)esp_timer_get_time();
    lastBeatTsMs = 0;
    ppgHr = 0;
    ppgRrMs = 0;
    hrHistCount = 0;
    memset(hrHist, 0, sizeof(hrHist));
    adcScheduleDirty = true;
    sendEvent("START");
    sendResponse("OK START");
    return;
  }

  if (!strcmp(cmd, "STOP")) {
    runState = STATE_STOPPED;
    clearBuffers();
    sendEvent("STOP");
    sendResponse("OK STOP");
    return;
  }

  if (!strcmp(cmd, "SAVE")) {
    saveConfig();
    sendResponse("OK SAVE");
    return;
  }

  if (!strcmp(cmd, "DEFAULTS")) {
    defaultsConfig();
    sendResponse("OK DEFAULTS");
    return;
  }

  if (!strcmp(cmd, "SLEEP")) {
    sleepRequested = true;
    sendResponse("OK SLEEP REQUEST");
    return;
  }

  bool handled = false;
  struct BoolCmd { const char *name; bool *dst; bool affectsAdc; } cmds[] = {
    {"ECG", &enEcg, true}, {"GSR", &enGsr, true}, {"THORAX", &enThorax, true},
    {"ABDOMEN", &enAbdomen, true}, {"PPG", &enPpg, false}, {"TEMP", &enTemp, false},
    {"BAT", &enBattery, false}, {"ECG_STREAM", &streamEcg, false}, {"PPG_STREAM", &streamPpg, false},
    {"TELEM_STREAM", &streamTelemetry, false}, {"JSON", &streamJson, false},
    {"USB_ECG", &usbEcgStream, false}, {"USB_PPG", &usbPpgStream, false},
    {"USB", &txUsb, false}, {"BLE", &txBle, false}
  };

  for (size_t i = 0; i < sizeof(cmds)/sizeof(cmds[0]); ++i) {
    size_t L = strlen(cmds[i].name);
    if (!strncasecmp(cmd, cmds[i].name, L) &&
        (cmd[L] == ' ' || cmd[L] == ':' || cmd[L] == '=')) {
      bool b = false;
      if (parseOnOff(cmd + L + 1, b)) {
        *cmds[i].dst = b;
        if (cmds[i].affectsAdc) adcScheduleDirty = true;
        markConfigDirty();
        sendResponse("OK");
      } else {
        sendResponse("ERR EXPECTED ON|OFF");
      }
      handled = true;
      break;
    }
  }
  if (handled) return;

  if (!strncasecmp(cmd, "SET ", 4)) {
    char key[48] = {0};
    char val[48] = {0};
    if (sscanf(cmd + 4, "%47[^ =:]%*[ =:]%47s", key, val) == 2) {
      long x = atol(val);
      if (!strcmp(key, "ECG_RATE")) {
        if (x >= 20 && x <= ECG_RATE_MAX) {
          ecgRateCfg = (uint16_t)x;
          if (adcTotalBudget() <= ADC_BUDGET_SPS) {
            adcScheduleDirty = true; markConfigDirty(); sendResponse("OK SET ECG_RATE"); return;
          }
        }
        sendResponse("ERR ECG_RATE budget/range"); return;
      }
      if (!strcmp(key, "PPG_RATE")) {
        if ((x == 100 || x == 200 || x == 400) && configurePpg((uint16_t)x)) {
          markConfigDirty(); sendResponse("OK SET PPG_RATE"); return;
        }
        sendResponse("ERR PPG_RATE"); return;
      }
      if (!strcmp(key, "GSR_RATE")) {
        if (x >= 1 && x <= 100) {
          gsrRateCfg = (uint16_t)x;
          if (adcTotalBudget() <= ADC_BUDGET_SPS) {
            adcScheduleDirty = true; markConfigDirty(); sendResponse("OK SET GSR_RATE"); return;
          }
        }
        sendResponse("ERR GSR_RATE budget/range"); return;
      }
      if (!strcmp(key, "THORAX_RATE")) {
        if (x >= 1 && x <= 100) {
          thoraxRateCfg = (uint16_t)x;
          if (adcTotalBudget() <= ADC_BUDGET_SPS) {
            adcScheduleDirty = true; markConfigDirty(); sendResponse("OK SET THORAX_RATE"); return;
          }
        }
        sendResponse("ERR THORAX_RATE budget/range"); return;
      }
      if (!strcmp(key, "ABDOMEN_RATE")) {
        if (x >= 1 && x <= 100) {
          abdomenRateCfg = (uint16_t)x;
          if (adcTotalBudget() <= ADC_BUDGET_SPS) {
            adcScheduleDirty = true; markConfigDirty(); sendResponse("OK SET ABDOMEN_RATE"); return;
          }
        }
        sendResponse("ERR ABDOMEN_RATE budget/range"); return;
      }
      if (!strcmp(key, "AUTO_ECG")) {
        bool b = false;
        if (parseOnOff(val, b)) {
          autoEcgBoost = b;
          if (adcTotalBudget() <= ADC_BUDGET_SPS || b) {
            adcScheduleDirty = true; markConfigDirty(); sendResponse("OK SET AUTO_ECG"); return;
          }
        }
        sendResponse("ERR AUTO_ECG"); return;
      }
      if (!strcmp(key, "VREF")) {
        float v = atof(val);
        if (isfinite(v) && v >= 0.45f && v <= 0.55f) {
          gsrVref = v; markConfigDirty(); sendResponse("OK SET VREF"); return;
        }
        sendResponse("ERR VREF range"); return;
      }
    }
  }

  sendResponse("ERR UNKNOWN_COMMAND");
}

static void processUsbCommands() {
  static char line[128];
  static size_t n = 0;
  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\n' || c == '\r') {
      if (n > 0) {
        line[n] = '\0';
        handleCommand(line);
        n = 0;
      }
    } else if (n < sizeof(line) - 1) {
      line[n++] = c;
    } else {
      n = 0;
      sendResponse("ERR COMMAND_TOO_LONG");
    }
  }
}

class CmdCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *c) override {
    std::string v = c->getValue();
    if (v.empty() || cmdQueue == nullptr) return;
    CmdMsg msg{};
    size_t n = v.size();
    if (n >= sizeof(msg.text)) n = sizeof(msg.text) - 1;
    memcpy(msg.text, v.data(), n);
    msg.text[n] = '\0';
    if (xQueueSend(cmdQueue, &msg, 0) != pdTRUE) cmdDrops++;
  }
};

static void processBleCommands() {
  if (!cmdQueue) return;
  CmdMsg msg;
  while (xQueueReceive(cmdQueue, &msg, 0) == pdTRUE) {
    handleCommand(msg.text);
  }
}

// ----------------------------------------------------------------------
// Sleep
// ----------------------------------------------------------------------
static void performDeepSleep() {
  if (deepSleeping) return;
  deepSleeping = true;
  runState = STATE_STOPPED;

  if (ppgOk) ppgHardware(false);
  sendEvent("SHUTDOWN");
  vTaskDelay(pdMS_TO_TICKS(20));

  if (adcTaskHandle) vTaskSuspend(adcTaskHandle);
  if (ppgTaskHandle) vTaskSuspend(ppgTaskHandle);
  if (slowTaskHandle) vTaskSuspend(slowTaskHandle);
  if (commsTaskHandle) vTaskSuspend(commsTaskHandle);

  BLEDevice::deinit(true);
  rtc_gpio_pullup_en(GPIO_NUM_4);
  rtc_gpio_pulldown_dis(GPIO_NUM_4);
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_4, 1);
  esp_deep_sleep_start();
}

static void sleepTask(void *arg) {
  (void)arg;
  uint32_t lowSince = 0;
  for (;;) {
    bool low = (digitalRead(PIN_SLEEP) == LOW);
    if (low) {
      if (lowSince == 0) lowSince = nowMs();
      if ((uint32_t)(nowMs() - lowSince) >= SLEEP_HOLD_MS || sleepRequested) {
        performDeepSleep();
      }
    } else {
      lowSince = 0;
      sleepRequested = false;
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

// ----------------------------------------------------------------------
// Diagnostics
// ----------------------------------------------------------------------
static void printDiagnostics() {
  static uint32_t lastMs = 0;
  static uint32_t lastCount[4] = {0,0,0,0};
  uint32_t n = nowMs();
  float sec = (lastMs == 0 || n <= lastMs) ? 1.0f : (float)(n - lastMs) / 1000.0f;
  uint32_t d0 = adcSamples[0] - lastCount[0];
  uint32_t d1 = adcSamples[1] - lastCount[1];
  uint32_t d2 = adcSamples[2] - lastCount[2];
  uint32_t d3 = adcSamples[3] - lastCount[3];

  Serial.printf("DIAG ADC_SPS=%.1f,%.1f,%.1f,%.1f ECGq=%u PPGq=%u TELq=%u "
                "ECGpk=%lu PPGpk=%lu drops=%lu/%lu errs=%lu,%lu,%lu,%lu mux=%lu "
                "BLE=%u MTU=%u reject=%lu\n",
                d0/sec, d1/sec, d2/sec, d3/sec,
                (unsigned)ecgCount(), (unsigned)ppgCount(), (unsigned)telCount(),
                (unsigned long)ecgPackets, (unsigned long)ppgPackets,
                (unsigned long)ecgDrops, (unsigned long)ppgDrops,
                (unsigned long)adcErrors[0], (unsigned long)adcErrors[1],
                (unsigned long)adcErrors[2], (unsigned long)adcErrors[3],
                (unsigned long)adcMuxChanges,
                (unsigned)bleConnected, (unsigned)negotiatedMtu(),
                (unsigned long)bleNotifyRejected);

  lastCount[0] = adcSamples[0];
  lastCount[1] = adcSamples[1];
  lastCount[2] = adcSamples[2];
  lastCount[3] = adcSamples[3];
  lastMs = n;
}

static void commsTask(void *arg) {
  (void)arg;
  for (;;) {
    processUsbCommands();
    processBleCommands();

    uint32_t n = nowMs();
    if ((uint32_t)(n - lastTelemetryMs) >= TELEMETRY_PERIOD_MS) {
      lastTelemetryMs = n;
      SlowSample s;
      if (telPop(s)) {
        sendTelemetryFrame(s);
      }
      sendLegacyJson();
      notifyHr();
    }

    // Drain several high-rate frames per pass, but never block the CPU
    // indefinitely. The acquisition tasks continue independently.
    for (uint8_t i = 0; i < 3; ++i) {
      if (!sendEcgFrame()) break;
    }
    for (uint8_t i = 0; i < 2; ++i) {
      if (!sendPpgFrame()) break;
    }

    if (configDirty && (uint32_t)(n - configDirtySinceMs) >= 5000) saveConfig();
    if ((uint32_t)(n - lastDiagMs) >= DIAG_PERIOD_MS) {
      lastDiagMs = n;
      printDiagnostics();
    }

    vTaskDelay(pdMS_TO_TICKS(2));
  }
}

// ----------------------------------------------------------------------
// BLE setup
// ----------------------------------------------------------------------
static void setupBle() {
  BLEDevice::init(BLE_NAME);
  BLEDevice::setMTU(247);

  bleServer = BLEDevice::createServer();
  class ServerCb : public BLEServerCallbacks {
    void onConnect(BLEServer *s) override {
      (void)s;
      bleConnected = true;
    }
    void onDisconnect(BLEServer *s) override {
      (void)s;
      bleConnected = false;
      BLEDevice::startAdvertising();
    }
  };
  bleServer->setCallbacks(new ServerCb());

  BLEService *svc = bleServer->createService(UUID_SVC);

  chJson = svc->createCharacteristic(UUID_JSON,
      BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
  chJson->addDescriptor(new BLE2902());

  chCmd = svc->createCharacteristic(UUID_CMD,
      BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE |
      BLECharacteristic::PROPERTY_WRITE_NR | BLECharacteristic::PROPERTY_NOTIFY);
  chCmd->addDescriptor(new BLE2902());
  chCmd->setCallbacks(new CmdCallbacks());

  chEcg = svc->createCharacteristic(UUID_ECG, BLECharacteristic::PROPERTY_NOTIFY);
  chEcg->addDescriptor(new BLE2902());

  chPpg = svc->createCharacteristic(UUID_PPG, BLECharacteristic::PROPERTY_NOTIFY);
  chPpg->addDescriptor(new BLE2902());

  chTel = svc->createCharacteristic(UUID_TEL, BLECharacteristic::PROPERTY_NOTIFY);
  chTel->addDescriptor(new BLE2902());

  chEvt = svc->createCharacteristic(UUID_EVT,
      BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
  chEvt->addDescriptor(new BLE2902());

  svc->start();

  BLEService *hrSvc = bleServer->createService(BLEUUID((uint16_t)0x180D));
  chHr = hrSvc->createCharacteristic(BLEUUID((uint16_t)0x2A37),
      BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
  chHr->addDescriptor(new BLE2902());
  uint8_t emptyHr[2] = {0, 0};
  chHr->setValue(emptyHr, 2);

  BLECharacteristic *body = hrSvc->createCharacteristic(BLEUUID((uint16_t)0x2A38),
      BLECharacteristic::PROPERTY_READ);
  uint8_t bodyLoc = 3;
  body->setValue(&bodyLoc, 1);
  hrSvc->start();

  BLEAdvertising *adv = BLEDevice::getAdvertising();
  adv->addServiceUUID(UUID_SVC);
  adv->addServiceUUID(BLEUUID((uint16_t)0x180D));
  adv->setScanResponse(true);
  adv->setMinPreferred(0x06);
  BLEDevice::startAdvertising();
}

// ----------------------------------------------------------------------
// Setup
// ----------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  delay(150);

  pinMode(PIN_SLEEP, INPUT_PULLUP);
  pinMode(PIN_LOP, INPUT);
  pinMode(PIN_LON, INPUT);

  analogReadResolution(12);
  analogSetPinAttenuation(PIN_VBAT, ADC_11db);

  Wire.begin(PIN_SDA, PIN_SCL);
  Wire.setClock(400000);
  i2cMutex = xSemaphoreCreateMutex();
  bleMutex = xSemaphoreCreateMutex();
  cmdQueue = xQueueCreate(8, sizeof(CmdMsg));

  loadConfig();

  // Hardware probes.
  adsOk = probeI2C(ADS_ADDR);
  if (adsOk) Serial.println("OK ADS1115 0x49 present");
  else Serial.println("ERR ADS1115 0x49 not found");

  ppgOk = probeI2C(PPG_ADDR);
  if (ppgOk) Serial.println("OK MAX30102 0x57 present");
  else Serial.println("ERR MAX30102 0x57 not found");

  tempOk = probeI2C(TEMP_ADDR);
  if (tempOk) {
    float t = 0;
    tempOk = max30205Read(t);
    if (tempOk) lastTempC = t;
  }
  Serial.println(tempOk ? "OK MAX30205 0x48 present" : "ERR MAX30205 0x48 not found");

  if (ppgOk) {
    if (!configurePpg(ppgRateCfg)) {
      ppgOk = false;
      Serial.println("ERR MAX30102 configuration");
    }
  }

  if (adcTotalBudget() > ADC_BUDGET_SPS) {
    Serial.printf("WARN ADS target budget %u > %u; reducing ECG to base target\n",
                  (unsigned)adcTotalBudget(), (unsigned)ADC_BUDGET_SPS);
    ecgRateCfg = ECG_RATE_DEFAULT;
  }

  setupBle();

  sessionStartMs = nowMs();
  sessionStartUs = (uint64_t)esp_timer_get_time();
  lastTelemetryMs = nowMs();
  lastDiagMs = nowMs();

  xTaskCreatePinnedToCore(adcTask, "ADC", 8192, nullptr, 6, &adcTaskHandle, 1);
  xTaskCreatePinnedToCore(ppgTask, "PPG", 8192, nullptr, 5, &ppgTaskHandle, 1);
  xTaskCreatePinnedToCore(slowTask, "SLOW", 4096, nullptr, 2, &slowTaskHandle, 0);
  xTaskCreatePinnedToCore(commsTask, "COMMS", 8192, nullptr, 3, &commsTaskHandle, 0);
  xTaskCreatePinnedToCore(sleepTask, "SLEEP", 2048, nullptr, 7, &sleepTaskHandle, 0);

  Serial.println("PinaBiosensor V1 engineered firmware READY - WiFi disabled");
  char s[600];
  statusText(s, sizeof(s));
  Serial.println(s);
  sendEvent("BOOT");
}

void loop() {
  // All runtime work is in FreeRTOS tasks.
  vTaskDelay(pdMS_TO_TICKS(1000));
}
