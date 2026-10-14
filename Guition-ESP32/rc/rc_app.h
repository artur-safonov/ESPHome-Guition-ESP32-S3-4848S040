#pragma once
// Entry points used by the firmware YAML.
#include "rc_music.h"

namespace rc {

inline int g_built_lang = -1;

inline void rebuild_all() {
  lv_obj_t *act = lv_screen_active();
  int tab = NAV.sel < 0 ? 0 : NAV.sel;
  bool on_rc = act == scr_lights || act == scr_devices || act == scr_music || act == scr_detail;
  if (on_rc) lv_screen_load(H.home);
  close_detail();
  for (lv_obj_t **s : {&scr_lights, &scr_devices, &scr_music}) {
    if (*s) { lv_obj_delete(*s); *s = nullptr; }
  }
  LUI.grid = nullptr; LUI.tiles.clear();
  DUI.grid = nullptr; for (auto *t : DUI.tiles) delete t; DUI.tiles.clear();
  MU = MusicUi();
  g_art_url.clear();
  nav_build();
  home_build();
  NAV.sel = -1;
  nav_set(on_rc ? tab : 0, false);
  if (on_rc) go(tab);
  g_built_lang = g_lang ? *g_lang : 7;
  g_dirty = true;
}

inline int tab_of(lv_obj_t *s) {
  if (s == H.home) return TAB_HOME;
  if (s == scr_lights) return TAB_LIGHTS;
  if (s == scr_devices) return TAB_DEVICES;
  if (s == scr_music) return TAB_MUSIC;
  if (s == H.settings) return TAB_SETTINGS;
  if (s == scr_detail && scr_detail) return detail_parent;
  return -1;
}

inline void tick(lv_timer_t *) {
  static uint32_t last = 0;
  uint32_t now = lv_tick_get();
  float dt = last ? (now - last) / 1000.f : 0;
  last = now;
  if (g_lang && *g_lang != g_built_lang) rebuild_all();
  lv_obj_t *act = lv_screen_active();
  int t = tab_of(act);
  if (t >= 0) nav_set(t);
  media_tick(dt);
  expire_optimistic();
  bool music_live = act == scr_music && is_playing(shown_player());
  if (!g_dirty && !music_live) return;
  g_dirty = false;
  home_refresh();
  if (act == scr_lights) lights_refresh();
  else if (act == scr_devices) devices_refresh();
  else if (act == scr_music) music_refresh();
  else if (act == scr_detail && scr_detail && detail_refresh) detail_refresh();
}

inline void init() {
  styles_init();
  rebuild_all();
  lv_timer_create(tick, 500, nullptr);
}

// API action "rc_show": home | lights | devices | music | settings | light:<entity> | dev:<n> | music:<0..2> | speakers
inline void show(const std::string &p) {
  if (H.wake) H.wake();  // API/automation calls wake the screen like a touch would
  if (p == "home") go(TAB_HOME);
  else if (p == "lights") go(TAB_LIGHTS);
  else if (p == "devices") go(TAB_DEVICES);
  else if (p == "music") go(TAB_MUSIC);
  else if (p == "settings") go(TAB_SETTINGS);
  else if (p.rfind("lights:", 0) == 0) { go(TAB_LIGHTS); LUI.room = atoi(p.c_str() + 7); seg_select(LUI.seg, LUI.room, false); lights_grid(); }
  else if (p.rfind("devices:", 0) == 0) { go(TAB_DEVICES); DUI.room = atoi(p.c_str() + 8); seg_select(DUI.seg, DUI.room, false); devices_grid(); }
  else if (p.rfind("light:", 0) == 0) {
    for (int i = 0; i < N_LIGHTS; i++) if (p.substr(6) == LIGHTS[i].ent) { go(TAB_LIGHTS); open_light(LIGHTS[i]); }
  } else if (p.rfind("dev:", 0) == 0) { go(TAB_DEVICES); open_device((DevKey) atoi(p.c_str() + 4)); }
  else if (p.rfind("music:", 0) == 0) { go(TAB_MUSIC); int i = atoi(p.c_str() + 6); seg_select(MU.tabs, i, false); music_show_view(i); }
  else if (p == "speakers") { go(TAB_MUSIC); sheet_open(); }
  else if (p.rfind("search:", 0) == 0) {
    go(TAB_MUSIC); seg_select(MU.tabs, 1, false); music_show_view(1);
    lv_textarea_set_text(MU.ta, p.substr(7).c_str()); do_search();
  }
  else if (p == "keyboard") { go(TAB_MUSIC); seg_select(MU.tabs, 1, false); music_show_view(1); lv_obj_remove_flag(MU.kb, LV_OBJ_FLAG_HIDDEN); }
  g_dirty = true;
}

// API action "rc_bench": full redraw time of each tab (ms), logged in one line.
inline void bench() {
  std::string out;
  auto one = [&out](const char *name, lv_obj_t *s) {
    if (!s) return;
    lv_screen_load(s);
    lv_refr_now(nullptr);
    int64_t sum = 0;
    for (int i = 0; i < 3; i++) {
      lv_obj_invalidate(s);
      int64_t t0 = esp_timer_get_time();
      lv_refr_now(nullptr);
      sum += esp_timer_get_time() - t0;
    }
    out += fmt("%s=%d ", name, (int) (sum / 3000));
  };
  lights_ensure(); devices_ensure(); music_ensure();
  one("home", H.home); one("lights", scr_lights); one("devices", scr_devices); one("music", scr_music);
  open_device(D_AC); one("ac", scr_detail);
  open_device(D_VAC); one("vacuum", scr_detail);
  go(TAB_HOME);
  ESP_LOGW("rc", "BENCH %s", out.c_str());
}

}  // namespace rc





