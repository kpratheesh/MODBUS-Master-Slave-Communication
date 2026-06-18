# MODBUS Master-Slave Communication

An Arduino-based **Modbus RTU** communication system that polls multiple industrial sensors simultaneously over a shared RS-485 serial bus. A single **Master** Arduino queries three slave devices — a current meter, a flow rate meter, and a PID temperature controller — and prints live sensor readings to the Serial Monitor.

---

## 📦 Project Structure

```
MODBUS-Master-Slave-Communication/
├── master.ino   # Master Arduino: polls all slave devices via Modbus RTU
└── slave.ino    # Slave Arduino: reads ADS1115 ADC, serves data over Modbus RTU
```

---

## 🔌 System Architecture

```
┌─────────────────────┐
│   Master Arduino    │
│  (master.ino)       │
│  SoftwareSerial     │
│  A0 (TX), A1 (RX)  │
└────────┬────────────┘
         │ RS-485 / SoftwareSerial @ 4800 baud
         │
   ┌─────┴──────────────────────────┐
   │                                │
   ▼                                ▼
┌─────────────────┐      ┌──────────────────────┐
│ Slave ID: 4     │      │ Slave ID: 3           │
│ Current Meter   │      │ Flow Rate Meter       │
│ (slave.ino +   │      │ (slave.ino)           │
│  ADS1115 ADC)  │      └──────────────────────┘
└─────────────────┘
                          ┌──────────────────────┐
                          │ Slave ID: 2           │
                          │ SELEC PID500-U        │
                          │ Temperature Controller│
                          └──────────────────────┘
```

---

## ⚙️ Hardware Requirements

| Component | Purpose |
|---|---|
| Arduino Uno / Nano (×2 or more) | Master + Slave microcontrollers |
| ADS1115 16-bit ADC | Reads 4-channel analog current signals (I²C, addr `0x49`) |
| SELEC PID500-U | Temperature controller (Slave ID 2, Modbus RTU) |
| Flow Rate Meter | Flow sensor with Modbus RTU output (Slave ID 3) |
| Current Meter | 4-channel shunt-based current measurement (Slave ID 4) |
| RS-485 Module (×N) | Serial bus transceiver for each node |
| Shunt Resistors / CT Sensors | Current sensing inputs (<75 mV full-scale) |

---

## 📚 Libraries Required

Install these via the **Arduino Library Manager** or from GitHub:

| Library | Purpose |
|---|---|
| [`ModbusRTUMaster`](https://github.com/CMB27/ModbusRTUMaster) | Modbus RTU Master implementation |
| [`ModbusRTUSlave`](https://github.com/CMB27/ModbusRTUSlave) | Modbus RTU Slave implementation |
| [`Adafruit ADS1X15`](https://github.com/adafruit/Adafruit_ADS1X15) | ADS1115 ADC driver |
| `SoftwareSerial` | Built-in Arduino library |
| `EEPROM` | Built-in Arduino library |
| `Wire` | Built-in Arduino library (I²C) |

---

## 🚀 Getting Started

### 1. Clone the repository

```bash
git clone https://github.com/kpratheesh/MODBUS-Master-Slave-Communication.git
```

### 2. Install libraries

In the Arduino IDE: **Sketch → Include Library → Manage Libraries**, then search for and install:
- `ModbusRTUMaster` by C. M. Bulliner
- `ModbusRTUSlave` by C. M. Bulliner
- `Adafruit ADS1X15`

### 3. Flash the sketches

- Upload **`slave.ino`** to each slave Arduino
- Upload **`master.ino`** to the master Arduino

### 4. Wire the hardware

- Connect all RS-485 transceivers in parallel on the same A/B bus lines
- Connect ADS1115 to the slave Arduino via I²C (SDA/SCL), I²C address `0x49`
- Ensure a common GND across all nodes

### 5. Monitor

Open the **Serial Monitor** at **115200 baud** on the master to see live sensor readings.

---

## 🖥️ Serial Monitor Commands (Slave)

These commands are entered via the **Serial Monitor** on the **slave** Arduino:

| Command | Description |
|---|---|
| `$a1` – `$a4` | Read current in **Amps** from channel 1–4 |
| `$v1` – `$v4` | Read voltage in **millivolts** from channel 1–4 |
| `$sX=mv,amps` | Save calibration values (full-scale mV and Amps) for channel X |
| `$all` | Read voltage and current from all 4 channels |
| `slaveid=<N>` | Change the Modbus Slave ID (1–247), saved to EEPROM |
| `baud=<N>` | Change the baud rate (4800 / 9600 / 19200), saved to EEPROM |
| `status` | Print current Slave ID and Baud Rate |

> **Example calibration:** `$s1=75,400` sets channel 1 to 75 mV full-scale = 400 A

---

## 📡 Modbus Register Map (Slave — Current Meter)

| Register | Type | Description |
|---|---|---|
| Holding 0–3 | `uint16_t` | Voltage CH1–CH4 (raw mV × 100) |
| Holding 4–7 | `uint16_t` | Current CH1–CH4 (Amps × 100) |

**Scale factor:** divide register value by `100` to get physical units.

---

## 📊 Master Serial Output (Sample)

```
=== Current Meter Data ===
CH1 - Voltage: 12.34 mV | Current: 65.78 A
CH2 - Voltage: 11.90 mV | Current: 63.20 A

=== Flow Rate Data ===
 flowrate: 24.00 L/min | volume: 1500.00 mL

=== Temperature Data ===
PV: 85
SV: 90
```

---

## 🛠️ Configuration (Master)

Edit these values at the top of `master.ino` before flashing:

| Variable | Default | Description |
|---|---|---|
| `MODBUS_BAUD` | `4800` | Bus baud rate (must match all slaves) |
| `MODBUS_UNIT_ID1` | `4` | Slave ID of the Current Meter |
| `MODBUS_UNIT_ID2` | `3` | Slave ID of the Flow Rate Meter |
| `MODBUS_UNIT_ID3` | `2` | Slave ID of the Temperature Controller |

---

## 🔋 Calibration (Slave — Current Measurement)

The slave uses a **linear scaling model**:

```
Amps = (measured_mV / set_mV) × set_amps
```

Default calibration per channel: **75 mV → 400 A** (CT shunt).  
Calibration values are stored in EEPROM and persist across reboots.

---

## 🐛 Troubleshooting

| Symptom | Likely Cause | Fix |
|---|---|---|
| `response timeout` | Wrong slave ID or wiring issue | Check bus connections and slave ID |
| `crc error` | Baud rate mismatch | Ensure all nodes use the same baud rate |
| `frame error` | Electrical noise or wrong config | Check RS-485 termination resistors |
| All readings = 0 | ADS1115 not found | Verify I²C address is `0x49` and wiring |
| EEPROM loads bad ID | Corrupted EEPROM | `slaveid=1` to reset; defaults are applied automatically |

---

## 📅 Changelog

| Date | Note |
|---|---|
| 2025-07-09 | Slave sketch tested and confirmed working |
| 2024-10-27 | Master sketch last modified |
| 2023-07-22 | Initial creation |

---

## 📄 License

This project is open-source. Based on the [ModbusRTUMaster example](https://github.com/CMB27/ModbusRTUMaster) by **C. M. Bulliner**.
