# Modifications

This program is a modified version of ESPHome-Guition-ESP32-S3-4848S040 by Aaron Ward
(PrayerfulDrop), forked at commit 1212f23. Modified by Artur Safonov, September–October 2026.
The modifications are licensed under the GNU General Public License v3.0, like the original.

## Added

- `Guition-ESP32/rc/`: the Routine Controller UI in C++ (Home, Lights, Devices, Music, scenes,
  bottom navigation), with Music Assistant search and library templates.
- `Guition-ESP32/widgets/rc.yaml`: wires the UI to ESPHome (fonts, Home Assistant calls, Music
  Assistant, test hooks).
- `tools/gen_rc.py`, `rc_home.example.yaml`: the home layout and its generator.
- `homeassistant/rc_call.yaml`: the Home Assistant script the UI calls.
- `Guition-ESP32/widgets/bench.yaml`: an optional render benchmark.
- `Guition-ESP32/img/flags/uk.png`: Ukrainian flag for the language selector.
- `README-ROUTINE-CONTROLLER.md`, `MODIFICATIONS.md`, `THIRD_PARTY_NOTICES.md`.

## Changed

- `main.yaml`: the Lights, Fans, Shortcuts, Covers, Devices and HVAC packages are replaced by the
  Routine Controller UI; new substitutions for the build path and the Wi-Fi setup access point.
- `Guition-ESP32/hardware.yaml`: QIO flash, performance-optimised compilation, a 32 KB
  instruction cache, Wi-Fi and LwIP buffers in PSRAM and a smaller LVGL draw buffer; Wi-Fi is
  provisioned through a captive portal or Improv instead of credentials in secrets.yaml.
- Widgets under `Guition-ESP32/widgets/`: Ukrainian translations, theme colours, layout and
  font adjustments to match the new UI, and a media player page that follows the selected speaker.
- `secrets.yaml.example`: the secrets this configuration uses.
