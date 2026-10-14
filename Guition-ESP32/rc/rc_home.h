#pragma once
// Home tab additions: kitchen scenes + now-playing bar (built onto the firmware's home page).
#include "rc_media.h"

namespace rc {

struct SceneDef {
  const char *ent; uint32_t icon, color, fg;
  const char *en, *ru, *uk, *sen, *sru, *suk;
};
inline const SceneDef SCENES[] = {RC_SCENES};

struct HomeUi {
  lv_obj_t *root = nullptr, *np_chip = nullptr, *np_title = nullptr, *np_sub = nullptr, *np_btn = nullptr;
};
inline HomeUi HOME;

inline void scene_feedback(lv_obj_t *chip, uint32_t cp, uint32_t color) {
  pulse(chip);
  set_icon(chip, ic::CHECK);
  anim_color(chip, C_OK);
  struct Back { lv_obj_t *chip; uint32_t cp, color; };
  auto *b = new Back{chip, cp, color};
  lv_timer_t *t = lv_timer_create([](lv_timer_t *tm) {
    auto *b = static_cast<Back *>(lv_timer_get_user_data(tm));
    set_icon(b->chip, b->cp);
    anim_color(b->chip, b->color, false, 400);
    delete b;
    lv_timer_delete(tm);
  }, 2200, b);
  (void) t;
}

inline void home_build() {
  if (!H.home) return;
  if (HOME.root) lv_obj_delete(HOME.root);
  HOME = HomeUi();
  HOME.root = plain(H.home, 0, 182, 480, 222);
  for (int i = 0; i < 6; i++) {
    const SceneDef &d = SCENES[i];
    int x = 20 + (i % 3) * 148, y = (i / 3) * 76;
    lv_obj_t *c = card(HOME.root, x, y, 140, 70, true);
    lv_obj_t *ch = chip(c, 10, 9, 30, d.icon, F.i20, d.fg, d.color);
    text(c, T(d.en, d.ru, d.uk), F.t16, C_TEXT, 46, 12, 90);
    text(c, T(d.sen, d.sru, d.suk), F.t13, C_TEXT2, 10, 47, 122);
    const char *ent = d.ent;
    uint32_t cp = d.icon, color = d.color;
    on_click(c, [ent, ch, cp, color]() {
      scene_feedback(ch, cp, color);
      call("script.turn_on", J().k("entity_id", ent).str());
    });
    reg(c, std::string("scene.") + std::to_string(i));
  }
  lv_obj_t *np = card(HOME.root, 20, 156, 440, 62, true);
  HOME.np_chip = chip(np, 10, 10, 42, ic::MUSIC, F.i24, C_TEXT2, C_SURF2);
  HOME.np_title = text(np, "", F.t18, C_TEXT, 64, 9, 300);
  HOME.np_sub = text(np, "", F.t13, C_TEXT2, 64, 35, 300);
  HOME.np_btn = chip(np, 386, 9, 44, ic::PLAY, F.i24, C_DARK, 0xFFFFFF);
  pressable(HOME.np_btn);
  on_click(np, []() { go(TAB_MUSIC); });
  on_click(HOME.np_btn, []() { pulse(HOME.np_btn); media_toggle(); g_dirty = true; });
  reg(np, "home.np");
  reg(HOME.np_btn, "home.np.play");
}

inline void home_refresh() {
  if (!HOME.root) return;
  const char *p = shown_player();
  bool active = media_active(p);
  bool playing = is_playing(p);
  if (active) {
    set_text(HOME.np_title, st(mk(p, "title")));
    std::string sub = speaker_name(p);
    const auto &artist = st(mk(p, "artist"));
    if (!artist.empty()) sub += "  ·  " + artist;
    set_text(HOME.np_sub, sub);
  } else {
    set_text(HOME.np_title, T("Nothing playing", "Ничего не играет", "Нічого не грає"));
    set_text(HOME.np_sub, std::string(speaker_name(p)) + "  ·  " + T("tap to choose music", "нажмите, чтобы выбрать", "натисніть, щоб обрати"));
  }
  anim_color(HOME.np_chip, playing ? C_GREEN : C_SURF2);
  lv_obj_set_style_text_color(chip_icon(HOME.np_chip), col(playing ? C_DARK : C_TEXT2), 0);
  set_icon(HOME.np_btn, playing ? ic::PAUSE : ic::PLAY);
}

}  // namespace rc



