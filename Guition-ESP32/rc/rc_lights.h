#pragma once
// Lights tab: rooms, tiles (tap = on/off, tune = brightness / white / colour).
#include "rc_home.h"

namespace rc {

enum Cap : uint8_t { CAP_BRI = 1, CAP_CT = 2, CAP_RGB = 4 };
struct LightDef {
  const char *ent;      // entity or group key
  const char *members;  // csv of member entities for groups, nullptr otherwise
  uint32_t icon; uint8_t caps; uint8_t room;
  const char *en, *ru, *uk;
};
inline const LightDef LIGHTS[] = {RC_LIGHTS};
constexpr int N_LIGHTS = sizeof(LIGHTS) / sizeof(LIGHTS[0]);

inline std::vector<std::string> split_csv(const char *csv) {
  std::vector<std::string> out;
  std::string s(csv), cur;
  for (char c : s) { if (c == ',') { out.push_back(cur); cur.clear(); } else cur += c; }
  if (!cur.empty()) out.push_back(cur);
  return out;
}
inline std::vector<std::string> light_ents(const LightDef &d) {
  if (d.members) return split_csv(d.members);
  return {d.ent};
}
inline std::string light_target(const LightDef &d) { return d.members ? ents(d.members) : "\"" + std::string(d.ent) + "\""; }
inline bool light_on(const LightDef &d) {
  for (auto &e : light_ents(d)) if (on(e)) return true;
  return false;
}
inline int light_pct(const LightDef &d) {
  int n = 0; float sum = 0;
  for (auto &e : light_ents(d)) if (on(e)) { sum += stf(e + "|b", 255); n++; }
  return n ? (int) lroundf(sum / n * 100.f / 255.f) : 0;
}
inline bool parse_rgb(const std::string &s, int &r, int &g, int &b) {
  int v[3], k = 0; const char *p = s.c_str();
  while (*p && k < 3) {
    if (*p >= '0' && *p <= '9') { v[k++] = (int) strtol(p, (char **) &p, 10); } else p++;
  }
  if (k < 3) return false;
  r = v[0]; g = v[1]; b = v[2];
  return true;
}
inline uint32_t light_color(const LightDef &d) {
  for (auto &e : light_ents(d)) {
    if (!on(e)) continue;
    int r, g, b;
    if (parse_rgb(st(e + "|rgb"), r, g, b)) {
      // Lift very dark / desaturated colours so the badge stays readable.
      int mx = r > g ? (r > b ? r : b) : (g > b ? g : b);
      if (mx < 120 && mx > 0) { r = r * 200 / mx; g = g * 200 / mx; b = b * 200 / mx; }
      if (mx == 0) return C_LAMP;
      return ((uint32_t) (r > 255 ? 255 : r) << 16) | ((uint32_t) (g > 255 ? 255 : g) << 8) | (uint32_t) (b > 255 ? 255 : b);
    }
    return C_LAMP;
  }
  return C_LAMP;
}
inline void light_set_optimistic(const LightDef &d, bool v) {
  for (auto &e : light_ents(d)) set_state(e, v ? "on" : "off");
}
inline void light_power(const LightDef &d, bool v) {
  light_set_optimistic(d, v);
  call(v ? "light.turn_on" : "light.turn_off", J().raw("entity_id", light_target(d)).str());
}

struct LTile { const LightDef *d; lv_obj_t *card, *chip, *status, *bar; };
struct LightsUi { int room = 0; lv_obj_t *grid = nullptr; Seg *seg = nullptr; std::vector<LTile> tiles; };
inline LightsUi LUI;

void open_light(const LightDef &d);

inline void lights_grid() {
  if (LUI.grid) lv_obj_clean(LUI.grid);
  LUI.tiles.clear();
  int k = 0;
  for (int i = 0; i < N_LIGHTS; i++) {
    const LightDef &d = LIGHTS[i];
    if (d.room != LUI.room) continue;
    int x = 16 + (k % 2) * 232, y = 8 + (k / 2) * 104;
    k++;
    LTile t{&d};
    t.card = card(LUI.grid, x, y, 216, 96, true);
    t.chip = chip(t.card, 14, 14, 48, d.icon, F.i24, C_TEXT2, C_SURF2);
    bool tunable = d.caps != 0;
    text(t.card, T(d.en, d.ru, d.uk), F.t18, C_TEXT, 74, 15, 128);
    t.status = text(t.card, "", F.t16, C_TEXT2, 74, 42, tunable ? 92 : 128);
    lv_obj_t *track = box(t.card, 14, 78, 188, 4, C_SURF2, 2);
    t.bar = box(track, 0, 0, 0, 4, C_LAMP, 2);
    if (tunable) {
      lv_obj_t *tune = chip(t.card, 170, 36, 34, ic::TUNE, F.i20, C_TEXT2, C_SURF2);
      pressable(tune);
      on_click(tune, [&d]() { open_light(d); });
      reg(tune, std::string("light.tune.") + d.ent);
    }
    on_click(t.card, [&d]() { light_power(d, !light_on(d)); g_dirty = true; });
    reg(t.card, std::string("light.") + d.ent);
    LUI.tiles.push_back(t);
  }
  g_dirty = true;
}

inline void lights_refresh() {
  for (auto &t : LUI.tiles) {
    bool v = light_on(*t.d);
    uint32_t c = light_color(*t.d);
    anim_color(t.chip, v ? c : C_SURF2);
    lv_obj_set_style_text_color(chip_icon(t.chip), col(v ? C_DARK : C_TEXT2), 0);
    int pct = light_pct(*t.d);
    std::string s;
    if (!v) s = T("Off", "Выключен", "Вимкнено");
    else if (t.d->caps & CAP_BRI) s = std::to_string(pct) + "%";
    else s = T("On", "Включён", "Увімкнено");
    set_text(t.status, s);
    lv_obj_set_style_bg_color(t.bar, col(c), 0);
    lv_obj_set_width(t.bar, v ? (t.d->caps & CAP_BRI ? 188 * (pct < 3 ? 3 : pct) / 100 : 188) : 0);
  }
}

inline void lights_ensure() {
  if (scr_lights) return;
  scr_lights = screen();
  header(scr_lights, T("Lights", "Свет", "Світло"), nullptr);
  LUI.seg = seg(scr_lights, 236, 20, 220, 40, {T(ROOM0_EN, ROOM0_RU, ROOM0_UK), T(ROOM1_EN, ROOM1_RU, ROOM1_UK)}, LUI.room,
      [](int i) { LUI.room = i; lights_grid(); }, "lights.room");
  LUI.grid = plain(scr_lights, 0, 76, 480, 330);
  lights_grid();
}

// ───── light detail ─────
inline void open_light(const LightDef &d) {
  open_detail(TAB_LIGHTS, [&d](lv_obj_t *s) {
    header(s, T(d.en, d.ru, d.uk), []() { go(TAB_LIGHTS); });
    auto *pw = toggle(s, 400, 22, light_on(d), [&d](bool v) { light_power(d, v); }, "light.power");
    int y = 84;
    lv_obj_t *val = nullptr, *sl = nullptr;
    if (d.caps & CAP_BRI) {
      caption(s, T("Brightness", "Яркость", "Яскравість"), 24, y + 4);
      val = text(s, "", F.t24, C_TEXT, 336, y, 120, LV_TEXT_ALIGN_RIGHT);
      sl = fill_slider(s, 24, y + 36, 432, 56, 1, 100, light_pct(d) ? light_pct(d) : 60, light_color(d));
      set_text(val, std::to_string(lv_slider_get_value(sl)) + "%");
      on_ev(sl, LV_EVENT_VALUE_CHANGED, [val, sl](lv_event_t *) { set_text(val, std::to_string(lv_slider_get_value(sl)) + "%"); });
      on_ev(sl, LV_EVENT_RELEASED, [&d, sl](lv_event_t *) {
        int v = lv_slider_get_value(sl);
        light_set_optimistic(d, true);
        for (auto &e : light_ents(d)) set_state(e + "|b", std::to_string(v * 255 / 100));
        call("light.turn_on", J().raw("entity_id", light_target(d)).n("brightness_pct", v).str());
      });
      reg(sl, "light.bri");
      y += 110;
    }
    if (d.caps & CAP_CT) {
      caption(s, T("White", "Белый свет", "Біле світло"), 24, y);
      struct W { int k; uint32_t dot; const char *en, *ru, *uk; };
      static const W ws[3] = {{2700, 0xFFB45A, "Warm", "Тёплый", "Теплий"}, {4000, 0xFFE6C2, "Daylight", "Дневной", "Денне"},
                              {6000, 0xCFE3FF, "Cool", "Холодный", "Холодне"}};
      for (int i = 0; i < 3; i++) {
        lv_obj_t *b = box(s, 24 + i * 148, y + 28, 136, 52, C_SURF, 16);
        pressable(b);
        box(b, 14, 16, 20, 20, ws[i].dot, 10);
        text(b, T(ws[i].en, ws[i].ru, ws[i].uk), F.t16, C_TEXT, 42, 15, 90);
        int k = ws[i].k;
        on_click(b, [&d, k, b]() {
          pulse(b);
          light_set_optimistic(d, true);
          call("light.turn_on", J().raw("entity_id", light_target(d)).n("color_temp_kelvin", k).str());
        });
        reg(b, "light.ct." + std::to_string(i));
      }
      y += 96;
    }
    if (d.caps & CAP_RGB) {
      caption(s, T("Colour", "Цвет", "Колір"), 24, y);
      static const uint32_t cs[8] = {0xFF3B30, 0xFF9500, 0xFFD60A, 0x34C759, 0x32D2F0, 0x0A84FF, 0xAF52DE, 0xFF6FAE};
      for (int i = 0; i < 8; i++) {
        lv_obj_t *sw = box(s, 24 + i * 54, y + 28, 44, 44, cs[i], LV_RADIUS_CIRCLE);
        pressable(sw);
        uint32_t c = cs[i];
        on_click(sw, [&d, c, sw]() {
          pulse(sw);
          light_set_optimistic(d, true);
          char rgb[32];
          snprintf(rgb, sizeof rgb, "[%u,%u,%u]", (unsigned) (c >> 16), (unsigned) ((c >> 8) & 255), (unsigned) (c & 255));
          for (auto &e : light_ents(d)) set_state(e + "|rgb", rgb);
          call("light.turn_on", J().raw("entity_id", light_target(d)).raw("rgb_color", rgb).str());
        });
        reg(sw, "light.rgb." + std::to_string(i));
      }
    }
    detail_refresh = [&d, pw, sl]() {
      bool v = light_on(d);
      if (pw->on != v) toggle_set(pw, v);
      if (sl) {
        lv_obj_set_style_bg_color(sl, col(light_color(d)), LV_PART_INDICATOR);
        if (!lv_obj_has_state(sl, LV_STATE_PRESSED) && v && light_pct(d) > 0 && lv_slider_get_value(sl) != light_pct(d)) {
          lv_slider_set_value(sl, light_pct(d), LV_ANIM_ON);
          lv_obj_send_event(sl, LV_EVENT_VALUE_CHANGED, nullptr);
        }
      }
    };
  });
}

}  // namespace rc




