#pragma once
// Media model shared by the home "now playing" bar and the Music tab.
#include "rc_nav.h"

namespace rc {

void music_art_ready();
void music_art_reset();

struct Speaker { const char *ent; uint32_t icon; const char *en, *ru, *uk; };
inline const Speaker SPEAKERS[] = {RC_SPEAKERS};
constexpr int N_SPEAKERS = sizeof(SPEAKERS) / sizeof(SPEAKERS[0]);
// SPOTIFY_ENT comes from rc_config.h; empty when there is no Spotify Connect fallback.
inline int g_speaker = 0;  // the first speaker in rc_home.yaml is selected by default

inline std::string mk(const char *ent, const char *attr) { return std::string(ent) + "|" + attr; }
inline bool media_active(const char *ent) {
  const auto &s = st(ent);
  return (s == "playing" || s == "paused" || s == "buffering") && !st(mk(ent, "title")).empty();
}
// Which player the UI shows: the chosen speaker if it has something loaded,
// otherwise any speaker that is playing, otherwise Spotify Connect, otherwise the chosen one.
inline const char *shown_player() {
  const char *sel = SPEAKERS[g_speaker].ent;
  if (media_active(sel)) return sel;
  for (auto &sp : SPEAKERS)
    if (st(sp.ent) == "playing" && media_active(sp.ent)) return sp.ent;
  if (st(SPOTIFY_ENT) == "playing") return SPOTIFY_ENT;
  return sel;
}
inline const char *speaker_name(const char *ent) {
  for (auto &sp : SPEAKERS)
    if (strcmp(sp.ent, ent) == 0) return T(sp.en, sp.ru, sp.uk);
  return "Spotify";
}
inline bool is_playing(const char *ent) { return st(ent) == "playing"; }

// Local position ticking between HA updates.
inline float g_pos = 0;
inline std::string g_pos_key;
inline void media_tick(float dt) {
  const char *p = shown_player();
  std::string key = std::string(p) + st(mk(p, "title")) + st(mk(p, "pos"));
  if (key != g_pos_key) { g_pos_key = key; g_pos = stf(mk(p, "pos"), 0); }
  else if (is_playing(p)) g_pos += dt;
}

inline void media_cmd(const char *svc) {
  call(std::string("media_player.") + svc, J().k("entity_id", shown_player()).str());
}
inline void media_toggle() {
  const char *p = shown_player();
  if (is_playing(p)) { set_state(p, "paused"); media_cmd("media_pause"); }
  else { set_state(p, "playing"); media_cmd("media_play"); }
}

// Album art: download is done by the firmware's online_image component.
inline std::string g_art_url;
inline void art_request() {
  const char *p = shown_player();
  std::string url = st(mk(p, "pic"));
  if (!url.empty() && url[0] == '/') url = H.ha_url + url;
  if (url == g_art_url) return;
  g_art_url = url;
  music_art_reset();
  if (!url.empty() && H.set_art) H.set_art(url);
}
}  // namespace rc

