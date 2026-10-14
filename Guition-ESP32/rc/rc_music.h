#pragma once
// Music tab: player, search, library, speaker picker (Music Assistant backed).
#include "rc_devices.h"

namespace rc {

struct Item { std::string t, s, u, k; };
struct MusicUi {
  lv_obj_t *views[3]{};
  Seg *tabs = nullptr;
  lv_obj_t *spk_name = nullptr, *spk_chip = nullptr;
  // player
  lv_obj_t *art_box = nullptr, *art_img = nullptr, *art_ph = nullptr, *title = nullptr, *artist = nullptr, *source = nullptr;
  lv_obj_t *prog = nullptr, *t_pos = nullptr, *t_dur = nullptr, *play = nullptr, *vol = nullptr;
  // search
  lv_obj_t *ta = nullptr, *kb = nullptr, *s_status = nullptr, *s_list = nullptr;
  // library
  lv_obj_t *l_status = nullptr, *l_list = nullptr;
  lv_obj_t *sheet = nullptr;
  uint32_t lib_loaded_ms = 0;
};
inline MusicUi MU;

inline void music_play_item(const Item &it) {
  const char *p = SPEAKERS[g_speaker].ent;
  call("music_assistant.play_media", J().k("entity_id", p).k("media_id", it.u).k("media_type", it.k).str());
  set_state(p, "playing");
  set_state(mk(p, "title"), it.t);
  set_state(mk(p, "artist"), it.s);
  seg_select(MU.tabs, 0);
  for (int i = 0; i < 3; i++) (i == 0 ? lv_obj_remove_flag : lv_obj_add_flag)(MU.views[i], LV_OBJ_FLAG_HIDDEN);
  g_dirty = true;
}

inline uint32_t kind_icon(const std::string &k) {
  if (k == "artist") return ic::ARTIST;
  if (k == "album") return ic::ALBUM;
  if (k == "playlist") return ic::PLAYLIST;
  return ic::NOTE;
}
inline uint32_t kind_color(const std::string &k) {
  if (k == "artist") return C_ROSE;
  if (k == "album") return C_PURPLE;
  if (k == "playlist") return C_TEAL;
  return C_GREEN;
}
inline const char *kind_name(const std::string &k) {
  if (k == "artist") return T("Artist", "Исполнитель", "Виконавець");
  if (k == "album") return T("Album", "Альбом", "Альбом");
  if (k == "playlist") return T("Playlist", "Плейлист", "Плейлист");
  return T("Song", "Трек", "Трек");
}
inline void fill_list(lv_obj_t *list, const std::vector<Item> &items, int max_rows, const std::string &regp) {
  lv_obj_clean(list);
  int n = (int) items.size() < max_rows ? (int) items.size() : max_rows;
  for (int i = 0; i < n; i++) {
    auto *it = new Item(items[i]);
    lv_obj_t *row = box(list, 0, i * 54, 432, 50, C_SURF, 14);
    pressable(row);
    chip(row, 6, 5, 40, kind_icon(it->k), F.i20, C_DARK, kind_color(it->k));
    text(row, it->t, F.t16, C_TEXT, 56, 5, 360);
    std::string sub = kind_name(it->k);
    if (!it->s.empty()) sub += "  ·  " + it->s;
    text(row, sub, F.t13, C_TEXT2, 56, 27, 360);
    on_click(row, [it, row]() { pulse(row); music_play_item(*it); });
    on_ev(row, LV_EVENT_DELETE, [it](lv_event_t *) { delete it; });
    reg(row, regp + std::to_string(i));
  }
}

// Called from YAML with parsed Music Assistant responses.
inline void search_result(const std::vector<Item> &items) {
  if (!MU.s_list) return;
  set_text(MU.s_status, items.empty() ? T("Nothing found", "Ничего не нашлось", "Нічого не знайдено") : "");
  fill_list(MU.s_list, items, 4, "music.result.");
}
inline void search_error(const std::string &err) {
  if (MU.s_status) set_text(MU.s_status, std::string(T("Search failed: ", "Поиск не удался: ", "Пошук не вдався: ")) + err);
}
inline void library_result(const std::vector<Item> &items) {
  if (!MU.l_list) return;
  MU.lib_loaded_ms = lv_tick_get();
  set_text(MU.l_status, items.empty() ? T("Library is empty", "Медиатека пуста", "Медіатека порожня") : "");
  fill_list(MU.l_list, items, 5, "music.lib.");
}
inline void library_error(const std::string &err) {
  if (MU.l_status) set_text(MU.l_status, std::string(T("Could not load: ", "Не удалось загрузить: ", "Не вдалося завантажити: ")) + err);
}

// ───── on-screen keyboard (Cyrillic / Latin / digits) ─────
inline const char *KB_BS() { static std::string s = U(0xF006E); return s.c_str(); }
inline const char *KB_GO() { static std::string s = U(0xF0349); return s.c_str(); }
inline void kb_maps(lv_obj_t *kb) {
  constexpr lv_buttonmatrix_ctrl_t K1 = LV_BUTTONMATRIX_CTRL_WIDTH_1, K2 = LV_BUTTONMATRIX_CTRL_WIDTH_2, K6 = LV_BUTTONMATRIX_CTRL_WIDTH_6;
  static std::string bs = U(0xF006E), go = U(0xF0349);
  static const char *ru[] = {"й", "ц", "у", "к", "е", "н", "г", "ш", "щ", "з", "х", "\n",
                             "ф", "ы", "в", "а", "п", "р", "о", "л", "д", "ж", "э", "\n",
                             "я", "ч", "с", "м", "и", "т", "ь", "б", "ю", bs.c_str(), "\n",
                             "ABC", "123", " ", go.c_str(), ""};
  static const char *uk[] = {"й", "ц", "у", "к", "е", "н", "г", "ш", "щ", "з", "х", "ї", "\n",
                             "ф", "і", "в", "а", "п", "р", "о", "л", "д", "ж", "є", "\n",
                             "я", "ч", "с", "м", "и", "т", "ь", "б", "ю", "ґ", bs.c_str(), "\n",
                             "ABC", "123", " ", go.c_str(), ""};
  static const char *en[] = {"q", "w", "e", "r", "t", "y", "u", "i", "o", "p", "\n",
                             "a", "s", "d", "f", "g", "h", "j", "k", "l", "\n",
                             "z", "x", "c", "v", "b", "n", "m", "'", bs.c_str(), "\n",
                             "АБВ", "123", " ", go.c_str(), ""};
  static const char *num[] = {"1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "\n",
                              "-", "&", ".", ",", "!", "?", "(", ")", "'", "\n",
                              "#", "/", ":", "+", "=", "\"", bs.c_str(), "\n",
                              "АБВ", "ABC", " ", go.c_str(), ""};
  static const lv_buttonmatrix_ctrl_t c_ru[] = {K1, K1, K1, K1, K1, K1, K1, K1, K1, K1, K1, K1, K1, K1, K1, K1, K1, K1, K1, K1, K1, K1,
                                                K1, K1, K1, K1, K1, K1, K1, K1, K1, K2, K2, K2, K6, K2};
  static const lv_buttonmatrix_ctrl_t c_uk[] = {K1, K1, K1, K1, K1, K1, K1, K1, K1, K1, K1, K1, K1, K1, K1, K1, K1, K1, K1, K1, K1, K1, K1,
                                                K1, K1, K1, K1, K1, K1, K1, K1, K1, K1, K2, K2, K2, K6, K2};
  static const lv_buttonmatrix_ctrl_t c_en[] = {K1, K1, K1, K1, K1, K1, K1, K1, K1, K1, K1, K1, K1, K1, K1, K1, K1, K1, K1,
                                                K1, K1, K1, K1, K1, K1, K1, K1, K2, K2, K2, K6, K2};
  static const lv_buttonmatrix_ctrl_t c_num[] = {K1, K1, K1, K1, K1, K1, K1, K1, K1, K1, K1, K1, K1, K1, K1, K1, K1, K1, K1,
                                                 K1, K1, K1, K1, K1, K1, K2, K2, K2, K6, K2};
  lv_keyboard_set_map(kb, LV_KEYBOARD_MODE_USER_1, lang() == UK ? uk : ru, lang() == UK ? c_uk : c_ru);
  lv_keyboard_set_map(kb, LV_KEYBOARD_MODE_USER_2, en, c_en);
  lv_keyboard_set_map(kb, LV_KEYBOARD_MODE_USER_3, num, c_num);
  lv_keyboard_set_mode(kb, lang() == EN ? LV_KEYBOARD_MODE_USER_2 : LV_KEYBOARD_MODE_USER_1);
}
inline void do_search() {
  std::string q = lv_textarea_get_text(MU.ta);
  lv_obj_add_flag(MU.kb, LV_OBJ_FLAG_HIDDEN);
  if (q.empty()) return;
  set_text(MU.s_status, T("Searching…", "Ищу…", "Шукаю…"));
  lv_obj_clean(MU.s_list);
  if (H.search) H.search(q);
}

// ───── speaker picker sheet ─────
inline void sheet_close() {
  if (MU.sheet) { lv_obj_delete(MU.sheet); MU.sheet = nullptr; }
}
inline void choose_speaker(int i) {
  // If music is playing elsewhere, move the queue to the chosen speaker.
  const char *src = nullptr;
  for (auto &sp : SPEAKERS) if (is_playing(sp.ent)) { src = sp.ent; break; }
  if (src && strcmp(src, SPEAKERS[i].ent) != 0)
    call("music_assistant.transfer_queue", J().k("entity_id", SPEAKERS[i].ent).k("source_player", src).str());
  g_speaker = i;
  sheet_close();
  g_dirty = true;
}
inline void sheet_open() {
  if (MU.sheet) { sheet_close(); return; }
  MU.sheet = plain(scr_music, 0, 0, 480, 404);
  lv_obj_add_flag(MU.sheet, LV_OBJ_FLAG_CLICKABLE);
  on_click(MU.sheet, []() { sheet_close(); });
  lv_obj_t *c = box(MU.sheet, 200, 68, 264, 60 + N_SPEAKERS * 58, C_SURF2, 20);
  lv_obj_set_style_border_width(c, 1, 0);
  lv_obj_set_style_border_color(c, col(C_SURF3), 0);
  lv_obj_add_flag(c, LV_OBJ_FLAG_CLICKABLE);  // swallow taps
  text(c, T("Play on", "Где играть", "Де грати"), F.t18, C_TEXT, 16, 14);
  for (int i = 0; i < N_SPEAKERS; i++) {
    lv_obj_t *r = box(c, 8, 48 + i * 58, 248, 52, i == g_speaker ? C_ACCENT_BG : C_SURF2, 14);
    pressable(r);
    chip(r, 6, 6, 40, SPEAKERS[i].icon, F.i20, i == g_speaker ? C_ACCENT : C_TEXT2, C_SURF3);
    text(r, T(SPEAKERS[i].en, SPEAKERS[i].ru, SPEAKERS[i].uk), F.t16, C_TEXT, 56, 6, 160);
    const auto &s = st(SPEAKERS[i].ent);
    std::string sub = s == "playing" ? T("Playing", "Играет", "Грає") : (s == "paused" ? T("Paused", "Пауза", "Пауза") : (avail(SPEAKERS[i].ent) ? T("Free", "Свободна", "Вільна") : T("Offline", "Не в сети", "Не в мережі")));
    text(r, sub, F.t13, C_TEXT2, 56, 28, 160);
    if (i == g_speaker) { lv_obj_t *chk = icon(r, ic::CHECK, F.i24, C_ACCENT); lv_obj_set_pos(chk, 212, 12); }
    on_click(r, [i]() { choose_speaker(i); });
    reg(r, "music.speaker." + std::to_string(i));
  }
}

inline std::string mmss(float s) {
  if (std::isnan(s) || s < 0) s = 0;
  int v = (int) s;
  return fmt("%d:%02d", v / 60, v % 60);
}

inline void music_show_view(int i) {
  if (!MU.views[0]) return;
  for (int k = 0; k < 3; k++) (k == i ? lv_obj_remove_flag : lv_obj_add_flag)(MU.views[k], LV_OBJ_FLAG_HIDDEN);
  if (i == 2 && (MU.lib_loaded_ms == 0 || lv_tick_elaps(MU.lib_loaded_ms) > 300000)) {
    set_text(MU.l_status, T("Loading…", "Загружаю…", "Завантажую…"));
    if (H.library) H.library();
  }
  if (i != 1) lv_obj_add_flag(MU.kb, LV_OBJ_FLAG_HIDDEN);
}

inline void music_ensure() {
  if (scr_music) return;
  scr_music = screen();
  lv_obj_t *s = scr_music;
  header(s, T("Music", "Музыка", "Музика"), nullptr);
  // speaker chip
  lv_obj_t *sp = box(s, 236, 18, 220, 44, C_SURF, 22);
  pressable(sp);
  MU.spk_chip = chip(sp, 4, 4, 36, ic::SPEAKER, F.i20, C_ACCENT, C_ACCENT_BG);
  MU.spk_name = text(sp, "", F.t16, C_TEXT, 48, 11, 136);
  lv_obj_t *dn = icon(sp, ic::CHEVRON_DOWN, F.i20, C_TEXT2);
  lv_obj_set_pos(dn, 190, 11);
  on_click(sp, []() { sheet_open(); });
  reg(sp, "music.speaker");
  MU.tabs = seg(s, 24, 72, 432, 40, {T("Player", "Плеер", "Плеєр"), T("Search", "Поиск", "Пошук"), T("Library", "Медиатека", "Медіатека")}, 0,
                [](int i) { music_show_view(i); }, "music.tab");
  for (int i = 0; i < 3; i++) MU.views[i] = plain(s, 0, 120, 480, 284);

  // ── player
  lv_obj_t *v = MU.views[0];
  MU.art_box = box(v, 24, 8, 160, 160, C_SURF2, 18);
  lv_obj_set_style_clip_corner(MU.art_box, true, 0);
  MU.art_ph = icon(MU.art_box, ic::MUSIC, F.i40, C_TEXT3);
  lv_obj_center(MU.art_ph);
  MU.art_img = lv_image_create(MU.art_box);
  lv_obj_set_pos(MU.art_img, 0, 0);
  lv_obj_add_flag(MU.art_img, LV_OBJ_FLAG_HIDDEN);
  MU.source = text(v, "", F.t13, C_ACCENT, 200, 10, 256);
  MU.title = text(v, "", F.t24, C_TEXT, 200, 30, 256);
  lv_label_set_long_mode(MU.title, LV_LABEL_LONG_MODE_WRAP);
  lv_obj_set_height(MU.title, LV_SIZE_CONTENT);
  lv_obj_set_style_max_height(MU.title, 64, 0);
  MU.artist = text(v, "", F.t18, C_TEXT2, 200, 100, 256);
  icon(v, ic::VOL_LO, F.i20, C_TEXT2);
  lv_obj_set_pos(lv_obj_get_child(v, lv_obj_get_child_count(v) - 1), 200, 140);
  MU.vol = slim_slider(v, 232, 150, 188, 0, 100, 30);
  lv_obj_t *vh = icon(v, ic::VOL_HI, F.i20, C_TEXT2);
  lv_obj_set_pos(vh, 432, 140);
  on_ev(MU.vol, LV_EVENT_RELEASED, [](lv_event_t *) {
    const char *p = shown_player();
    float lvl = lv_slider_get_value(MU.vol) / 100.f;
    set_state(mk(p, "vol"), fmt("%.2f", lvl));
    call("media_player.volume_set", J().k("entity_id", p).n("volume_level", lvl).str());
  });
  reg(MU.vol, "music.volume");
  lv_obj_t *track = box(v, 24, 188, 432, 6, C_SURF2, 3);
  MU.prog = box(track, 0, 0, 0, 6, C_TEXT, 3);
  MU.t_pos = text(v, "0:00", F.t13, C_TEXT2, 24, 200);
  MU.t_dur = text(v, "0:00", F.t13, C_TEXT2, 356, 200, 100, LV_TEXT_ALIGN_RIGHT);
  lv_obj_t *prev = chip(v, 136, 222, 52, ic::PREV, F.i28, C_TEXT, C_SURF);
  MU.play = chip(v, 208, 216, 64, ic::PLAY, F.i40, C_DARK, 0xFFFFFF);
  lv_obj_t *next = chip(v, 292, 222, 52, ic::NEXT, F.i28, C_TEXT, C_SURF);
  for (lv_obj_t *b : {prev, MU.play, next}) pressable(b);
  on_click(prev, [prev]() { pulse(prev); media_cmd("media_previous_track"); });
  on_click(next, [next]() { pulse(next); media_cmd("media_next_track"); });
  on_click(MU.play, []() { pulse(MU.play); media_toggle(); g_dirty = true; });
  reg(prev, "music.prev");
  reg(next, "music.next");
  reg(MU.play, "music.play");

  // ── search
  v = MU.views[1];
  MU.ta = lv_textarea_create(v);
  lv_obj_remove_style_all(MU.ta);
  lv_obj_set_pos(MU.ta, 24, 8);
  lv_obj_set_size(MU.ta, 336, 48);
  lv_textarea_set_one_line(MU.ta, true);
  lv_textarea_set_placeholder_text(MU.ta, T("Song, artist or playlist", "Трек, артист или плейлист", "Трек, артист або плейлист"));
  lv_obj_set_style_bg_color(MU.ta, col(C_SURF2), 0);
  lv_obj_set_style_bg_opa(MU.ta, LV_OPA_COVER, 0);
  lv_obj_set_style_radius(MU.ta, 16, 0);
  lv_obj_set_style_pad_hor(MU.ta, 16, 0);
  lv_obj_set_style_pad_top(MU.ta, 12, 0);
  lv_obj_set_style_text_font(MU.ta, F.t18, 0);
  lv_obj_set_style_text_color(MU.ta, col(C_TEXT), 0);
  lv_obj_set_style_text_color(MU.ta, col(C_TEXT3), LV_PART_TEXTAREA_PLACEHOLDER);
  lv_obj_set_style_bg_color(MU.ta, col(C_ACCENT), LV_PART_CURSOR);
  lv_obj_set_style_bg_opa(MU.ta, LV_OPA_COVER, LV_PART_CURSOR);
  lv_obj_set_style_width(MU.ta, 2, LV_PART_CURSOR);
  on_ev(MU.ta, LV_EVENT_CLICKED, [](lv_event_t *) { lv_obj_remove_flag(MU.kb, LV_OBJ_FLAG_HIDDEN); });
  reg(MU.ta, "music.search.field");
  lv_obj_t *go_b = pill(v, 368, 8, 88, 48, "", PRIMARY, ic::SEARCH);
  on_click(go_b, []() { do_search(); });
  reg(go_b, "music.search.go");
  MU.s_status = text(v, T("Find anything on Spotify", "Найдите что угодно в Spotify", "Знайдіть будь-що у Spotify"), F.t16, C_TEXT2, 24, 70, 432);
  MU.s_list = plain(v, 24, 66, 432, 216);
  MU.kb = lv_keyboard_create(v);
  lv_obj_remove_style_all(MU.kb);
  lv_obj_set_pos(MU.kb, 8, 64);
  lv_obj_set_size(MU.kb, 464, 218);
  lv_obj_set_style_bg_color(MU.kb, col(C_SURF), 0);
  lv_obj_set_style_bg_opa(MU.kb, LV_OPA_COVER, 0);
  lv_obj_set_style_radius(MU.kb, 18, 0);
  lv_obj_set_style_pad_all(MU.kb, 6, 0);
  lv_obj_set_style_pad_gap(MU.kb, 5, 0);
  lv_obj_set_style_bg_color(MU.kb, col(C_SURF3), LV_PART_ITEMS);
  lv_obj_set_style_bg_opa(MU.kb, LV_OPA_COVER, LV_PART_ITEMS);
  lv_obj_set_style_bg_color(MU.kb, col(C_ACCENT), LV_PART_ITEMS | LV_STATE_PRESSED);
  lv_obj_set_style_radius(MU.kb, 9, LV_PART_ITEMS);
  lv_obj_set_style_text_color(MU.kb, col(C_TEXT), LV_PART_ITEMS);
  lv_obj_set_style_text_font(MU.kb, F.kb, LV_PART_ITEMS);
  lv_keyboard_set_textarea(MU.kb, MU.ta);
  lv_obj_remove_event_cb(MU.kb, lv_keyboard_def_event_cb);
  kb_maps(MU.kb);
  on_ev(MU.kb, LV_EVENT_VALUE_CHANGED, [](lv_event_t *) {
    uint32_t id = lv_buttonmatrix_get_selected_button(MU.kb);
    if (id == LV_BUTTONMATRIX_BUTTON_NONE) return;
    const char *t = lv_buttonmatrix_get_button_text(MU.kb, id);
    if (!t) return;
    if (strcmp(t, KB_BS()) == 0) lv_textarea_delete_char(MU.ta);
    else if (strcmp(t, KB_GO()) == 0) do_search();
    else if (strcmp(t, "ABC") == 0) lv_keyboard_set_mode(MU.kb, LV_KEYBOARD_MODE_USER_2);
    else if (strcmp(t, "АБВ") == 0) lv_keyboard_set_mode(MU.kb, LV_KEYBOARD_MODE_USER_1);
    else if (strcmp(t, "123") == 0) lv_keyboard_set_mode(MU.kb, LV_KEYBOARD_MODE_USER_3);
    else lv_textarea_add_text(MU.ta, t);
  });
  lv_obj_add_flag(MU.kb, LV_OBJ_FLAG_HIDDEN);

  // ── library
  v = MU.views[2];
  MU.l_status = text(v, "", F.t16, C_TEXT2, 24, 12, 432);
  MU.l_list = plain(v, 24, 8, 432, 276);

  music_show_view(0);
}

inline void music_on_show() { g_dirty = true; }

inline void music_art_reset() {
  if (!MU.art_img) return;
  lv_obj_add_flag(MU.art_img, LV_OBJ_FLAG_HIDDEN);
  lv_obj_remove_flag(MU.art_ph, LV_OBJ_FLAG_HIDDEN);
}

inline void music_art_ready() {
  if (!MU.art_img || !H.get_art) return;
  lv_image_dsc_t *dsc = H.get_art();  // refreshed after every download
  if (!dsc || !dsc->data) return;
  lv_image_set_src(MU.art_img, dsc);
  lv_obj_invalidate(MU.art_img);
  lv_obj_remove_flag(MU.art_img, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(MU.art_ph, LV_OBJ_FLAG_HIDDEN);
}

inline void music_refresh() {
  if (!scr_music) return;
  const char *p = shown_player();
  set_text(MU.spk_name, T(SPEAKERS[g_speaker].en, SPEAKERS[g_speaker].ru, SPEAKERS[g_speaker].uk));
  set_icon(MU.spk_chip, SPEAKERS[g_speaker].icon);
  bool act = media_active(p);
  set_text(MU.title, act ? st(mk(p, "title")) : T("Nothing playing", "Ничего не играет", "Нічого не грає"));
  lv_obj_update_layout(MU.title);
  lv_obj_set_y(MU.artist, lv_obj_get_y(MU.title) + lv_obj_get_height(MU.title) + 4);
  set_text(MU.artist, act ? st(mk(p, "artist")) : T("Search or open the library", "Найдите трек или откройте медиатеку", "Знайдіть трек або відкрийте медіатеку"));
  std::string src = act ? (std::string(is_playing(p) ? T("Playing on ", "Играет: ", "Грає: ") : T("Paused on ", "Пауза: ", "Пауза: ")) + speaker_name(p)) : "";
  set_text(MU.source, src);
  set_icon(MU.play, is_playing(p) ? ic::PAUSE : ic::PLAY);
  float dur = stf(mk(p, "dur"), 0);
  float pos = g_pos;
  if (dur > 0 && pos > dur) pos = dur;
  lv_obj_set_width(MU.prog, dur > 0 ? (int) (432 * pos / dur) : 0);
  set_text(MU.t_pos, mmss(pos));
  set_text(MU.t_dur, mmss(dur));
  float vol = stf(mk(p, "vol"), -1);
  if (vol >= 0 && !lv_obj_has_state(MU.vol, LV_STATE_PRESSED)) lv_slider_set_value(MU.vol, (int) lroundf(vol * 100), LV_ANIM_ON);
  if (!act) {
    lv_obj_add_flag(MU.art_img, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(MU.art_ph, LV_OBJ_FLAG_HIDDEN);
    g_art_url.clear();
  } else {
    art_request();
  }
}

}  // namespace rc





