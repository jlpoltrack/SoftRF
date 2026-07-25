# RP2040 + LR2021 ADS-B — Bring-up Checklist

**Firmware:** SoftRF built with `-DPICO_LR2021_ADSB`. Either
`rp2040:rp2040:rpipico` or `rp2040:rp2040:generic` works — the flag now pulls in
RadioLib itself, so the FQBN no longer decides whether the LR2021 driver is
compiled in.

```
arduino-cli compile \
  --fqbn rp2040:rp2040:rpipico \
  --libraries software/firmware/source/libraries \
  --build-property "build.extra_flags=-DPICO_LR2021_ADSB" \
  software/firmware/source/SoftRF/SoftRF.ino
```

A correct build is ~268 KB. If you get ~219 KB, the flag did not reach the
compiler and you have a firmware with no radio driver — see step 3.

## 1. Flash

- Hold BOOTSEL on the Pico, plug in USB.
- Drag `/tmp/softrf-pico-lr2021/SoftRF.ino.uf2` onto the `RPI-RP2` mass-storage volume.
- Pico reboots automatically.

## 2. USB CDC console

- Open the Pico's USB-CDC serial port at 115200 baud (e.g. `screen /dev/tty.usbmodem* 115200`).
- Expected: SoftRF banner including the firmware version and `hw_info.model = SOFTRF_MODEL_ADSB_PICO`.

## 3. LR2021 probe

- The boot log should print `INFO: LR2021 base FW version` followed by the chip firmware revision (radiolib.cpp logs this when `lr2021_probe()` succeeds), then `LR2021 RFIC is detected.`
- If you see `WARNING! None of supported RFICs is detected!`, distinguish the two causes:
  - **No driver compiled in** — the firmware has no LR2021 code at all. Check with `strings SoftRF.ino.elf | grep -c -i radiolib`; a correct build reports ~200, a broken one reports 0. Cause is the `-DPICO_LR2021_ADSB` flag not reaching the compiler.
  - **Driver present, chip not answering** — check SPI1 wiring (GP10/11/12/13), RST (GP7), BUSY (GP8), and 3V3 supply to the LR2021. The chip should hold BUSY low when idle.

## 4. RF switch

- With a scope or logic analyzer, verify during steady-state RX:
  - DIO5..DIO8 on the LR2021 are all LOW (1090 MHz uses LF RX path).
  - If you switch to a 2.4 GHz protocol via settings, DIO5 and DIO7 should go HIGH (RX_HF).
- This confirms the chip-internal RF switch driver is configured.

## 5. ADS-B reception — 1090 ES and UAT 978

Both links are compiled in, but the radio decodes **one at a time** —
`settings->rf_protocol` is a single value. 1090 ES is the default for this
model.

- Connect an antenna for the band you are testing (1090 MHz dipole / commercial
  ADS-B antenna; 978 MHz is close enough that a 1090 antenna will usually hear
  strong UAT traffic).
- USB CDC log should show decoded updates in this form, for either link:
  `ADSB,ICAO=A1B2C3,CALL=N123AB,LAT=37.618900,LON=-122.375000,ALT_M=3050,SPD_KT=142,CRS=274,VS=0,RSSI=-82`.
- Cross-check with a separate ADS-B receiver if possible.

### Switching to UAT 978

There is no web UI on this target (`EXCLUDE_WIFI`), so use `$PSRFC` on the USB
CDC console. Protocol indices come from `protocol.h`: `3` = ADS-B 1090 ES,
`4` = ADS-B UAT 978.

- Send the current settings query first, then re-send the sentence with field 3
  changed to `4`. Confirm with the `Protocol = 4` echo, then reboot.
- The RF switch table needs no change: 978 MHz is below the 1.5 GHz threshold,
  so it uses the same LF RX path as 1090.
- UAT is a US-only datalink. Outside the US there will be nothing to hear.

## 6. MAVLink output (UART0 → ArduPilot)

- Connect GP0 (TX) to ArduPilot's serial RX, and a common GND.
- Optionally connect ArduPilot TX to GP1 if you want SoftRF to see autopilot heartbeats.
- Baud is **57600** (`SERIAL_OUT_BR` is overridden for this board) — ArduPilot's
  `SERIALn_BAUD` default, so no change needed on the autopilot side.
- On the autopilot: set that port's `SERIALn_PROTOCOL` to MAVLink and enable
  `ADSB_TYPE` so `AP_ADSB` consumes the stream. Frames are MAVLink v1.
- Expected: `ADSB_VEHICLE` MAVLink messages visible in Mission Planner's traffic display, populated with received aircraft.

## 7. Risk: 1090 MHz path mismatch

- If reception fails despite a working antenna and successful probe, check whether the user's hardware actually expects 1090 MHz to enter through the LR2021 HF antenna pin.
- The current implementation matches `SOFTRF_MODEL_PRIME_MK4`'s convention (1090 MHz = LF RX path, all DIOs LOW). If your hardware is wired the other way, override radiolib's `setRxPath(RX_PATH_HF)` post-setup or raise the `high` threshold in `radiolib.cpp` (`bool high = (frequency >= 1500000000)`).

## 8. Known follow-ups

- TX is implemented (TX_LF on DIO6) but not exercised by the ADS-B sniffer use case.
- No GNSS, battery, or button on this board.
- USB CDC stays free for the SoftRF debug log; user may also use it as a secondary data port if SerialOutput is not enough.
- `SOFTRF_MODEL_ADSB_PICO` defaults RF protocol to ADS-B 1090, disables RF TX power, and leaves NMEA/GDL90/D1090 UART output off so MAVLink is not mixed with text/binary non-MAVLink output on UART0.
- Own position is never populated on this board (no GNSS, and the autopilot's
  position is not copied back into `ThisAircraft`). `ADSB_VEHICLE` carries
  absolute coordinates so the output is correct, but `fop->distance` /
  `bearing` are measured from 0°/0°, which makes the distance-based eviction in
  `Traffic_Add()` arbitrary once more than `MAX_TRACKING_OBJECTS` (8) aircraft
  are in range. Switching this model's default to `SOFTRF_MODE_UAV` would fix
  it by reusing `uav()`.
