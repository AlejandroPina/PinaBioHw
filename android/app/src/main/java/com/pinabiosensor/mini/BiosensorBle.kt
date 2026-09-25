package com.pinabiosensor.mini

import android.annotation.SuppressLint
import android.bluetooth.BluetoothDevice
import android.bluetooth.BluetoothGatt
import android.bluetooth.BluetoothGattCallback
import android.bluetooth.BluetoothGattCharacteristic
import android.bluetooth.BluetoothGattDescriptor
import android.bluetooth.BluetoothManager
import android.bluetooth.BluetoothProfile
import android.bluetooth.le.ScanCallback
import android.bluetooth.le.ScanFilter
import android.bluetooth.le.ScanResult
import android.bluetooth.le.ScanSettings
import android.content.Context
import android.os.Build
import android.os.Handler
import android.os.Looper
import android.os.ParcelUuid
import java.nio.charset.StandardCharsets
import java.util.UUID

class BiosensorBle(
    context: Context,
    private val onStatus: (String) -> Unit,
    private val onPacket: (Telemetry) -> Unit,
) {
    private val app = context.applicationContext
    private val manager = app.getSystemService(BluetoothManager::class.java)
    private val adapter = manager?.adapter
    private val scanner get() = adapter?.bluetoothLeScanner
    private var gatt: BluetoothGatt? = null
    private var scanning = false
    private val main = Handler(Looper.getMainLooper())
    private val stopScanRunnable = Runnable {
        if (scanning) {
            stopScan()
            onStatus("No aparece ${BleIds.DEVICE_NAME}. ¿Encendida, cerca, Bluetooth on?")
        }
    }

    private val serviceUuid = UUID.fromString(BleIds.SERVICE)
    private val jsonUuid = UUID.fromString(BleIds.JSON_CHAR)
    private val cccd = UUID.fromString("00002902-0000-1000-8000-00805f9b34fb")

    private val scanCb = object : ScanCallback() {
        @SuppressLint("MissingPermission")
        override fun onScanResult(callbackType: Int, result: ScanResult) {
            val name = result.device.name ?: result.scanRecord?.deviceName ?: ""
            val uuids = result.scanRecord?.serviceUuids?.map { it.uuid } ?: emptyList()
            val hit = name == BleIds.DEVICE_NAME ||
                name.contains("PinaBio", ignoreCase = true) ||
                serviceUuid in uuids
            if (!hit) return
            stopScan()
            onStatus("Encontrado. Conectando…")
            connect(result.device)
        }

        override fun onScanFailed(errorCode: Int) {
            onStatus("El escaneo BLE falló (código $errorCode).")
        }
    }

    private val gattCb = object : BluetoothGattCallback() {
        @SuppressLint("MissingPermission")
        override fun onConnectionStateChange(g: BluetoothGatt, status: Int, newState: Int) {
            if (newState == BluetoothProfile.STATE_CONNECTED) {
                onStatus("Conectado. Pidiendo MTU…")
                if (!g.requestMtu(247)) {
                    g.discoverServices()
                }
                main.postDelayed({
                    if (g.services.isEmpty()) g.discoverServices()
                }, 800)
            } else if (newState == BluetoothProfile.STATE_DISCONNECTED) {
                onStatus("Desconectado.")
                g.close()
                if (gatt === g) gatt = null
            }
        }

        @SuppressLint("MissingPermission")
        override fun onMtuChanged(g: BluetoothGatt, mtu: Int, status: Int) {
            onStatus("MTU $mtu. Leyendo servicios…")
            g.discoverServices()
        }

        @SuppressLint("MissingPermission")
        override fun onServicesDiscovered(g: BluetoothGatt, status: Int) {
            val ch = g.getService(serviceUuid)?.getCharacteristic(jsonUuid)
            if (ch == null) {
                onStatus("La placa no ofrece la característica JSON.")
                return
            }
            g.setCharacteristicNotification(ch, true)
            val desc = ch.getDescriptor(cccd) ?: return
            if (Build.VERSION.SDK_INT >= 33) {
                g.writeDescriptor(desc, BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE)
            } else {
                @Suppress("DEPRECATION")
                desc.value = BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE
                @Suppress("DEPRECATION")
                g.writeDescriptor(desc)
            }
            @Suppress("DEPRECATION")
            g.readCharacteristic(ch)
            onStatus("Suscrito. Esperando JSON…")
        }

        @Deprecated("Deprecated in Java")
        override fun onCharacteristicChanged(
            g: BluetoothGatt,
            characteristic: BluetoothGattCharacteristic,
        ) {
            if (Build.VERSION.SDK_INT < 33) {
                @Suppress("DEPRECATION")
                handleBytes(characteristic.value)
            }
        }

        override fun onCharacteristicChanged(
            g: BluetoothGatt,
            characteristic: BluetoothGattCharacteristic,
            value: ByteArray,
        ) {
            handleBytes(value)
        }

        @Deprecated("Deprecated in Java")
        override fun onCharacteristicRead(
            g: BluetoothGatt,
            characteristic: BluetoothGattCharacteristic,
            status: Int,
        ) {
            if (Build.VERSION.SDK_INT < 33 && status == BluetoothGatt.GATT_SUCCESS) {
                @Suppress("DEPRECATION")
                handleBytes(characteristic.value)
            }
        }

        override fun onCharacteristicRead(
            g: BluetoothGatt,
            characteristic: BluetoothGattCharacteristic,
            value: ByteArray,
            status: Int,
        ) {
            if (status == BluetoothGatt.GATT_SUCCESS) handleBytes(value)
        }
    }

    private fun handleBytes(value: ByteArray?) {
        if (value == null || value.isEmpty()) return
        val text = String(value, StandardCharsets.UTF_8)
        try {
            onPacket(Telemetry.parse(text))
        } catch (e: Exception) {
            onStatus("JSON inválido: ${e.message}")
        }
    }

    @SuppressLint("MissingPermission")
    fun startScan() {
        if (adapter == null || adapter?.isEnabled != true) {
            onStatus("Activa Bluetooth en el teléfono.")
            return
        }
        if (scanning) return
        scanning = true
        onStatus("Buscando ${BleIds.DEVICE_NAME} (20 s)…")
        main.removeCallbacks(stopScanRunnable)
        main.postDelayed(stopScanRunnable, 20_000)
        val filters = listOf(
            ScanFilter.Builder().setDeviceName(BleIds.DEVICE_NAME).build(),
            ScanFilter.Builder().setServiceUuid(ParcelUuid(serviceUuid)).build(),
        )
        val settings = ScanSettings.Builder()
            .setScanMode(ScanSettings.SCAN_MODE_LOW_LATENCY)
            .build()
        scanner?.startScan(filters, settings, scanCb)
    }

    @SuppressLint("MissingPermission")
    fun stopScan() {
        if (!scanning) return
        scanning = false
        main.removeCallbacks(stopScanRunnable)
        scanner?.stopScan(scanCb)
    }

    @SuppressLint("MissingPermission")
    private fun connect(device: BluetoothDevice) {
        gatt?.close()
        gatt = device.connectGatt(app, false, gattCb, BluetoothDevice.TRANSPORT_LE)
    }

    @SuppressLint("MissingPermission")
    fun disconnect() {
        stopScan()
        gatt?.disconnect()
        gatt?.close()
        gatt = null
        onStatus("Desconectado.")
    }
}
