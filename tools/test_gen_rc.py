"""Tests for the home layout generator: run with  python -m unittest discover -s tools"""
import copy
import sys
import unittest
from pathlib import Path

import yaml

sys.path.insert(0, str(Path(__file__).resolve().parent))
import gen_rc  # noqa: E402

EXAMPLE = yaml.safe_load((gen_rc.ROOT / "rc_home.example.yaml").read_text(encoding="utf-8"))


def layout(**changes):
    data = copy.deepcopy(EXAMPLE)
    data.update(changes)
    return data


class GeneratorTests(unittest.TestCase):
    def test_example_generates_every_section(self):
        header, entities = gen_rc.build(layout())
        for macro in ("RC_LIGHTS", "RC_SPEAKERS", "RC_SCENES", "RC_DEVICES", "RC_BLINDS_ROOM"):
            self.assertIn(f"#define {macro} ", header)
        self.assertIn("light.kitchen_downlight_1|b", entities)
        self.assertIn("cover.living_room_left|pos", entities)
        self.assertEqual(entities.count("entity_id: light.kitchen_downlight_1\n"), 3)  # group member once

    def test_devices_left_out_get_no_card_and_no_ids(self):
        header, entities = gen_rc.build(layout())
        self.assertNotIn("D_FRYER", header)
        self.assertIn('inline const char *FR = "";', header)
        self.assertIn('#define RC_BLINDS_SINGLE \\\n    {"", "", "", "", ""}', header)

    def test_rejects_mistakes_with_the_offending_field(self):
        bad_icon = layout(); bad_icon["lights"][1]["icon"] = "NOPE"
        wrong_domain = layout(); wrong_domain["lights"][1]["entity"] = "switch.spot"
        two_in_blind = layout(); two_in_blind["devices"]["blind"] = {
            "script": "script.blinds_control",
            "covers": [{"entity": "cover.a", "target": "a", "label": "A"},
                       {"entity": "cover.b", "target": "b", "label": "B"}]}
        cases = {
            "lights[1]: unknown icon": bad_icon,
            "lights[1].entity: expected a light": wrong_domain,
            "devices.blind.covers: list exactly one cover": two_in_blind,
            "music.speakers: list at least one": layout(music={"music_assistant_entry_id": "X1"}),
            "devices: unknown device": layout(devices={"toaster": {}}),
        }
        for message, data in cases.items():
            with self.subTest(message=message):
                with self.assertRaisesRegex(gen_rc.LayoutError, message.replace("[", r"\[").replace("]", r"\]")):
                    gen_rc.build(data)

    def test_labels_are_escaped_for_cpp(self):
        data = layout(); data["scenes"][0]["label"] = 'Say "hi" \\ now'
        header, _ = gen_rc.build(data)
        self.assertIn(r'"Say \"hi\" \\ now"', header)


if __name__ == "__main__":
    unittest.main()
