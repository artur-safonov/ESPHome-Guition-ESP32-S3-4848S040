# Routine Controller UI

This fork replaces the dashboard's Lights, Devices and media pages with a touch UI written in C++
on top of LVGL, for the same Guition ESP32-S3-4848S040 480×480 display:

- **Home** keeps the original clock, weather and calendar, and adds scene tiles and a
  now-playing bar.
- **Lights** groups lights into two rooms. Tap a tile to switch it; open it for brightness,
  white temperature and colour.
- **Devices** shows a card per appliance with its live status and one quick action, and a
  detail screen for each: kettle, dishwasher, air fryer, robot vacuum, air conditioner, aroma
  diffuser and blinds.
- **Music** searches and plays through Music Assistant, shows your library, and moves playback
  between speakers.

The UI speaks English, Russian and Ukrainian, following the firmware's language setting.

## How it is wired

```text
rc_home.yaml ──tools/gen_rc.py──► Guition-ESP32/rc/rc_config.h           (ids for the C++ UI)
                               └► Guition-ESP32/widgets/rc_entities.yaml (Home Assistant state)
main.yaml + your *.local.yaml ──► widgets/rc.yaml + rc/*.h ──► ESPHome
```

Every button on the display calls one Home Assistant script, `script.rc_call`, with a service
name and JSON data; appliances are driven through scripts you write for your own devices.

## Set up

1. **Home Assistant.**
   - Install [Music Assistant](https://www.music-assistant.io/) if you want the Music page.
   - Create `script.rc_call` from [homeassistant/rc_call.yaml](homeassistant/rc_call.yaml).
   - Create a script for each appliance you list (interfaces below), and one script per scene.
   - In the ESPHome device settings, allow the device to perform Home Assistant actions.
2. **Your layout.** Copy `rc_home.example.yaml` to `rc_home.yaml` and enter your entity ids.
   Leave out any appliance you do not have; its card disappears.
3. **Generate.** `python tools/gen_rc.py` checks the ids, icons and colours and writes the two
   generated files. Run it again after every change to `rc_home.yaml`.
4. **Your display.** Create `my-display.local.yaml` (any `*.local.yaml` is ignored by Git):

   ```yaml
   packages:
     base: !include main.yaml
   substitutions:
     esphome_name: kitchen-display
     time_zone: Europe/Berlin
     weather_entity: weather.home
   ```

   Any substitution from `main.yaml` can be overridden there. Copy `secrets.yaml.example` to
   `secrets.yaml` and fill it in.
5. **Build and flash.** `esphome run my-display.local.yaml`. On first boot, join the
   "Routine Controller Setup" Wi-Fi network from a phone and pick your network.

On Windows, keep both paths short: set `build_path` to something like `C:/esp/build`, and if
ESP-IDF fails to unpack a component with "No such file or directory", set the environment
variable `ESPHOME_DATA_DIR` to a short folder. Both work around the 260-character path limit.

## Appliance scripts

The display passes these variables. Write each script for your own hardware; the script names in
`rc_home.yaml` can be anything.

| Device | Variables the display sends |
|---|---|
| kettle | `temperature` (number) to heat; `power: off` to stop |
| dishwasher | `program`: `eco_50`, `auto`, `intensive_70`, `quick_65`, `pre_rinse`, with `extra_dry` (bool); `action: stop` |
| air_fryer | `mode`: a device program (`French Fries`, `Chicken Wing`, `Pizza`...) or `Manual` with `temperature` and `minutes` |
| vacuum | `mode`: `vacuum`, `vacuum_and_mop`, `mop`; `suction`: `silent`, `standard`, `strong`, `turbo`; `water_level`: 1–3; `action: return` to dock |
| climate | `power`: `on` / `off`, optionally `temperature`. Mode, target and fan are set on the climate entity directly. |
| diffuser | `action: on` with `duration_minutes` and `interval_minutes`; `action: off` |
| blind / blinds | `target` (the name from `rc_home.yaml`) with `action`: `open`, `close`, or `position` with `close_percent` |

The program, mode and preset lists match the appliances this UI was built for (a Home Connect
dishwasher, a Xiaomi air fryer and robot vacuum). For other models, map these values in your script.

## Status

The UI has been used daily on one display. This release moved the home layout out of the C++
source into `rc_home.yaml`. Firmware generated from the example layout and from the original
home's layout both compile; the original layout reproduces the original UI. The example layout
has not been run on a display.

## Credits and license

This is a fork of [PrayerfulDrop/ESPHome-Guition-ESP32-S3-4848S040](https://github.com/PrayerfulDrop/ESPHome-Guition-ESP32-S3-4848S040),
itself based on [alaltitov/Guition-ESP32-S3-4848S040](https://github.com/alaltitov/Guition-ESP32-S3-4848S040).
It is distributed under the GNU General Public License v3.0 ([LICENSE](LICENSE)), like the
version it was forked from. The changes are listed in [MODIFICATIONS.md](MODIFICATIONS.md);
fonts and icons are listed in [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).
