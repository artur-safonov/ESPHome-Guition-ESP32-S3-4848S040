#!/usr/bin/env python3
"""Generate the Routine Controller home layout from rc_home.yaml.

The Routine Controller UI (Guition-ESP32/rc/*.h) is written in C++, and it needs to know which
Home Assistant entities to draw and which to subscribe to. Both come from one file you edit,
rc_home.yaml (start from rc_home.example.yaml). This script turns it into two generated files:

    Guition-ESP32/rc/rc_config.h          entity ids, lights, speakers, scenes, devices for C++
    Guition-ESP32/widgets/rc_entities.yaml Home Assistant state subscriptions for ESPHome

Both are ignored by Git. Run it again whenever rc_home.yaml changes, then compile as usual:

    python tools/gen_rc.py                      # reads rc_home.yaml
    python tools/gen_rc.py rc_home.example.yaml # or any other layout file

Requires Python 3.9+ and PyYAML (installed with ESPHome).
"""
from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

import yaml

ROOT = Path(__file__).resolve().parents[1]
CORE = ROOT / "Guition-ESP32" / "rc" / "rc_core.h"
OUT_H = ROOT / "Guition-ESP32" / "rc" / "rc_config.h"
OUT_YAML = ROOT / "Guition-ESP32" / "widgets" / "rc_entities.yaml"

ENTITY = re.compile(r"^[a-z_]+\.[a-z0-9_]+$")
LANGS = ("en", "ru", "uk")
FEATURES = {"brightness": 1, "color_temp": 2, "rgb": 4}

# Which attributes each domain publishes to the UI, and the key suffix C++ reads them under.
ATTRIBUTES = {
    "light": [("brightness", "b"), ("rgb_color", "rgb")],
    "climate": [("temperature", "t"), ("fan_mode", "fan")],
    "media_player": [("media_title", "title"), ("media_artist", "artist"),
                     ("entity_picture", "pic"), ("media_position", "pos"),
                     ("media_duration", "dur"), ("volume_level", "vol")],
}

# Device cards, in display order: config key, C++ key, icon, colour, default labels.
DEVICES = [
    ("kettle", "D_KETTLE", "KETTLE", "C_ORANGE", ("Kettle", "Чайник", "Чайник")),
    ("dishwasher", "D_DISH", "DISHWASHER", "C_SKY", ("Dishwasher", "Посудомойка", "Посудомийка")),
    ("air_fryer", "D_FRYER", "OVEN", "C_AMBER", ("Air fryer", "Аэрогриль", "Аерогриль")),
    ("vacuum", "D_VAC", "VACUUM", "C_TEAL", ("Robot vacuum", "Пылесос", "Пилосос")),
    ("blind", "D_KBLIND", "SHADE", "C_PURPLE", ("Blind", "Штора", "Штора")),
    ("climate", "D_AC", "AC", "C_SKY", ("Air conditioner", "Кондиционер", "Кондиціонер")),
    ("diffuser", "D_DIF", "SCENT", "C_ROSE", ("Aroma diffuser", "Диффузор", "Дифузор")),
    ("blinds", "D_RBLINDS", "SHADE", "C_PURPLE", ("Blinds", "Шторы", "Штори")),
]

# Entities each device needs: config field -> C++ constant. Fields marked * are subscribed to.
DEVICE_FIELDS = {
    "kettle": [("*power", "KETTLE_ON"), ("*temperature", "KETTLE_T"),
               ("*target_temperature", "KETTLE_TGT"), ("*keep_warm", "KETTLE_WARM"),
               ("*status", "KETTLE_ST")],
    "dishwasher": [("*operation_state", "DW_OP"), ("*program_progress", "DW_PROG")],
    "air_fryer": [("*state", "FR"), ("*time_left", "FR_LEFT"), ("cancel_button", "FR_CANCEL")],
    "vacuum": [("*entity", "VAC"), ("*battery", "VAC_BAT"), ("*mop_type", "VAC_TYPE"),
               ("*suction", "VAC_SUCT"), ("*water_level", "VAC_WATER"),
               ("*cleaned_area", "VAC_AREA"), ("pause_button", "VAC_PAUSE"),
               ("resume_button", "VAC_RESUME")],
    "climate": [("*entity", "AC"), ("*room_temperature", "ROOM_T")],
    "diffuser": [("*power", "DIF_ON"), ("*run_script", "DIF_RUN")],
    "blind": [],
    "blinds": [],
}
SCRIPT_CONST = {"kettle": "S_KETTLE", "dishwasher": "S_DISHWASHER", "air_fryer": "S_FRYER",
                "vacuum": "S_VACUUM", "climate": "S_CLIMATE", "diffuser": "S_DIFFUSER",
                "blind": "S_BLINDS", "blinds": "S_BLINDS"}


class LayoutError(ValueError):
    pass


def core_names() -> tuple[set[str], set[str]]:
    """Icon and colour names that exist in rc_core.h, so the layout can only use real ones."""
    text = CORE.read_text(encoding="utf-8")
    icons = set(re.findall(r"\b([A-Z][A-Z0-9_]*) = 0x[0-9A-F]+",
                           text.split("namespace ic {", 1)[1].split("}", 1)[0]))
    colours = set(re.findall(r"\b(C_[A-Z0-9_]+) = 0x", text))
    return icons, colours


def c_str(value: str | None) -> str:
    if value is None:
        return "nullptr"
    return '"' + value.replace("\\", "\\\\").replace('"', '\\"') + '"'


def entity(value, where: str, domain: str | None = None, optional: bool = False) -> str:
    value = "" if value is None else str(value).strip()
    if not value and optional:
        return ""
    if not ENTITY.match(value):
        raise LayoutError(f"{where}: expected an entity id such as light.kitchen, got {value!r}")
    if domain and not value.startswith(domain + "."):
        raise LayoutError(f"{where}: expected a {domain}.* entity, got {value!r}")
    return value


def labels(raw, where: str, default=None) -> tuple[str, str, str]:
    if raw is None and default is not None:
        return default
    if isinstance(raw, str):
        return (raw, raw, raw)
    if not isinstance(raw, dict) or not raw.get("en"):
        raise LayoutError(f"{where}: label must be text or a mapping with at least 'en'")
    return tuple(str(raw.get(lang) or raw["en"]) for lang in LANGS)


def icon(name, where: str, icons: set[str]) -> str:
    name = str(name or "").upper()
    if name not in icons:
        raise LayoutError(f"{where}: unknown icon {name!r}; use one of {', '.join(sorted(icons))}")
    return f"ic::{name}"


def colour(name, where: str, colours: set[str]) -> str:
    raw = str(name or "")
    if re.fullmatch(r"#?[0-9A-Fa-f]{6}", raw):
        return "0x" + raw.lstrip("#").upper()
    const = "C_" + raw.upper().removeprefix("C_")
    if const not in colours:
        raise LayoutError(f"{where}: unknown colour {raw!r}; use #RRGGBB or one of "
                          f"{', '.join(sorted(c[2:] for c in colours))}")
    return const


def room(value, where: str) -> int:
    if value not in (0, 1):
        raise LayoutError(f"{where}: room must be 0 (first tab) or 1 (second tab)")
    return value


class Subscriptions:
    def __init__(self):
        self.items: list[tuple[str, str, str]] = []
        self._seen: set[tuple[str, str]] = set()

    def add(self, ent: str, *, position: bool = False) -> None:
        if not ent:
            return
        domain = ent.split(".", 1)[0]
        wanted = [("", "")] + ATTRIBUTES.get(domain, [])
        if position:
            wanted.append(("current_position", "pos"))
        for attribute, suffix in wanted:
            if (ent, attribute) in self._seen:
                continue
            self._seen.add((ent, attribute))
            self.items.append((ent, attribute, f"{ent}|{suffix}" if suffix else ent))


def build(layout: dict) -> tuple[str, str]:
    if not isinstance(layout, dict):
        raise LayoutError("the layout file must be a YAML mapping")
    icons, colours = core_names()
    subs = Subscriptions()
    h: list[str] = []

    rooms = layout.get("rooms") or [{"en": "Kitchen", "ru": "Кухня", "uk": "Кухня"},
                                   {"en": "Room", "ru": "Комната", "uk": "Кімната"}]
    if not isinstance(rooms, list) or len(rooms) != 2:
        raise LayoutError("rooms: list exactly two tab names")
    room_labels = [labels(r, f"rooms[{i}]") for i, r in enumerate(rooms)]

    # ── lights ──
    lights = layout.get("lights") or []
    if not lights:
        raise LayoutError("lights: list at least one light")
    light_rows = []
    for i, d in enumerate(lights):
        where = f"lights[{i}]"
        members = d.get("members")
        features = d.get("features", list(FEATURES))
        unknown_features = [f for f in features if f not in FEATURES]
        if unknown_features:
            raise LayoutError(f"{where}: unknown feature(s) {unknown_features}; "
                              f"use {', '.join(FEATURES)}")
        caps = sum(FEATURES[f] for f in features)
        if members:
            key = str(d.get("key") or "").strip()
            if not re.fullmatch(r"[a-z0-9_]+", key):
                raise LayoutError(f"{where}: a group needs a key such as kitchen_all")
            ents = [entity(m, f"{where}.members", "light") for m in members]
            for m in ents:
                subs.add(m)
            ent, csv = key, ",".join(ents)
        else:
            ent, csv = entity(d.get("entity"), f"{where}.entity", "light"), None
            subs.add(ent)
        en, ru, uk = labels(d.get("label"), where)
        light_rows.append(f"{{{c_str(ent)}, {c_str(csv)}, {icon(d.get('icon'), where, icons)}, "
                          f"{caps}, {room(d.get('room', 0), where)}, {c_str(en)}, {c_str(ru)}, "
                          f"{c_str(uk)}}}")

    # ── devices ──
    devices = layout.get("devices") or {}
    unknown = set(devices) - {d[0] for d in DEVICES}
    if unknown:
        raise LayoutError(f"devices: unknown device(s) {', '.join(sorted(unknown))}")
    consts: dict[str, str] = {}
    scripts: dict[str, str] = {}
    dev_rows, blinds = [], {"blind": [], "blinds": []}
    for key, dev_key, dev_icon, dev_colour, default_label in DEVICES:
        d = devices.get(key)
        for field, const in DEVICE_FIELDS[key]:
            consts.setdefault(const, "")
        scripts.setdefault(SCRIPT_CONST[key], "")
        if not d:
            continue
        where = f"devices.{key}"
        for field, const in DEVICE_FIELDS[key]:
            subscribed = field.startswith("*")
            name = field.lstrip("*")
            value = entity(d.get(name), f"{where}.{name}", optional=not subscribed)
            consts[const] = value
        script = entity(d.get("script"), f"{where}.script", "script")
        if scripts[SCRIPT_CONST[key]] and scripts[SCRIPT_CONST[key]] != script:
            raise LayoutError(f"{where}.script: blind and blinds must use the same script")
        scripts[SCRIPT_CONST[key]] = script
        if key in blinds:
            covers = d.get("covers") or []
            if not covers or (key == "blind" and len(covers) != 1):
                raise LayoutError(f"{where}.covers: list {'exactly one cover' if key == 'blind' else 'at least one cover'}")
            for j, c in enumerate(covers):
                cw = f"{where}.covers[{j}]"
                cover = entity(c.get("entity"), f"{cw}.entity", "cover")
                target = str(c.get("target") or "").strip()
                if not target:
                    raise LayoutError(f"{cw}.target: the name your blinds script uses for this cover")
                en, ru, uk = labels(c.get("label"), cw)
                blinds[key].append(f"{{{c_str(cover)}, {c_str(target)}, {c_str(en)}, {c_str(ru)}, {c_str(uk)}}}")
                blinds[key + "_subs"] = blinds.get(key + "_subs", []) + [(cover, bool(c.get("position", False)))]
        en, ru, uk = labels(d.get("label"), where, default_label)
        dev_rows.append(f"{{{dev_key}, ic::{dev_icon}, {dev_colour}, {room(d.get('room', 0), where)}, "
                        f"{c_str(en)}, {c_str(ru)}, {c_str(uk)}}}")
    if not dev_rows:
        raise LayoutError("devices: configure at least one device")
    # Subscribe in the order the UI groups them: devices, then room blinds, then the single blind.
    for key, *_ in DEVICES:
        for field, const in DEVICE_FIELDS[key]:
            if field.startswith("*"):
                subs.add(consts[const])
    for key in ("blinds", "blind"):
        for cover, position in blinds.get(key + "_subs", []):
            subs.add(cover, position=position)

    # ── music ──
    music = layout.get("music") or {}
    speakers = music.get("speakers") or []
    if not speakers:
        raise LayoutError("music.speakers: list at least one media player")
    speaker_rows = []
    for i, s in enumerate(speakers):
        where = f"music.speakers[{i}]"
        ent = entity(s.get("entity"), f"{where}.entity", "media_player")
        subs.add(ent)
        en, ru, uk = labels(s.get("label"), where)
        speaker_rows.append(f"{{{c_str(ent)}, {icon(s.get('icon', 'SPEAKER'), where, icons)}, "
                            f"{c_str(en)}, {c_str(ru)}, {c_str(uk)}}}")
    spotify = entity(music.get("spotify"), "music.spotify", "media_player", optional=True)
    subs.add(spotify)
    ma_entry = str(music.get("music_assistant_entry_id") or "").strip()
    if not re.fullmatch(r"[0-9A-Za-z_]+", ma_entry):
        raise LayoutError("music.music_assistant_entry_id: the config entry id of your Music "
                          "Assistant integration (Settings > Devices & services > Music Assistant)")

    # ── scenes ──
    scenes = layout.get("scenes") or []
    if not scenes:
        raise LayoutError("scenes: list at least one scene script")
    scene_rows = []
    for i, s in enumerate(scenes):
        where = f"scenes[{i}]"
        ent = entity(s.get("script"), f"{where}.script", "script")
        en, ru, uk = labels(s.get("label"), where)
        sen, sru, suk = labels(s.get("subtitle", ""), f"{where}.subtitle") if s.get("subtitle") else ("", "", "")
        scene_rows.append(
            f"{{{c_str(ent)}, {icon(s.get('icon'), where, icons)}, "
            f"{colour(s.get('color', 'ACCENT'), where + '.color', colours)}, "
            f"{colour(s.get('text_color', 'FFFFFF'), where + '.text_color', colours)}, "
            f"{c_str(en)}, {c_str(ru)}, {c_str(uk)}, {c_str(sen)}, {c_str(sru)}, {c_str(suk)}}}")

    # ── rc_config.h ──
    def initialiser(name: str, rows: list[str]) -> str:
        return f"#define {name} \\\n    " + ", \\\n    ".join(rows) + "\n"

    def placeholder(rows: list[str]) -> list[str]:
        # Arrays may not be empty in C++; a disabled card never reads its placeholder entry.
        return rows or ['{"", "", "", "", ""}']

    h.append("// Generated by tools/gen_rc.py from your layout file. Do not edit; edit the layout.\n")
    h.append("#pragma once\n\nnamespace rc {\nnamespace e {\n")
    for const, value in consts.items():
        h.append(f"inline const char *{const} = {c_str(value)};\n")
    h.append("}  // namespace e\nnamespace s {\n")
    for const, value in scripts.items():
        h.append(f"inline const char *{const} = {c_str(value)};\n")
    h.append("}  // namespace s\n")
    h.append(f"inline const char *SPOTIFY_ENT = {c_str(spotify)};\n")
    for i, (en, ru, uk) in enumerate(room_labels):
        h.append(f"inline const char *ROOM{i}_EN = {c_str(en)}, *ROOM{i}_RU = {c_str(ru)}, "
                 f"*ROOM{i}_UK = {c_str(uk)};\n")
    h.append("}  // namespace rc\n\n")
    h.append(initialiser("RC_LIGHTS", light_rows))
    h.append(initialiser("RC_SPEAKERS", speaker_rows))
    h.append(initialiser("RC_SCENES", scene_rows))
    h.append(initialiser("RC_DEVICES", dev_rows))
    h.append(initialiser("RC_BLINDS_ROOM", placeholder(blinds["blinds"])))
    h.append(initialiser("RC_BLINDS_SINGLE", placeholder(blinds["blind"])))

    # ── rc_entities.yaml ──
    y = ["# Generated by tools/gen_rc.py from your layout file. Do not edit; edit the layout.",
         "substitutions:",
         f'  ma_entry_id: "{ma_entry}"',
         "",
         "text_sensor:"]
    for n, (ent, attribute, key) in enumerate(subs.items):
        y += ["  - platform: homeassistant", f"    id: rcs_{n}", f"    entity_id: {ent}"]
        if attribute:
            y.append(f"    attribute: {attribute}")
        y += ["    on_value:", f"      - lambda: 'rc::ha_state(\"{key}\", x);'"]
    return "".join(h), "\n".join(y) + "\n"


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("layout", nargs="?", default=str(ROOT / "rc_home.yaml"))
    args = parser.parse_args()
    path = Path(args.layout)
    if not path.is_file():
        print(f"{path} not found. Copy rc_home.example.yaml to rc_home.yaml and edit it.", file=sys.stderr)
        return 1
    try:
        header, entities = build(yaml.safe_load(path.read_text(encoding="utf-8")))
    except LayoutError as exc:
        print(f"{path}: {exc}", file=sys.stderr)
        return 1
    OUT_H.write_text(header, encoding="utf-8", newline="\n")
    OUT_YAML.write_text(entities, encoding="utf-8", newline="\n")
    count = entities.count("platform: homeassistant")
    print(f"wrote {OUT_H.relative_to(ROOT)} and {OUT_YAML.relative_to(ROOT)} ({count} subscriptions)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
