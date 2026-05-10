# RP2040 + LR2021 ADS-B Hardware Test Plan

## Scope

Validate the custom Pi Pico RP2040 + LR2021 ADS-B firmware on real hardware before treating the branch as hardware-ready.

Firmware artifact:

`software/firmware/binaries/RP2040/SoftRF/SoftRF-firmware-v1.9-Pico-LR2021-ADSB.uf2`

Build identity:

- Board target: `rp2040:rp2040:rpipico`
- Build flag: `-DPICO_LR2021_ADSB`
- SoftRF firmware version: `1.9`
- Expected model: `SOFTRF_MODEL_ADSB_PICO`
- UF2 SHA-256: `882e00d48db0a74001ec6e4775a7de289762ad2c29e5c0e0a633e47a3d15da22`

## Required Equipment

- Custom RP2040 + LR2021 ADS-B board.
- USB cable for Pico BOOTSEL flashing and USB CDC logs.
- 1090 MHz ADS-B antenna.
- ArduPilot target or UART-to-USB adapter that can decode MAVLink.
- Logic analyzer or oscilloscope for SPI, IRQ, BUSY, RESET, and LR2021 DIO5..DIO8 checks.
- Optional known-good ADS-B receiver for cross-checking aircraft reception.

## Test Matrix

| Area | Required Result |
| --- | --- |
| Flashing | UF2 copies to `RPI-RP2` and board reboots |
| USB CDC boot log | SoftRF boots and identifies the ADS-B Pico model |
| LR2021 probe | LR2021 firmware version is printed |
| SPI1 bus | GP10/GP11/GP12/GP13 activity during radio init |
| IRQ path | LR2021 DIO9/board IRQ reaches GP6 |
| RF switch | 1090 MHz RX path uses expected DIO5..DIO8 state |
| ADS-B receive | DF17/Mode-S traffic appears with plausible ICAO addresses |
| MAVLink output | `ADSB_VEHICLE` messages appear on UART0 TX GP0 |

## Procedure

### 1. Flash Firmware

1. Hold BOOTSEL while connecting USB.
2. Copy `software/firmware/binaries/RP2040/SoftRF/SoftRF-firmware-v1.9-Pico-LR2021-ADSB.uf2` to the `RPI-RP2` volume.
3. Confirm the board reboots and the mass-storage volume disappears.

Pass: board reboots after the UF2 copy without manual reset.

### 2. Capture Boot Log

1. Open the USB CDC serial port at 115200 baud.
2. Reset the board.
3. Save the full boot log.

Pass:

- SoftRF banner appears.
- Firmware version is `1.9`.
- Board/model path reports `SOFTRF_MODEL_ADSB_PICO` or equivalent ADS-B Pico identity.
- No GNSS probe failure is treated as a fatal startup error.

### 3. Verify LR2021 Initialization

1. Watch the boot log for the LR2021 probe line.
2. Probe these pins during startup:
   - SPI1 SCK GP10
   - SPI1 MOSI GP11
   - SPI1 MISO GP12
   - SPI1 CS GP13
   - LR2021 reset GP7
   - LR2021 busy GP8
   - IRQ GP6

Pass:

- LR2021 firmware version is printed.
- SPI transactions occur during probe/init.
- BUSY settles to idle after commands.
- IRQ line changes when packets or radio events occur.

### 4. Verify RF Switch State

1. Attach a scope or logic analyzer to LR2021 DIO5, DIO6, DIO7, and DIO8.
2. Let the firmware enter steady 1090 MHz receive mode.

Pass:

- DIO5..DIO8 match the expected 1090 MHz RX state from the current firmware table.
- No MCU GPIO attempts to drive an external antenna switch line.

If the radio probes but receives no aircraft, record the DIO5..DIO8 state and test whether the hardware expects 1090 MHz on the LR2021 HF path instead of the LF path.

### 5. Verify ADS-B Reception

1. Connect a 1090 MHz antenna.
2. Place the board where aircraft are normally visible, or use a controlled ADS-B test source if available.
3. Compare received ICAO addresses and positions against a known-good ADS-B receiver or public traffic display.

Pass:

- Aircraft records appear without a local GNSS fix.
- USB CDC prints `ADSB,...` lines with decoded ICAO, optional callsign, position, altitude, speed/course, vertical speed, and RSSI.
- ICAO addresses are stable and plausible.
- Position, altitude, course, and speed values are populated when present in received frames.

### 6. Verify MAVLink Output

1. Connect GP0 TX to ArduPilot serial RX, with common ground.
2. Optionally connect ArduPilot TX to GP1 RX if heartbeat/input visibility is required.
3. Configure the ArduPilot serial port for ADS-B/MAVLink at the firmware UART baud rate.
4. Watch Mission Planner, QGroundControl, or a MAVLink inspector.

Pass:

- MAVLink heartbeat from the ADS-B component appears, if monitored.
- `ADSB_VEHICLE` messages are emitted for received traffic.
- No NMEA, GDL90, or D1090 bytes are interleaved on the same UART by default.

### 7. Soak Test

1. Run the board for at least 60 minutes with antenna and MAVLink output connected.
2. Record USB CDC logs and MAVLink traffic counts.

Pass:

- No watchdog resets.
- No repeated LR2021 probe/init failures.
- Traffic entries expire when aircraft disappear.
- MAVLink output continues after quiet periods and renewed ADS-B reception.

## Evidence To Keep

- Boot log text file.
- Logic analyzer capture of LR2021 init and at least one IRQ event.
- Screenshot or log showing `ADSB_VEHICLE` MAVLink messages.
- Cross-check notes against known-good ADS-B receiver.
- Final pass/fail table with hardware revision, firmware SHA-256, antenna, and test location.

## Known Risks

- The RF switch table assumes the current board routes 1090 MHz through the LR2021 LF RX path. Hardware using the HF path will probe correctly but receive poorly or not at all.
- ADS-B reception depends strongly on antenna placement and nearby aircraft density.
- UART0 is intended for MAVLink by default; enabling other serial outputs on the same UART can corrupt the MAVLink stream.
