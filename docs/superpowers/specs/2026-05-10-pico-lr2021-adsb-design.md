# Pi Pico (RP2040) + LR2021 ADS-B Sniffer — Design

**Status:** Draft for review
**Date:** 2026-05-10
**Author:** JLP

## Summary

Add SoftRF firmware support for a custom RP2040 + Semtech LR2021 board whose
primary use case is sniffing 1090 MHz ADS-B (Mode-S DF17) and forwarding
decoded traffic as MAVLink to an ArduPilot flight controller over a UART. USB
CDC stays free as the debug console.

The work is scoped as a board variant. The platform (`RP2XXX`), the LR2021
radio driver (`radiolib.cpp`), the 1090 ES protocol decoder (`ES1090.cpp`),
and the MAVLink output path are all already present in the codebase. This
spec is glue and configuration only.

## Goals

- Boot the new board, probe the LR2021, configure the chip-internal RF
  antenna switch correctly for the user's hardware.
- Receive 1090 ES (DF17) packets via the existing decoder.
- Emit MAVLink traffic frames on UART0 (GP0/GP1) at a configurable baud rate
  to ArduPilot.
- Keep USB CDC (`Serial`) as the debug console.
- Leave every existing RP2040/RP2350 build target unaffected. Selection is
  opt-in via a build flag.

## Non-goals

- TX support on this board (configurable later, but the documented role is
  receive-only).
- On-board GNSS support (no GNSS hardware on this board; position comes from
  ArduPilot via MAVLink if needed).
- Battery monitoring, OLED, button, buzzer, or other peripheral integrations.
- Runtime auto-detection between LR2021 and SX1262 — selection is
  compile-time.

## Hardware

Custom PCB. RP2040 host, Semtech LR2021 radio.

| Signal | LR2021 / peripheral side | RP2040 GPIO |
|---|---|---|
| SPI MOSI | LR2021 SPI MOSI | GP11 |
| SPI MISO | LR2021 SPI MISO | GP12 |
| SPI SCK  | LR2021 SPI SCK  | GP10 |
| SPI CS   | LR2021 SPI NSS  | GP13 |
| LR2021 RST | LR2021 NRESET | GP7  |
| LR2021 BUSY | LR2021 BUSY  | GP8  |
| LR2021 IRQ  | LR2021 DIO9 (board-labeled "IRQ") | GP6 |
| MAVLink TX  | UART0 TX | GP0 |
| MAVLink RX  | UART0 RX | GP1 |
| LED         | onboard | GP23 |
| Console     | USB CDC | n/a |

**SPI bus:** SPI1, max 16 MHz per LR2021 datasheet.

**TCXO:** 3.3 V, supplied by LR2021 via TCXO control.

**LR2021 internal RF antenna switch:** the LR2021 drives DIO5–DIO8 itself
based on its current operating state (RX_LF / TX_LF / RX_HF / TX_HF). The
host MCU does not drive these pins. Configuration:

| LR2021 DIO | Asserted when chip is in |
|---|---|
| DIO5 | RX_HF |
| DIO6 | TX_LF |
| DIO7 | RX_HF or TX_HF |
| DIO8 | TX_HF |

ADS-B 1090 MHz uses the LR2021's LF RX path in this codebase (radiolib's
`setRxPath` is gated on `frequency >= 1.5 GHz`). So at 1090 MHz none of
DIO5–DIO8 is asserted, and the antenna lands on the LF input — matching the
existing `SOFTRF_MODEL_PRIME_MK4` behavior.

**No GNSS, button, buzzer, OLED, or battery sense on this board.**

## Approach

A new `SOFTRF_MODEL_*` value plus a build-flag-gated pin block in
`RP2XXX.h`, an additional RF-switch entry in `radiolib.cpp`, and a small
amount of platform glue in `RP2XXX.cpp`. This matches every existing LR2021
board's integration pattern in the codebase (`SOFTRF_MODEL_ACADEMY`,
`SOFTRF_MODEL_CARD`, `SOFTRF_MODEL_PRIME_MK4`, etc.).

Alternatives considered and rejected:

- **Reuse `Semtech_LR2021EVK1XCS1.h` iomap** — the file is named for the
  Semtech EVK and uses `EVK_*` macros; this is a custom PCB with a
  different pinout. Mixing namespaces is confusing.
- **Runtime probe between LR2021 and SX1262** — different MOSI/MISO pin
  assignments make probing unsafe; SoftRF doesn't do this elsewhere on
  RP2XXX.

## Components

### Build flag

`-DPICO_LR2021_ADSB`. Adding this to the Arduino-Pico build selects the new
pin block in `RP2XXX.h`, the new model in `RP2XXX.cpp`, and the new switch
table / IRQ DIO in `radiolib.cpp`. Without the flag, today's
`ARDUINO_RASPBERRY_PI_PICO` build (Waveshare Pico-LoRa-SX1262 default) is
unchanged. The flag will be defined in `build_opt.h`, with a clear
comment, behind a normally-disabled `#if 0` block so users can enable it
deliberately. (Alternatively: pass via `--build-property
build.extra_flags=-DPICO_LR2021_ADSB`.)

### `SoftRF.h` — new SOFTRF_MODEL value

Append `SOFTRF_MODEL_ADSB_PICO` to the `SOFTRF_MODEL_*` enum.

### `RP2XXX.h` — pin map

Add `RP2040_PICO_LR2021` to the `RP2xxx_board_id` enum.

Add a new `#elif defined(PICO_LR2021_ADSB)` block (placed before the
existing `ARDUINO_RASPBERRY_PI_PICO` block so the flag wins) defining all
`SOC_GPIO_PIN_*` macros per the table above. `Serial_GNSS_In` and console
RX/TX are mapped to `SOC_UNUSED_PIN`. `RadioSPI = SPI1`.

### `RP2XXX.cpp` — platform setup

When `PICO_LR2021_ADSB` is defined:

- `hw_info.model = SOFTRF_MODEL_ADSB_PICO` (override the default
  `SOFTRF_MODEL_STANDALONE`).
- `RP2xxx_board = RP2040_PICO_LR2021`.
- `SerialOutput` is bound to `Serial1` (UART0) with `setRX(GP1) / setTX(GP0)`
  for MAVLink. The MAVLink baud rate uses `SERIAL_OUT_BR` (existing
  setting) by default.
- `Serial_GNSS_In.begin(...)` is **not** called — no on-board GNSS. Any
  callers that touch `Serial_GNSS_In` must already tolerate it being
  uninitialized; verify during implementation.
- `Serial` (USB CDC) is the debug console — already the case on Pico
  builds.
- `lr2021_ops` must be registered in the RP2XXX chip-ops table so
  `RF_setup()` selects it. (Verify whether the LR2021 ops are already
  visible to RP2XXX builds; if not, expose them under the build flag.)

### `radiolib.cpp` — LR2021 RF switch + IRQ DIO

Add a new switch table:

```c
static const uint32_t rfswitch_dio_pins_pico_lr2021[] = {
    RADIOLIB_LR2021_DIO5, RADIOLIB_LR2021_DIO6,
    RADIOLIB_LR2021_DIO7, RADIOLIB_LR2021_DIO8,
    RADIOLIB_NC
};

static const Module::RfSwitchMode_t rfswitch_table_pico_lr2021[] = {
    // mode                 DIO5  DIO6  DIO7  DIO8
    { LR2021::MODE_STBY,  { LOW,  LOW,  LOW,  LOW  } },
    { LR2021::MODE_RX,    { LOW,  LOW,  LOW,  LOW  } },  // RX_LF, incl. 1090 MHz
    { LR2021::MODE_TX,    { LOW,  HIGH, LOW,  LOW  } },  // TX_LF (DIO6)
    { LR2021::MODE_RX_HF, { HIGH, LOW,  HIGH, LOW  } },  // RX_HF (DIO5+DIO7)
    { LR2021::MODE_TX_HF, { LOW,  LOW,  HIGH, HIGH } },  // TX_HF (DIO7+DIO8)
    END_OF_MODE_TABLE,
};
```

Add a `case SOFTRF_MODEL_ADSB_PICO` to:

- the IRQ DIO selection switch — `radio_g4->irqDioNum = 9; Vtcxo = 3.3;`
- the `setRfSwitchTable` selection switch — install
  `rfswitch_dio_pins_pico_lr2021` / `rfswitch_table_pico_lr2021`.

### Default settings

The default `settings->rf_protocol` for this model should be
`RF_PROTOCOL_ADSB_1090`, and the default NMEA/data output should be MAVLink
on UART0. EEPROM defaulting in `EEPROM.cpp` is keyed on `hw_info.model`,
so this should plug in cleanly.

## Data flow

```
1090 MHz ADS-B ─► LR2021 (HF antenna port)
                    │ chip downconverts via LF path (per radiolib)
                    │ DF17 packets in radio FIFO
                    ▼ via SPI1 + IRQ on DIO9 → GP6
                  radiolib.cpp lr20xx_receive()
                    │
                    ▼
                  ES1090.cpp → mode_s decoder
                    │
                    ▼
                  TrafficHelper (deduplicate, range filter)
                    │
                    ▼
                  MAVLinkShareTraffic() → write_mavlink()
                    │
                    ▼
                  SerialOutput == Serial1 (UART0, GP0)
                    │
                    ▼
                  ArduPilot ADSB_VEHICLE messages
```

USB CDC `Serial` remains free for the SoftRF debug log.

## Build & test plan

1. **Build smoke test (no flag):** existing `arduino-cli compile -b
   rp2040:rp2040:rpipico` builds successfully and is byte-identical to
   pre-change.
2. **Build smoke test (flag set):** same target with
   `--build-property build.extra_flags=-DPICO_LR2021_ADSB` builds
   successfully.
3. **Boot/probe:** USB CDC banner appears; `lr2021_probe()` returns true;
   firmware version log line is printed.
4. **Radio init:** 1090 ES protocol active, no `setRfSwitchTable` error,
   no `setRxPath` error.
5. **RX validation:** with antenna connected, USB debug shows DF17 frames
   received; `lr20xx_rx_monitor_marker` ticks normally; receive callback
   count > 0.
6. **MAVLink output:** logic analyzer on GP0 shows MAVLink frames at the
   configured baud; `ADSB_VEHICLE` message IDs visible.
7. **End-to-end:** connect GP0/GP1 to ArduPilot serial; Mission Planner /
   QGroundControl shows ADS-B traffic from received aircraft.

## Open items for implementation

- Verify whether `lr2021_ops` is already exposed to RP2XXX builds; if not,
  add the `#include` / chip-table entry under the build flag.
- Confirm `Serial_GNSS_In` references in shared SoftRF code paths
  (`GNSS.cpp`, `SoftRF.ino`) are safe with no GNSS UART started — add
  guards if needed.
- Confirm the desired default MAVLink baud rate (default is
  `SERIAL_OUT_BR`; ArduPilot serial protocol typically wants 57600 or
  115200).
- Decide whether to expose this model in `EEPROM.cpp` defaulting (NMEA out
  = MAVLink on UART, RF protocol = ADSB_1090) or rely on user-side
  configuration.

## Risks

- **Frequency-path mismatch:** if the user's hardware actually expects
  1090 MHz to come in through the HF antenna port (contrary to radiolib's
  current behavior at this frequency), reception will fail. Mitigation:
  during bring-up, scope DIO5/7 during RX — they should stay LOW. If they
  must be HIGH for the user's antenna design, we'd need to override
  radiolib's `highFreq` or call `setRxPath(RX_PATH_HF)` explicitly after
  setup. Note as bring-up checkpoint.
- **Console / NMEA output collision:** SoftRF's `SerialOutput` is used by
  multiple output sinks. Forcing it to UART0 means USB-side debug must
  use `Serial.print(...)` directly, not `SerialOutput.print(...)`. Spot
  check existing call sites during implementation.
- **`build_opt.h` precedence:** must confirm the flag definition reaches
  `RP2XXX.h` before its `#elif defined(ARDUINO_RASPBERRY_PI_PICO)` chain
  evaluates. Put the new block first.
