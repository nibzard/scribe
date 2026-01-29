# Tab5 Device Specifications

**Product Name:** Tab5  
**SKU:** C145 (device only) / K145 (with battery kit)  
**Manufacturer:** M5Stack  

---

## Overview

Tab5 is a highly expandable, portable smart-IoT terminal development device designed for developers, integrating a dual-core architecture and rich hardware resources. It serves as a full-featured, easily expandable high-performance platform for smart-home control, remote monitoring, industrial automation, IoT prototyping, and education applications.

---

## Main Controller & Processing

| Specification | Details |
|---|---|
| **SoC** | ESP32-P4NRW32 (RISC-V 32-bit architecture) |
| **CPU Cores** | Dual-core @ 400MHz + LP Single-core @ 40MHz |
| **Flash** | 16MB |
| **PSRAM** | 32MB |

---

## Wireless Module

| Specification | Details |
|---|---|
| **Module** | ESP32-C6-MINI-1U |
| **Wi-Fi Standard** | 2.4 GHz Wi-Fi 6 |
| **Additional Protocols** | Thread, Zigbee support |
| **Antenna System** | Built-in 3D antenna + 2× MMCX external antenna ports (user-switchable) |
| **Antenna Switching** | Controlled via RF_PTH_L_INT_H_EXT pin (LOW = internal, HIGH = external) |

---

## Display

| Specification | Details |
|---|---|
| **Size** | 5 inches |
| **Resolution** | 1280×720 (720P) |
| **Panel Type** | IPS TFT LCD |
| **Interface** | MIPI-DSI (dedicated pins) |
| **Touch Controller** | ST7123 (integrated display-touch driver) or GT911 alternative |
| **Display Driver** | ILI9881C or ST7123 (see Driver Change Note below) |
| **Brightness Control** | Via G22 LED pin (LEDA) |

**Driver Change Note:** Starting October 14, 2025, Tab5 units ship with integrated ST7123 display-touch driver instead of separate ILI9881C and GT911 components. Check device sticker to confirm driver model. Latest M5Unified and M5GFX are compatible; legacy firmware may require recompilation.

---

## Camera

| Specification | Details |
|---|---|
| **Sensor** | SC2356 |
| **Resolution** | 2MP (1600×1200) |
| **Interface** | MIPI-CSI (dedicated pins) |
| **Capabilities** | HD video recording, image processing, edge AI (facial recognition, object tracking) |

---

## Audio System

| Specification | Details |
|---|---|
| **Audio Codec** | ES8388 |
| **AEC Front-End** | ES7210 with dual-microphone array |
| **Microphone Configuration** | Dual-mic array with AEC echo cancellation |
| **Speaker** | 1W @ 8Ω NS4150B amplifier |
| **Speaker Enable** | Controlled via PI4IOE5V6408-1 (SPK_EN) |
| **Headphone Jack** | 3.5mm stereo jack with headphone detection |
| **Audio Features** | Hi-Fi recording/playback, accurate voice recognition |

---

## Motion & Orientation Sensor

| Specification | Details |
|---|---|
| **Sensor** | BMI270 6-axis MEMS |
| **Axes** | Accelerometer + Gyroscope |
| **I2C Address** | 0x68 |
| **Features** | Interrupt wake-up support, motion-tracking capable |
| **Application** | MCU wake-up in motion-tracking scenarios, low-power mode efficiency boost |

---

## Real-Time Clock (RTC)

| Specification | Details |
|---|---|
| **RTC Chip** | RX8130CE |
| **I2C Address** | 0x32 |
| **Features** | Timed interrupt wake-up support |
| **Backup Capacitor** | 70000μF / 3.3V, size Φ4.8×1.4mm supercapacitor |

---

## Power Management

| Specification | Details |
|---|---|
| **Charging Management IC** | IP2326 |
| **Power Monitoring IC** | INA226 (bus voltage and current monitoring) |
| **Battery Type** | NP-F550 removable Li-ion (7.4V @ 2000mAh, 14.8 Wh) |
| **Charging Requirement** | Device must be powered on and initialized before charging; charging unavailable when powered off |
| **External Power Input** | 6–24V DC (via power supply connector) |
| **Battery Life** | ~6 hours under standard conditions (50% screen brightness, Wi-Fi always on, background tasks running) |
| **Discharge Range** | Full (8.23V) to shutdown threshold (6.0V) |

**Important Power Notes:**
- Always shut down gracefully before disconnecting power or replacing battery
- Direct power disconnect: wait 5 seconds before powering on again; otherwise IMU may fail to initialize due to abnormal voltage
- Charging only possible when device is on and fully initialized

---

## USB Interfaces

| Interface | Type | Mode | Purpose |
|---|---|---|---|
| **USB-A** | Host | Host Mode | Connect mouse, keyboard, and other USB devices |
| **USB Type-C** | OTG | USB 2.0 OTG | Device/Host mode, firmware upload |

---

## Industrial Interface

| Specification | Details |
|---|---|
| **RS-485 Controller** | SIT3088 |
| **Features** | Switchable 120Ω terminator resistor |
| **Pin Configuration** | RX (G21), TX (G20), DIR (G34) |
| **Application** | Industrial serial communication, long-distance data transmission |

---

## Storage & Expansion

### microSD Card Slot
- **Mode Support:** SPI mode (standard) and SDIO mode (high-speed)
- **SPI Pinout:** MISO (G39), CS (G40), SCK (G41), MOSI (G44), DAT1 (G41), DAT2 (G42)
- **SDIO Pinout:** DAT0 (G39), DAT1 (G40), DAT2 (G41), DAT3 (G42), CLK (G43), CMD (G44)

### M5-Bus Connector
- 30-pin standardized M5Stack bus for modular expansion
- Powers back-mounted M5 Module series products
- Includes GPIO, I2C, UART, SPI, power distribution pins
- HVIN supply pins for high-voltage applications
- See pinmap section for detailed M5-Bus pin assignments

### HY2.0-4P Expansion Port (Type-A)
- **Pinout:** GND (Black), 5V (Red), G53 (Yellow), G54 (White)
- Controlled by PI4IOE5V6408 I2C I/O expanders
- Powered via EXT5V_EN control pin

### GPIO_EXT Expansion Header
- Additional GPIO expansion capability
- Flexible I/O connections for custom applications

### Stamp Pads
- Reserved connector footprints for modular wireless expansion
- Supports: Cat-M, NB-IoT, LoRaWAN, and other communication modules

---

## Control Buttons & Indicators

| Button/Feature | Function |
|---|---|
| **Power Button** | Single press (OFF): power on; Double press (ON): graceful shutdown |
| **Reset Button** | Press and hold (~2 seconds) until internal green LED flashes rapidly: enter download mode for firmware flashing |
| **Internal LED** | Green status indicator (rapid flash = download mode active) |

---

## IO Expanders

| Chip | I2C Address | Function |
|---|---|---|
| **PI4IOE5V6408-1** | 0x43 | Wireless antenna switch, speaker enable, external 5V bus control, LCD/TP/CAM reset, headphone detection |
| **PI4IOE5V6408-2** | 0x44 | Wi-Fi SoC power, USB-A power, device power control, charging management, LED indicators |

---

## Physical Specifications

### Dimensions
| Configuration | Length | Width | Height |
|---|---|---|---|
| **Tab5 (device only)** | 128.0 mm | 80.0 mm | 12.0 mm |
| **Tab5 Kit (with battery)** | 128.0 mm | 80.0 mm | 26.7 mm |

### Weight
| Configuration | Weight |
|---|---|
| **Tab5 (device only)** | 124.5g |
| **Battery only** | 97.9g |
| **Tab5 Kit** | 277.4g (total with battery installed) |

### Package Dimensions
| Configuration | Length | Width | Height |
|---|---|---|---|
| **Tab5 Package** | 148.0 mm | 103.0 mm | 21.0 mm |
| **Tab5 Kit Package** | 191.0 mm | 103.0 mm | 25.0 mm |

### Package Weight
| Configuration | Weight |
|---|---|
| **Tab5 Boxed** | 162.5g |
| **Tab5 Kit Boxed** | 277.4g |

### Mounting
- **Tripod Mount:** 1/4"-20 tripod nut on side (allows direct mounting to standard tripods and brackets)

---

## Operating Conditions

| Parameter | Range |
|---|---|
| **Operating Temperature** | 0°C – 40°C |

---

## Complete Pinmap

### ESP32-P4 Main Processor Pin Assignments

#### Camera (SC2356 via MIPI-CSI)
| ESP32-P4 Pin | Function | SC2356 Signal |
|---|---|---|
| G32 | SCL | CAM_SCL |
| G31 | SDA | CAM_SDA |
| G36 | MCLK | CAM_MCLK |
| CSI_DATAP1 (Dedicated) | CSI Data Lane 1 (Positive) | CAM_D1P |
| CSI_DATAN1 (Dedicated) | CSI Data Lane 1 (Negative) | CAM_D1N |
| CSI_CLKP (Dedicated) | CSI Clock (Positive) | CAM_CSI_CKP |
| CSI_CLKN (Dedicated) | CSI Clock (Negative) | CAM_CSI_CKN |
| CSI_DATAP0 (Dedicated) | CSI Data Lane 0 (Positive) | CSI_DOP |
| CSI_DATAN0 (Dedicated) | CSI Data Lane 0 (Negative) | CSI_DON |

#### Audio Codec (ES8388)
| ESP32-P4 Pin | Function | ES8388 Signal | I2C Address |
|---|---|---|---|
| G30 | MCLK | MCLK | 0x10 |
| G27 | SCLK | SCLK |  |
| G26 | DSDIN | DSDIN |  |
| G29 | LRCK | LRCK |  |
| G32 | SCL | SCL |  |
| G31 | SDA | SDA |  |

#### Audio AEC Front-End (ES7210)
| ESP32-P4 Pin | Function | ES7210 Signal | I2C Address |
|---|---|---|---|
| G30 | MCLK | MCLK | 0x40 |
| G27 | SCLK | SCLK |  |
| G28 | ASDOUT | ASDOUT |  |
| G29 | LRCK | LRCK |  |
| G32 | SCL | SCL |  |
| G31 | SDA | SDA |  |

#### Display (ST7123/ILI9881C via MIPI-DSI)
| ESP32-P4 Pin | Function | Display Signal |
|---|---|---|
| G22 | LED Control | LEDA (brightness) |
| DSI_CLKN (Dedicated) | DSI Clock (Negative) | DSI_CK_N |
| DSI_CLKP (Dedicated) | DSI Clock (Positive) | DSI_CK_P |
| DSI_DATAN1 (Dedicated) | DSI Data Lane 1 (Negative) | DSI_D1_N |
| DSI_DATAP1 (Dedicated) | DSI Data Lane 1 (Positive) | DSI_D1_P |
| DSI_DATAN0 (Dedicated) | DSI Data Lane 0 (Negative) | DSI_D0_N |
| DSI_DATAP0 (Dedicated) | DSI Data Lane 0 (Positive) | DSI_D0_P |

#### Touch Input
| ESP32-P4 Pin | Function | Driver | I2C Address |
|---|---|---|---|
| G31 | SDA | GT911 / ST7123 | 0x14 / 0x55 |
| G32 | SCL | GT911 / ST7123 |  |
| G23 | TP_INT | Touch Interrupt |  |

#### Sensor & RTC I2C Bus (Shared I2C-1)
| ESP32-P4 Pin | Function | Device | I2C Address |
|---|---|---|---|
| G32 | SCL | BMI270, RX8130CE, INA226 | 0x68, 0x32, 0x40 |
| G31 | SDA | BMI270, RX8130CE, INA226 |  |

#### Interrupt Wakeup (PMS150G-U06 Power Management)
| PMS150G Pin | Signal | Connected Device |
|---|---|---|
| PA6/CIN- | Interrupt Input | BMI270 (0x68), RX8130CE (INT) |

#### ESP32-C6 Wireless Module (SDIO Interface)
| ESP32-P4 Pin | Function | ESP32-C6 Pin |
|---|---|---|
| G11 | SDIO2_D0 | SDIO2_D0 |
| G10 | SDIO2_D1 | SDIO2_D1 |
| G9 | SDIO2_D2 | SDIO2_D2 |
| G8 | SDIO2_D3 | SDIO2_D3 |
| G13 | SDIO2_CMD | SDIO2_CMD |
| G12 | SDIO2_CK | SDIO2_CK |
| G15 | RESET | RESET |
| G14 | IO2 | IO2 |

#### microSD Card Slot
| ESP32-P4 Pin | Function | microSD Signal |
|---|---|---|
| G39 | MISO / DAT0 | MISO (SPI) / DAT0 (SDIO) |
| G40 | CS / DAT1 | CS (SPI) / DAT1 (SDIO) |
| G41 | SCK / DAT2 | SCK (SPI) / DAT2 (SDIO) |
| G42 | MOSI / DAT3 | MOSI (SPI) / DAT3 (SDIO) |
| G43 | CLK | CLK (SDIO) |
| G44 | CMD | CMD (SDIO) |

#### RS-485 Interface
| ESP32-P4 Pin | Function | SIT3088 Signal |
|---|---|---|
| G21 | RX | RX |
| G20 | TX | TX |
| G34 | DIR | DIR (Driver Enable) |

#### HY2.0-4P Expansion Port (PORT.A)
| Pin | Function | Signal |
|---|---|---|
| 1 | GND | Black (Ground) |
| 2 | 5V | Red (5V Supply) |
| 3 | GPIO | Yellow (G53) |
| 4 | GPIO | White (G54) |

#### PI4IOE5V6408 I2C I/O Expanders

**PI4IOE5V6408-1 (Address 0x43)** - Antenna, Audio, Display, Touch, Camera Control:
| Expander Pin | Function | Controlled Device |
|---|---|---|
| E1.P0 | RF_PTH_L_INT_H_EXT | Internal/External antenna switching (L=internal, H=external) |
| E1.P1 | SPK_EN | Speaker enable |
| E1.P2 | EXT5V_EN | External 5V bus enable (M5-Bus, GPIO_EXT, HY2.0-4P) |
| E1.P4 | LCD_RST | Display reset |
| E1.P5 | TP_RST | Touch panel reset |
| E1.P6 | CAM_RST | Camera reset |
| E1.P7 | HP_DET | Headphone detection |
| SCL/SDA/RST | I2C & Reset | To ESP32-P4 G32/G31 and CHIP_PU |

**PI4IOE5V6408-2 (Address 0x44)** - Power & Charging Control:
| Expander Pin | Function | Controlled Device |
|---|---|---|
| E2.P0 | WLAN_PWR_EN | ESP32-C6 (Wi-Fi SoC) power enable |
| E2.P3 | USB5V_EN | USB-A power enable |
| E2.P4 | DEVICE_PWR | Device power control |
| E2.P5 | PWROFF_PLUSE | Power off pulse control |
| E2.P6 | nCHG_QC_EN | Charging QC enable (active low) |
| E2.P7 | CHG_EN | Charging enable |
| SCL/SDA/RST | I2C & Reset | To ESP32-P4 G32/G31 and CHIP_PU |

### M5-Bus Connector (30-pin Header)
| Pin # | Left Side | Right Side | Pin # |
|---|---|---|---|
| 1 | GND | G16 (GPIO) | 2 |
| 3 | GND | G17 (GPIO/PB_IN) | 4 |
| 5 | GND | RST (Reset) | 6 |
| 7 | MOSI (G18) | G45 (GPIO) | 8 |
| 9 | MISO (G19) | G52 (GPIO/PB_OUT) | 10 |
| 11 | SCK (G5) | 3V3 | 12 |
| 13 | RXD0 (G38) | G37 (TXD0) | 14 |
| 15 | PC_RX (G7) | G6 (PC_TX) | 16 |
| 17 | SDA (G31) | G32 (SCL) | 18 |
| 19 | G3 (GPIO) | G4 (GPIO) | 20 |
| 21 | G2 (GPIO) | G48 (GPIO) | 22 |
| 23 | G47 (GPIO) | G35 (GPIO) | 24 |
| 25 | HVIN | G51 (GPIO) | 26 |
| 27 | HVIN | 5V | 28 |
| 29 | HVIN | BAT (Battery) | 30 |

---

## Package Contents

### Tab5 (SKU: C145)
- 1× Tab5 device
- 1× 1.25-6P single-ended terminal cable

### Tab5 Kit (SKU: K145)
- 1× Tab5 device with battery installed
- 1× 1.25-6P single-ended terminal cable
- 1× NP-F550 2000mAh removable Li-ion battery (spare)

---

## Battery Specifications (NP-F550)

| Parameter | Value |
|---|---|
| **Type** | Removable Li-ion |
| **Voltage** | 7.4V nominal |
| **Capacity** | 2000mAh |
| **Energy** | 14.8 Wh |
| **Weight** | 97.9g |
| **Installation** | Press red locking button, align battery contacts with "BATTERY" slot, slide downward into rails, release button to lock |
| **Removal** | Press red locking button, slide battery upward out of rails |

---

## Supported Development Platforms

| Platform | Notes |
|---|---|
| **Arduino IDE** | Requires M5Unified and M5GFX libraries |
| **UiFlow2** | Visual block-based programming platform |
| **ESP-IDF** | Low-level development framework |
| **PlatformIO** | IDE with custom ESP32-P4 board configuration |

### PlatformIO Configuration Example
```ini
[env:esp32p4_pioarduino]
platform = https://github.com/pioarduino/platform-espressif32.git#54.03.21
upload_speed = 1500000
monitor_speed = 115200
build_type = debug
framework = arduino
board = esp32-p4-evboard
board_build.mcu = esp32p4
board_build.flash_mode = qio
build_flags =
    -DBOARD_HAS_PSRAM
    -DCORE_DEBUG_LEVEL=5
    -DARDUINO_USB_CDC_ON_BOOT=1
    -DARDUINO_USB_MODE=1
lib_deps =
    https://github.com/M5Stack/M5Unified.git
    https://github.com/M5Stack/M5GFX.git
```

---

## Applications

- Smart-home control and automation
- Remote monitoring systems
- IoT device development and prototyping
- Industrial automation and control
- Educational platforms for embedded systems
- Edge AI (facial recognition, object tracking)
- Voice command interfaces
- Portable terminal for field operations

---

## Important Notes

### Power Management
- **Before disconnecting power or replacing battery:** Always perform a graceful shutdown
- **Direct power disconnect:** Wait 5 seconds before powering on again; otherwise IMU sensor (BMI270) may fail to initialize due to abnormal voltage
- **Charging requirement:** Device must be powered on and fully initialized before charging begins; charging is disabled when powered off

### Display Driver Version
- **Before October 14, 2025:** Separate ILI9881C display driver and GT911 touch controller
- **After October 14, 2025:** Integrated ST7123 display-touch driver
- **Firmware compatibility:** Latest M5Unified and M5GFX support both driver versions; legacy firmware may require recompilation with latest libraries
- **Device identification:** Check sticker on back of device to confirm driver model

### Wireless Module Power
- ESP32-C6 Wi-Fi SoC power is controlled via PI4IOE5V6408-2 (WLAN_PWR_EN)
- Can be disabled to save power in low-power operation modes

---

## Version History

| Release Date | Change | Notes |
|---|---|---|
| **October 14, 2025** | Display driver optimization | Replaced separate ILI9881C (display) + GT911 (touch) with integrated ST7123 driver |
| **May 9, 2025** | First product release | Initial Tab5 launch |

---

*Document compiled from M5Stack Tab5 official specifications. Information current as of January 28, 2026.*
