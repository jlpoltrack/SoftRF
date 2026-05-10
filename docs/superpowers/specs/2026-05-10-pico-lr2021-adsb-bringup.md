# RP2040 + LR2021 ADS-B — Bring-up Checklist

**Firmware:** SoftRF built with `-DPICO_LR2021_ADSB` for `rp2040:rp2040:rpipico`.

## 1. Flash

- Hold BOOTSEL on the Pico, plug in USB.
- Drag `/tmp/softrf-pico-lr2021/SoftRF.ino.uf2` onto the `RPI-RP2` mass-storage volume.
- Pico reboots automatically.

## 2. USB CDC console

- Open the Pico's USB-CDC serial port at 115200 baud (e.g. `screen /dev/tty.usbmodem* 115200`).
- Expected: SoftRF banner including the firmware version and `hw_info.model = SOFTRF_MODEL_ADSB_PICO`.

## 3. LR2021 probe

- The boot log should print `INFO: LR2021 base FW version` followed by the chip firmware revision (radiolib.cpp logs this when `lr2021_probe()` succeeds).
- If absent: check SPI1 wiring (GP10/11/12/13), RST (GP7), BUSY (GP8), and 3V3 supply to the LR2021. The chip should hold BUSY low when idle.

## 4. RF switch

- With a scope or logic analyzer, verify during steady-state RX:
  - DIO5..DIO8 on the LR2021 are all LOW (1090 MHz uses LF RX path).
  - If you switch to a 2.4 GHz protocol via settings, DIO5 and DIO7 should go HIGH (RX_HF).
- This confirms the chip-internal RF switch driver is configured.

## 5. ADS-B reception

- Connect a 1090 MHz antenna (a tuned dipole or commercial ADS-B antenna).
- USB CDC log should show DF17 packet activity (look for `cb_cnt` ticks in the receive callback or the existing radiolib debug lines).
- Cross-check with a separate ADS-B receiver if possible.

## 6. MAVLink output (UART0 → ArduPilot)

- Connect GP0 (TX) to ArduPilot's serial RX, and a common GND.
- In Mission Planner / QGroundControl, configure that serial as `ADSB` protocol at the matching baud rate (default `SERIAL_OUT_BR`; check `SoftRF.h` if uncertain).
- Expected: `ADSB_VEHICLE` MAVLink messages visible in Mission Planner's traffic display, populated with received aircraft.

## 7. Risk: 1090 MHz path mismatch

- If reception fails despite a working antenna and successful probe, check whether the user's hardware actually expects 1090 MHz to enter through the LR2021 HF antenna pin.
- The current implementation matches `SOFTRF_MODEL_PRIME_MK4`'s convention (1090 MHz = LF RX path, all DIOs LOW). If your hardware is wired the other way, override radiolib's `setRxPath(RX_PATH_HF)` post-setup or raise the `high` threshold in `radiolib.cpp:5470` (`bool high = (frequency >= 1500000000)`).

## 8. Known follow-ups

- TX is implemented (TX_LF on DIO6) but not exercised by the ADS-B sniffer use case.
- No GNSS, battery, or button on this board.
- USB CDC stays free for the SoftRF debug log; user may also use it as a secondary data port if SerialOutput is not enough.
- `SOFTRF_MODEL_ADSB_PICO` defaults RF protocol to ADS-B 1090 and `nmea_out` to UART. There is no MAVLink-protocol-specific EEPROM field; selecting the data protocol on the UART output (NMEA / GDL90 / D1090 / MAVLink) is done via the SoftRF web/serial settings interface.
