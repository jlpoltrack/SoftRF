/*
 * Platform_RP2XXX.h
 * Copyright (C) 2022-2026 Linar Yusupov
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#if defined(ARDUINO_ARCH_RP2040) || defined(ARDUINO_ARCH_RP2350)

#ifndef PLATFORM_RP2XXX_H
#define PLATFORM_RP2XXX_H

#include <avr/dtostrf.h>

/* Maximum of tracked flying objects is now SoC-specific constant */
#define MAX_TRACKING_OBJECTS    8

#define DEFAULT_SOFTRF_MODEL    SOFTRF_MODEL_LEGO

#define isValidFix()            isValidGNSSFix()

#define uni_begin()             strip.begin()
#define uni_show()              strip.show()
#define uni_setPixelColor(i, c) strip.setPixelColor(i, c)
#define uni_numPixels()         strip.numPixels()
#define uni_Color(r,g,b)        strip.Color(r,g,b)
#define color_t                 uint32_t

#define EEPROM_commit()         EEPROM.commit()

#define LED_STATE_ON            HIGH  // State when LED is litted

#define SerialOutput            Serial2

#if !defined(ARDUINO_ARCH_MBED)
#define USBSerial               Serial
#else
#define USBSerial               SerialUSB
#endif /* ARDUINO_ARCH_MBED */

#define Serial_GNSS_In          Serial1
#define Serial_GNSS_Out         Serial_GNSS_In
#define UATSerial               Serial

enum rst_reason {
  REASON_DEFAULT_RST      = 0,  /* normal startup by power on */
  REASON_WDT_RST          = 1,  /* hardware watch dog reset */
  REASON_EXCEPTION_RST    = 2,  /* exception reset, GPIO status won't change */
  REASON_SOFT_WDT_RST     = 3,  /* software watch dog reset, GPIO status won't change */
  REASON_SOFT_RESTART     = 4,  /* software restart ,system_restart , GPIO status won't change */
  REASON_DEEP_SLEEP_AWAKE = 5,  /* wake up from deep-sleep */
  REASON_EXT_SYS_RST      = 6   /* external system reset */
};

enum RP2xxx_board_id {
  RP2040_RAK11300,
  RP2040_RESERVED1,
  RP2040_RPIPICO,
  RP2040_RPIPICO_W,
  RP2040_WEACT,
  RP2350_RPIPICO_2,
  RP2350_RPIPICO_2W,
  RP2040_PICO_LR2021,
};

struct rst_info {
  uint32_t reason;
  uint32_t exccause;
  uint32_t epc1;
  uint32_t epc2;
  uint32_t epc3;
  uint32_t excvaddr;
  uint32_t depc;
};

#if defined(PICO_LR2021_ADSB)

/* Override SerialOutput so MAVLink emission goes out UART0 (GP0/GP1).        */
/* Default for RP2XXX maps SerialOutput=Serial2; this board has no UART1     */
/* peripheral wired, and the user wants UART0 free for ArduPilot MAVLink.    */
#undef  SerialOutput
#define SerialOutput            Serial1

/* Console RX/TX — these are repurposed for MAVLink (SerialOutput pins).     */
#define SOC_GPIO_PIN_CONS_RX    ( 1u) // GP1, UART0 RX
#define SOC_GPIO_PIN_CONS_TX    ( 0u) // GP0, UART0 TX

/*
 * MAVLink, not NMEA-over-Bluetooth, is what leaves this UART. Use the rate
 * MAVLink_setup() applies on every other platform, which is also ArduPilot's
 * SERIALn_BAUD default. STD_OUT_BR (38400) would silently mismatch.
 */
#if defined(SERIAL_OUT_BR)
#undef  SERIAL_OUT_BR
#endif
#define SERIAL_OUT_BR           57600

/* No on-board GNSS receiver. */
#define SOC_GPIO_PIN_GNSS_RX    SOC_UNUSED_PIN
#define SOC_GPIO_PIN_GNSS_TX    SOC_UNUSED_PIN
#define SOC_GPIO_PIN_GNSS_PPS   SOC_UNUSED_PIN
#define SOC_GPIO_PIN_GNSS_RST   SOC_UNUSED_PIN
#define SOC_GPIO_PIN_GNSS_SBY   SOC_UNUSED_PIN
#define SOC_GPIO_PIN_GNSS_FON   SOC_UNUSED_PIN

/* Built-in SPI0 (unused) */
#define SOC_GPIO_PIN_MOSI0      (19u)
#define SOC_GPIO_PIN_MISO0      (16u)
#define SOC_GPIO_PIN_SCK0       (18u)
#define SOC_GPIO_PIN_SS0        (17u)

/* SPI1 → LR2021 (16 MHz max) */
#define SOC_GPIO_PIN_MOSI       (11u)
#define SOC_GPIO_PIN_MISO       (12u)
#define SOC_GPIO_PIN_SCK        (10u)
#define SOC_GPIO_PIN_SS         (13u)
#define RadioSPI                SPI1

/* NRF905 — unused on this board */
#define SOC_GPIO_PIN_TXE        SOC_UNUSED_PIN
#define SOC_GPIO_PIN_CE         SOC_UNUSED_PIN
#define SOC_GPIO_PIN_PWR        SOC_UNUSED_PIN

/* LR2021 control */
#define SOC_GPIO_PIN_RST        ( 7u) // LR2021 NRESET
#define SOC_GPIO_PIN_BUSY       ( 8u) // LR2021 BUSY
#define SOC_GPIO_PIN_DIO1       ( 6u) // LR2021 DIO9 (board-labeled "IRQ"); used as IRQ

/* RF antenna switching is driven internally by the LR2021 chip via its own  */
/* DIO5..DIO8. Host MCU does not drive any antenna-switch line.              */
#define SOC_GPIO_PIN_ANT_RXTX   SOC_UNUSED_PIN

/* I2C0 / I2C1 — unused */
#define SOC_GPIO_PIN_SDA        SOC_UNUSED_PIN
#define SOC_GPIO_PIN_SCL        SOC_UNUSED_PIN
#define SOC_GPIO_PIN_SDA1       SOC_UNUSED_PIN
#define SOC_GPIO_PIN_SCL1       SOC_UNUSED_PIN

/*
 * Host is a stock Raspberry Pi Pico (non-W), so the fixed-function pins are the
 * Pico's own: GP25 on-board LED, GP23 SMPS power-save control, GP24 VBUS sense,
 * GP29 VSYS/3 sense. GP23 is NOT an LED -- driving it only switches the
 * RT6150 between PFM and PWM. Kept literal rather than PIN_LED so the block is
 * valid under both the rpipico and generic FQBNs.
 *
 * SOC_GPIO_PIN_LED is the NeoPixel ring data line, not the status LED; there is
 * no ring on this board.
 */
#define SOC_GPIO_PIN_LED        SOC_UNUSED_PIN
#define SOC_GPIO_PIN_STATUS     (25u) // on-board LED, active HIGH
#define SOC_GPIO_PIN_BUTTON     SOC_UNUSED_PIN // BOOTSEL, via USE_BOOTSEL_BUTTON
#define SOC_GPIO_PIN_BUZZER     SOC_UNUSED_PIN
#define SOC_GPIO_PIN_BATTERY    SOC_UNUSED_PIN // set to VSYS to monitor the rail
#define SOC_GPIO_PIN_VBUS       (24u)
#define SOC_GPIO_PIN_VSYS       (29u)
#define SOC_GPIO_PIN_PS         (23u)
#define SOC_GPIO_PIN_CYW43_EN   SOC_UNUSED_PIN // non-W: no CYW43

#define SOC_GPIO_RADIO_LED_RX   SOC_UNUSED_PIN
#define SOC_GPIO_RADIO_LED_TX   SOC_UNUSED_PIN

#define SOC_GPIO_PIN_USBH_DP    SOC_UNUSED_PIN
#define SOC_GPIO_PIN_USBH_DN    SOC_UNUSED_PIN

#define SOC_ADC_VOLTAGE_DIV     (1.0)

#elif defined(ARDUINO_GENERIC_RP2040)

/* Console, I/O SLOT only */
#define SOC_GPIO_PIN_CONS_RX  (5u)
#define SOC_GPIO_PIN_CONS_TX  (4u)

/* RAK1910 (Ublox-7), SENSOR SLOT A only */
#define SOC_GPIO_PIN_GNSS_RX  (1u)  // J10
#define SOC_GPIO_PIN_GNSS_TX  (0u)  // J10
#define SOC_GPIO_PIN_GNSS_PPS (6u)  // IO1, J11
#define SOC_GPIO_PIN_GNSS_RST (22u) // IO2, J11

#define SOC_GPIO_PIN_STATUS   SOC_UNUSED_PIN // LED

/* RAK18001, SENSOR SLOT C */
#define SOC_GPIO_PIN_BUZZER   (7u)  // IO3

/* SPI0 */
#define SOC_GPIO_PIN_MOSI0    (19u)
#define SOC_GPIO_PIN_MISO0    (16u)
#define SOC_GPIO_PIN_SCK0     (18u)
#define SOC_GPIO_PIN_SS0      (17u)

/* RAK11310, SPI1 */
#define SOC_GPIO_PIN_MOSI     (11u)
#define SOC_GPIO_PIN_MISO     (12u)
#define SOC_GPIO_PIN_SCK      (10u)
#define SOC_GPIO_PIN_SS       (13u)

#define RadioSPI              SPI1

/* NRF905 */
#define SOC_GPIO_PIN_TXE      SOC_UNUSED_PIN
#define SOC_GPIO_PIN_CE       SOC_UNUSED_PIN
#define SOC_GPIO_PIN_PWR      SOC_UNUSED_PIN

/* RAK11310, SX1262 */
#define SOC_GPIO_PIN_RST      (14u)
#define SOC_GPIO_PIN_BUSY     (15u)
#define SOC_GPIO_PIN_DIO1     (29u)

/* RAK11310, RF antenna switch */
#define SOC_GPIO_PIN_ANT_RXTX (25u) // RXEN

/* I2C0 */
#define SOC_GPIO_PIN_SDA0     (20u)
#define SOC_GPIO_PIN_SCL0     (21u)

/* RAK11310, I2C1, J12 */
#define SOC_GPIO_PIN_SDA      (2u)
#define SOC_GPIO_PIN_SCL      (3u)
#define Wire                  Wire1

#define SOC_GPIO_PIN_LED      SOC_UNUSED_PIN
#define SOC_GPIO_PIN_BUTTON   SOC_UNUSED_PIN

/* RAK11310 */
#define SOC_GPIO_PIN_BATTERY  (26u) // ADC0
#define SOC_GPIO_RADIO_LED_RX SOC_UNUSED_PIN // LED1 (23u)
#define SOC_GPIO_RADIO_LED_TX (24u) // LED2

#define SOC_GPIO_PIN_IO1      (6u)
#define SOC_GPIO_PIN_IO2      (22u) // 3V3_S PWR_EN, GNSS NRST
#define SOC_GPIO_PIN_IO3      (7u)
#define SOC_GPIO_PIN_IO4      (28u)
#define SOC_GPIO_PIN_IO5      (9u)
#define SOC_GPIO_PIN_IO6      (8u)
#define SOC_GPIO_PIN_A0       (26u) // ADC_VBAT
#define SOC_GPIO_PIN_A1       (27u)

#define SOC_GPIO_PIN_USBH_DP  (8u)  // Pin used as D+ for host, D- = D+ + 1
#define SOC_GPIO_PIN_USBH_DN  (9u)

#define SOC_ADC_VOLTAGE_DIV   (5.0 / 3)

#elif defined(ARDUINO_RASPBERRY_PI_PICO)    || \
      defined(ARDUINO_RASPBERRY_PI_PICO_2)  || \
      defined(ARDUINO_RASPBERRY_PI_PICO_W)  || \
      defined(ARDUINO_RASPBERRY_PI_PICO_2W) || \
      defined(ARDUINO_NANO_RP2040_CONNECT)

/* Console I/O */
#define SOC_GPIO_PIN_CONS_RX  ( 5u) // "RX"
#define SOC_GPIO_PIN_CONS_TX  ( 4u) // "TX"

/* Waveshare Pico-GPS-L76B (MTK) */
#define SOC_GPIO_PIN_GNSS_RX  ( 1u) // D3, (5u) , H2
#define SOC_GPIO_PIN_GNSS_TX  ( 0u) // D9, (4u) , H1
#define SOC_GPIO_PIN_GNSS_PPS (16u) // R20      (NC by default)
#define SOC_GPIO_PIN_GNSS_RST SOC_UNUSED_PIN // NA
#define SOC_GPIO_PIN_GNSS_SBY (17u) // STANDBY  (NC by default)
#define SOC_GPIO_PIN_GNSS_FON (14u) // FORCE_ON (NC by default)

/* SPI0 */
#define SOC_GPIO_PIN_MOSI0    (19u)
#define SOC_GPIO_PIN_MISO0    (16u)
#define SOC_GPIO_PIN_SCK0     (18u)
#define SOC_GPIO_PIN_SS0      (17u)

/* Waveshare Pico-LoRa-SX1262-868M, SPI1 */
#define SOC_GPIO_PIN_MOSI     (11u) // D7
#define SOC_GPIO_PIN_MISO     (12u) // D6
#define SOC_GPIO_PIN_SCK      (10u) // D5
#define SOC_GPIO_PIN_SS       ( 3u) // D8

#define RadioSPI              SPI1

/* NRF905 */
#define SOC_GPIO_PIN_TXE      SOC_UNUSED_PIN
#define SOC_GPIO_PIN_CE       SOC_UNUSED_PIN
#define SOC_GPIO_PIN_PWR      SOC_UNUSED_PIN

/* Waveshare Pico-LoRa-SX1262-868M, SX1262 */
#define SOC_GPIO_PIN_RST      (15u) // D2
#define SOC_GPIO_PIN_BUSY     ( 2u) // D0
#define SOC_GPIO_PIN_DIO1     (20u) // D4, may cause conflict with SDA

/* Waveshare Pico-LoRa-SX1262-868M, RF antenna switch */
#define SOC_GPIO_PIN_ANT_RXTX (22u) // RXEN

/* Waveshare Pico-Environment-Sensor, BME280, I2C0 */
#define SOC_GPIO_PIN_SDA      (20u)
#define SOC_GPIO_PIN_SCL      (21u)

/* I2C1 */
#define SOC_GPIO_PIN_SDA1     (26u)
#define SOC_GPIO_PIN_SCL1     (27u)

#define SOC_GPIO_PIN_LED      (13u) // D1
#define SOC_GPIO_PIN_VBUS     (24u) // Pico
#define SOC_GPIO_PIN_VSYS     (29u) // Pico
#define SOC_GPIO_PIN_PS       (23u) // Pico
#define SOC_GPIO_PIN_BUTTON   (23u) // WeAct
#define SOC_GPIO_PIN_CYW43_EN (25u) // Pico W

#define SOC_GPIO_PIN_STATUS   PIN_LED // Pico/WeAct - 25, W - 64 (CYW43 GPIO 0)
#define SOC_GPIO_PIN_BATTERY  SOC_GPIO_PIN_VSYS // A0, (8u)
#define SOC_GPIO_PIN_BUZZER   ( 9u) // D10

/* Waveshare Pico-LoRa-SX1262-868M */
#define SOC_GPIO_RADIO_LED_RX SOC_UNUSED_PIN
#define SOC_GPIO_RADIO_LED_TX SOC_UNUSED_PIN

#define SOC_GPIO_PIN_USBH_DP  ( 6u) // Pin used as D+ for host, D- = D+ + 1
#define SOC_GPIO_PIN_USBH_DN  ( 7u)

#define SOC_ADC_VOLTAGE_DIV   (3.0) // 20K + 10K voltage divider of VSYS

#else
#error "This RP2040 build variant is not supported!"
#endif

#if defined(ARDUINO_RASPBERRY_PI_PICO_W) || \
    defined(ARDUINO_RASPBERRY_PI_PICO_2W)
#define EXCLUDE_OTA
#define USE_WIFI_NINA         false
#define USE_WIFI_CUSTOM       true
#include <ESP8266WiFi.h>
#define Serial_setDebugOutput(x) ({})
#define WIFI_STA_TIMEOUT      20000
#define NMEA_TCP_SERVICE
/* Experimental */
//#define ENABLE_BT_VOICE
#else
#if defined(ESPHOSTSPI)
#include <ESP8266WiFi.h>
#define Serial_setDebugOutput(x) ({})
#define USE_ARDUINO_WIFI
#define EXCLUDE_OTA
#else
#define EXCLUDE_WIFI
#endif /* ESPHOSTSPI */
//#define EXCLUDE_OTA
//#define USE_ARDUINO_WIFI
//#define USE_WIFI_NINA         false
//#define USE_WIFI_CUSTOM       true
//#include <WiFiNINA.h>
//#define Serial_setDebugOutput(x) ({})
//#define WIFI_STA_TIMEOUT      20000

#if !defined(ARDUINO_ARCH_MBED)
#define EXCLUDE_BLUETOOTH
#else
#if defined(ARDUINO_NANO_RP2040_CONNECT)
//#define EXCLUDE_BLUETOOTH
#define USE_ARDUINOBLE
#else
#define EXCLUDE_BLUETOOTH
#endif /* ARDUINO_NANO_RP2040_CONNECT */
#endif /* ARDUINO_ARCH_MBED */
#endif /* ARDUINO_RASPBERRY_PI_PICO_W or 2W */

#define EXCLUDE_CC13XX
#define EXCLUDE_TEST_MODE
#define EXCLUDE_WATCHOUT_MODE
//#define EXCLUDE_TRAFFIC_FILTER_EXTENSION
#define EXCLUDE_LK8EX1

//#define EXCLUDE_GNSS_UBLOX    /* Neo-6/7/8 */
#define EXCLUDE_GNSS_SONY
//#define EXCLUDE_GNSS_MTK
#define EXCLUDE_GNSS_GOKE
#define EXCLUDE_GNSS_AT65
#define EXCLUDE_GNSS_UC65
#define EXCLUDE_GNSS_AG33
//#define EXCLUDE_LOG_GNSS_VERSION

/* Component                         Cost */
/* -------------------------------------- */
#define USE_NMEA_CFG             //  +    kb
#define EXCLUDE_BMP180           //  -    kb
//#define EXCLUDE_BMP280         //  -    kb
#define EXCLUDE_BME680           //  -    kb
#define EXCLUDE_BME280AUX        //  -    kb
#define EXCLUDE_MPL3115A2        //  -    kb
#define EXCLUDE_SPA06            //  -    kb
#define EXCLUDE_NRF905           //  -    kb
#define EXCLUDE_UATM             //  -    kb
#if !defined(PICO_LR2021_ADSB)
#define EXCLUDE_MAVLINK          //  -    kb
#endif /* PICO_LR2021_ADSB */
//#define EXCLUDE_EGM96          //  -    kb
//#define EXCLUDE_SOUND

#define USE_OLED                 //       kb
#define EXCLUDE_OLED_049
//#define EXCLUDE_OLED_BARO_PAGE

/* Experimental */
#define ENABLE_ADSL
#define ENABLE_PROL
#if defined(USE_TINYUSB)
//#define USE_USB_HOST
#endif /* USE_TINYUSB */

#define EXCLUDE_ETHERNET

#if !defined(ARDUINO_ARCH_MBED)
#define USE_BOOTSEL_BUTTON
#if defined(ARDUINO_GENERIC_RP2040)
#define EXCLUDE_LED_RING
#endif /* ARDUINO_GENERIC_RP2040 */
#else
#define EXCLUDE_EEPROM
#define EXCLUDE_LED_RING

#if defined(USE_ARDUINOBLE)
#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)
#define ARDUINO_CORE_VERSION  STR(CORE_MAJOR) "." STR(CORE_MINOR) "." STR(CORE_PATCH)
#endif /* USE_ARDUINOBLE */
#endif /* ARDUINO_ARCH_MBED */

#define USE_BASICMAC
/*
 * The LR2021 is only reachable through RadioLib. Key the driver selection off
 * the board flag as well as the FQBN: PICO_LR2021_ADSB is an opt-in variant
 * layered onto a stock Pico FQBN, and picking one where USE_RADIOLIB stays
 * undefined yields a firmware that compiles and boots but has no radio driver
 * at all ("None of supported RFICs is detected!").
 */
#if defined(ARDUINO_GENERIC_RP2040) || defined(PICO_LR2021_ADSB)
#define USE_RADIOLIB
//#define USE_RADIOHEAD
//#define EXCLUDE_LR11XX
#if defined(USE_RADIOLIB)
#include <BuildOpt.h>
#if RADIOLIB_VERSION_MAJOR <= 7 && RADIOLIB_VERSION_MINOR < 6
#define EXCLUDE_LR20XX
#endif /* RADIOLIB_VERSION */
#endif /* USE_RADIOLIB */
#if defined(PICO_LR2021_ADSB)
/*
 * No SX1276 on this board. BasicMAC probes it first, over the same SPI1 pins
 * the LR2021 sits on, so leaving it in risks a false positive ahead of
 * lr2021_probe(). Matches the ESP32-S3 LR2021 platform config.
 */
#define EXCLUDE_SX1276
#endif /* PICO_LR2021_ADSB */
#define EXCLUDE_CC1101
#define EXCLUDE_SI443X
#define EXCLUDE_SI446X
#define EXCLUDE_SX1231
#define EXCLUDE_SX1280
#endif /* ARDUINO_GENERIC_RP2040 || PICO_LR2021_ADSB */

#define USE_TIME_SLOTS

#define USE_OGN_ENCRYPTION

extern const char *RP2xxx_Device_Manufacturer, *RP2xxx_Device_Model;

#if !defined(EXCLUDE_LED_RING)
#include <Adafruit_NeoPixel.h>

extern Adafruit_NeoPixel strip;
#endif /* EXCLUDE_LED_RING */

#if defined(USE_OLED)
#if defined(ARDUINO_GENERIC_RP2040)
#define U8X8_OLED_I2C_BUS_TYPE  U8X8_SSD1306_128X64_NONAME_2ND_HW_I2C
#else
#define U8X8_OLED_I2C_BUS_TYPE  U8X8_SSD1306_128X64_NONAME_HW_I2C
#endif /* ARDUINO_GENERIC_RP2040 */
#endif /* USE_OLED */

#endif /* PLATFORM_RP2XXX_H */
#endif /* ARDUINO_ARCH_RP2XXX */
