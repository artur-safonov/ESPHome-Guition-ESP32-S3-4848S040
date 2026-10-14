#pragma once
// Navigation shell: bottom tab bar (always visible), tab switching, detail screens.
#include "rc_core.h"

namespace rc {

enum Tab { TAB_HOME = 0, TAB_LIGHTS, TAB_DEVICES, TAB_MUSIC, TAB_SETTINGS };
inline lv_obj_t *scr_lights = nullptr, *scr_devices = nullptr, *scr_music = nullptr, *scr_detail = nullptr;
inline int detail_parent = TAB_HOME;
inline std::function<void()> detail_refresh;

void lights_ensure();
void devices_ensure();
void music_ensure();
void music_on_show();
void lights_refresh();
void devices_refresh();
void music_refresh();
void home_refresh();

// ───── bottom navigation ─────
struct NavUi { lv_obj_t *ind = nullptr; lv_obj_t *ico[5]{}; lv_obj_t *lbl[5]{}; int sel = -1; };
inline NavUi NAV;
constexpr int NAV_TAB_W = 92;

void go(int tab);

inline void nav_set(int i, bool animate = true) {
  if (!NAV.ind || NAV.sel == i || i < 0 || i > 4) return;
  NAV.sel = i;
  int x = 2 + i * NAV_TAB_W + 8;
  if (animate) anim_x(NAV.ind, x, 220);
  else lv_obj_set_x(NAV.ind, x);
  for (int k = 0; k < 5; k++) {
    anim_color(NAV.ico[k], k == i ? C_ACCENT : C_TEXT2, true, 180);
    lv_obj_set_style_text_color(NAV.lbl[k], col(k == i ? C_TEXT : C_TEXT2), 0);
  }
}

inline void nav_build() {
  lv_obj_t *n = H.nav;
  if (!n) return;
  lv_obj_clean(n);
  lv_obj_remove_style_all(n);
  lv_obj_set_size(n, 464, 68);
  lv_obj_align(n, LV_ALIGN_BOTTOM_MID, 0, -6);
  lv_obj_set_style_bg_color(n, col(C_SURF), 0);
  lv_obj_set_style_bg_opa(n, LV_OPA_COVER, 0);
  lv_obj_set_style_radius(n, 24, 0);
  lv_obj_set_style_border_width(n, 1, 0);
  lv_obj_set_style_border_color(n, col(C_SURF2), 0);
  lv_obj_remove_flag(n, LV_OBJ_FLAG_SCROLLABLE);
  NAV = NavUi();
  NAV.ind = box(n, 10, 6, NAV_TAB_W - 16, 56, C_ACCENT_BG, 18);
  const uint32_t icons[5] = {ic::NAV_HOME, ic::NAV_LIGHT, ic::NAV_DEV, ic::MUSIC, ic::NAV_COG};
  const char *names[5] = {T("Home", "Главная", "Головна"), T("Lights", "Свет", "Світло"),
                          T("Devices", "Устройства", "Пристрої"), T("Music", "Музыка", "Музика"),
                          T("Settings", "Настройки", "Налаштування")};
  for (int i = 0; i < 5; i++) {
    lv_obj_t *hit = plain(n, 2 + i * NAV_TAB_W, 0, NAV_TAB_W, 68);
    NAV.ico[i] = icon(hit, icons[i], F.i24, C_TEXT2);
    lv_obj_align(NAV.ico[i], LV_ALIGN_TOP_MID, 0, 9);
    NAV.lbl[i] = text(hit, names[i], F.t13, C_TEXT2, 0, 0);
    lv_obj_align(NAV.lbl[i], LV_ALIGN_TOP_MID, 0, 41);
    on_click(hit, [i]() { go(i); });
    reg(hit, "nav." + std::to_string(i));
  }
}

inline void close_detail() {
  if (!scr_detail) return;
  lv_obj_t *d = scr_detail;
  scr_detail = nullptr;
  detail_refresh = nullptr;
  if (lv_screen_active() != d) lv_obj_delete(d);
  else lv_obj_delete_async(d);
}

inline void show_nav() {
  if (H.nav && !(H.screensaver && H.screensaver())) lv_obj_remove_flag(H.nav, LV_OBJ_FLAG_HIDDEN);
}

inline void go(int tab) {
  if (H.screensaver && H.screensaver()) return;
  lv_obj_t *target = nullptr;
  switch (tab) {
    case TAB_HOME: target = H.home; break;
    case TAB_LIGHTS: lights_ensure(); target = scr_lights; break;
    case TAB_DEVICES: devices_ensure(); target = scr_devices; break;
    case TAB_MUSIC: music_ensure(); target = scr_music; break;
    case TAB_SETTINGS: target = H.settings; break;
  }
  if (!target) return;
  lv_screen_load(target);
  close_detail();
  if (tab == TAB_MUSIC) music_on_show();
  nav_set(tab);
  show_nav();
  g_dirty = true;
}

// Detail screens are built on demand and destroyed when leaving them.
inline void open_detail(int parent_tab, const std::function<void(lv_obj_t *)> &build) {
  lv_obj_t *old = scr_detail;
  scr_detail = nullptr;
  detail_refresh = nullptr;
  lv_obj_t *s = screen();
  detail_parent = parent_tab;
  build(s);
  lv_screen_load(s);
  if (old) lv_obj_delete(old);
  scr_detail = s;
  show_nav();
  g_dirty = true;
}

// ───── date formatting for RU/UK (genitive month, capitalised weekday) ─────
inline const char *month_gen(int lang_idx, int m) {
  static const char *ru[12] = {"января", "февраля", "марта", "апреля", "мая", "июня", "июля", "августа", "сентября", "октября", "ноября", "декабря"};
  static const char *uk[12] = {"січня", "лютого", "березня", "квітня", "травня", "червня", "липня", "серпня", "вересня", "жовтня", "листопада", "грудня"};
  static const char *en[12] = {"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};
  if (m < 1 || m > 12) m = 1;
  return lang_idx == 6 ? ru[m - 1] : (lang_idx == 9 ? uk[m - 1] : en[m - 1]);
}
inline const char *weekday(int lang_idx, int dow, bool longf) {  // dow: 1 = Sunday
  static const char *ru_l[7] = {"Воскресенье", "Понедельник", "Вторник", "Среда", "Четверг", "Пятница", "Суббота"};
  static const char *uk_l[7] = {"Неділя", "Понеділок", "Вівторок", "Середа", "Четвер", "П'ятниця", "Субота"};
  static const char *en_l[7] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
  static const char *ru_s[7] = {"вс", "пн", "вт", "ср", "чт", "пт", "сб"};
  static const char *uk_s[7] = {"нд", "пн", "вт", "ср", "чт", "пт", "сб"};
  static const char *en_s[7] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
  if (dow < 1 || dow > 7) dow = 1;
  if (lang_idx == 6) return (longf ? ru_l : ru_s)[dow - 1];
  if (lang_idx == 9) return (longf ? uk_l : uk_s)[dow - 1];
  return (longf ? en_l : en_s)[dow - 1];
}
// "Понедельник, 28 сентября" / "Monday, September 28"
inline void long_date(char *buf, size_t n, int lang_idx, int dow, int day, int mon) {
  if (lang_idx == 6 || lang_idx == 9) snprintf(buf, n, "%s, %d %s", weekday(lang_idx, dow, true), day, month_gen(lang_idx, mon));
  else snprintf(buf, n, "%s, %s %d", weekday(lang_idx, dow, true), month_gen(lang_idx, mon), day);
}
// "пн, 28 сентября" / "Mon, September 28"
inline void short_date(char *buf, size_t n, int lang_idx, int dow, int day, int mon) {
  if (lang_idx == 6 || lang_idx == 9) snprintf(buf, n, "%s, %d %s", weekday(lang_idx, dow, false), day, month_gen(lang_idx, mon));
  else snprintf(buf, n, "%s, %s %d", weekday(lang_idx, dow, false), month_gen(lang_idx, mon), day);
}

}  // namespace rc


