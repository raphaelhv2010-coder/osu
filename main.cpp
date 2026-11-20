#include "eadk.h"
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

extern "C" {
const char eadk_app_name[] __attribute__((section(".rodata.eadk_app_name"))) = "osu!mania 4K";
const uint32_t eadk_api_level __attribute__((section(".rodata.eadk_api_level"))) = 0;
}

// --- CONSTANTS & LAYOUT ---
constexpr int SCREEN_W = 320;
constexpr int SCREEN_H = 240;

// 4K Playfield centered horizontally
constexpr int LANE_COUNT = 4;
constexpr int LANE_W = 36;
constexpr int FIELD_W = LANE_COUNT * LANE_W; // 144 px
constexpr int FIELD_X = (SCREEN_W - FIELD_W) / 2; // 88 px
constexpr int HIT_LINE_Y = 202;
constexpr int NOTE_H = 10;

// Timing windows in milliseconds (inspired by osu!mania OD8)
constexpr int32_t WIN_MARVELOUS = 24;
constexpr int32_t WIN_PERFECT   = 52;
constexpr int32_t WIN_GREAT     = 85;
constexpr int32_t WIN_GOOD      = 120;
constexpr int32_t WIN_BAD       = 155;
constexpr int32_t WIN_MISS      = 185;

// Colors (RGB565)
constexpr eadk_color_t COL_BLACK      = 0x0000;
constexpr eadk_color_t COL_BG         = 0x0842; // Deep dark blue-gray
constexpr eadk_color_t COL_FIELD_BG   = 0x10A3; // Dark lane bg
constexpr eadk_color_t COL_LANE_SEP   = 0x2966; // Subtle lane separator
constexpr eadk_color_t COL_BORDER     = 0xF8B2; // osu! pink border
constexpr eadk_color_t COL_PINK       = 0xF8B2; // #FF66AA
constexpr eadk_color_t COL_CYAN       = 0x07FF; // Cyan (outer lanes)
constexpr eadk_color_t COL_WHITE      = 0xFFFF; // White (inner lanes)
constexpr eadk_color_t COL_HOLD_CYAN  = 0x04D3; // Darker cyan for LN body
constexpr eadk_color_t COL_HOLD_PINK  = 0xA18A; // Darker pink for LN body
constexpr eadk_color_t COL_HIT_LINE   = 0xFFE0; // Bright yellow-gold judgement line
constexpr eadk_color_t COL_GOLD       = 0xFEA0;
constexpr eadk_color_t COL_GREEN      = 0x2FE8;
constexpr eadk_color_t COL_BLUE       = 0x3DFD;
constexpr eadk_color_t COL_ORANGE     = 0xFC60;
constexpr eadk_color_t COL_RED        = 0xF986;
constexpr eadk_color_t COL_GRAY       = 0x8410;
constexpr eadk_color_t COL_DARK_PANEL = 0x18E4;

// Keys: 7, 8, (, )
const eadk_key_t LANE_KEYS[LANE_COUNT] = {
  eadk_key_seven,
  eadk_key_eight,
  eadk_key_left_parenthesis,
  eadk_key_right_parenthesis
};

const char* LANE_LABELS[LANE_COUNT] = { "7", "8", "(", ")" };

// --- BEATMAP DATA STRUCTURES ---
struct Note {
  int32_t time_ms;     // Hit time in ms from start
  uint8_t lane;        // 0..3
  int32_t duration_ms; // 0 = regular tap note, >0 = Long Note (LN)
  uint8_t state;       // 0 = active, 1 = holding (LN), 2 = hit/done, 3 = missed
};

struct BeatmapMeta {
  const char* title;
  const char* artist;
  const char* diff_name;
  uint8_t stars;       // 1 to 6
  uint16_t bpm;
  uint16_t scroll_ms;  // Time for note to travel from top to hit line (lower = faster)
  uint32_t seed;
  uint16_t note_count;
};

// 6 Progressive Levels from Beginner (1*) to Demon/Extra (6*)
const BeatmapMeta BEATMAPS[] = {
  { "Tutorial Groove",   "NumWorks Sound",  "1* Beginner",   1, 100, 1150, 10101, 48  },
  { "Nekodex - Circles", "osu! Originals",  "2* Easy",       2, 125, 980,  20202, 76  },
  { "Cyber Pulse 4K",    "Kuro Beats",      "3* Normal",     3, 145, 840,  30303, 115 },
  { "Sakura Rush",       "Rhythm Lab",      "4* Hard",       4, 168, 720,  40404, 165 },
  { "Galaxy Collapse",   "Kurokotei Mix",   "5* Insane",     5, 190, 620,  50505, 230 },
  { "Freedom Dive 4K",   "xi (NumWorks)",   "6* Expert+",    6, 222, 540,  60606, 300 }
};
constexpr int NUM_BEATMAPS = sizeof(BEATMAPS) / sizeof(BEATMAPS[0]);
constexpr int MAX_NOTES = 320;

Note g_notes[MAX_NOTES];
int g_note_count = 0;

// Deterministic PRNG for procedural beatmap patterns (staircases, jacks, jumps, trills, LNs)
static uint32_t rng_state = 1;
static uint32_t next_rand() {
  rng_state = rng_state * 1664525u + 1013904223u;
  return rng_state;
}

void generate_beatmap(int map_idx) {
  const BeatmapMeta& meta = BEATMAPS[map_idx];
  rng_state = meta.seed;
  g_note_count = 0;

  int32_t step_ms = 60000 / meta.bpm; // Quarter note
  if (meta.stars >= 3) step_ms /= 2;  // Eighth notes
  if (meta.stars >= 5) step_ms = (60000 / meta.bpm) / 2;

  int32_t t = 1800; // 1.8s lead-in
  int last_lane = 0;
  int dir = 1;

  while (g_note_count < meta.note_count && g_note_count < MAX_NOTES - 2) {
    uint32_t r = next_rand() % 100;

    if (meta.stars == 1) {
      // Beginner: simple single steps + occasional easy hold
      int lane = (last_lane + ((r < 70) ? 1 : 3)) % 4;
      int32_t dur = (r > 82) ? step_ms : 0;
      g_notes[g_note_count++] = { t, (uint8_t)lane, dur, 0 };
      last_lane = lane;
      t += (dur > 0) ? step_ms * 2 : step_ms;
    } else if (meta.stars == 2) {
      // Easy: staircases + simple holds
      last_lane += dir;
      if (last_lane >= 3) { last_lane = 3; dir = -1; }
      if (last_lane <= 0) { last_lane = 0; dir = 1; }
      int32_t dur = (r > 80) ? step_ms : 0;
      g_notes[g_note_count++] = { t, (uint8_t)last_lane, dur, 0 };
      t += (dur > 0) ? step_ms * 2 : step_ms;
    } else if (meta.stars == 3) {
      // Normal: staircases, occasional 2-note jumps, holds
      if (r < 22 && g_note_count + 1 < MAX_NOTES) {
        // Jump chord
        int l1 = next_rand() % 2;
        int l2 = 2 + (next_rand() % 2);
        g_notes[g_note_count++] = { t, (uint8_t)l1, 0, 0 };
        g_notes[g_note_count++] = { t, (uint8_t)l2, 0, 0 };
        t += step_ms * 2;
      } else {
        int lane = (last_lane + 1 + (next_rand() % 3)) % 4;
        int32_t dur = (r > 85) ? step_ms * 2 : 0;
        g_notes[g_note_count++] = { t, (uint8_t)lane, dur, 0 };
        last_lane = lane;
        t += (dur > 0) ? step_ms * 2 : step_ms;
      }
    } else if (meta.stars == 4) {
      // Hard: trills, jumps, and mixed LNs
      if (r < 30 && g_note_count + 1 < MAX_NOTES) {
        int l1 = next_rand() % 4;
        int l2 = (l1 + 1 + (next_rand() % 3)) % 4;
        g_notes[g_note_count++] = { t, (uint8_t)l1, 0, 0 };
        g_notes[g_note_count++] = { t, (uint8_t)l2, 0, 0 };
        t += step_ms;
      } else if (r < 55 && g_note_count + 3 < MAX_NOTES) {
        // 4-note mini trill
        int a = next_rand() % 2;
        int b = 2 + (next_rand() % 2);
        for (int k = 0; k < 4; k++) {
          g_notes[g_note_count++] = { t + k * (step_ms / 2), (uint8_t)((k % 2 == 0) ? a : b), 0, 0 };
        }
        t += step_ms * 2;
      } else {
        int lane = (last_lane + 1 + (next_rand() % 3)) % 4;
        int32_t dur = (r > 80) ? step_ms : 0;
        g_notes[g_note_count++] = { t, (uint8_t)lane, dur, 0 };
        last_lane = lane;
        t += step_ms;
      }
    } else if (meta.stars == 5) {
      // Insane: dense streams, jumpstreams, jackhammers
      if (r < 35 && g_note_count + 1 < MAX_NOTES) {
        int l1 = next_rand() % 4;
        int l2 = (l1 + 2) % 4;
        g_notes[g_note_count++] = { t, (uint8_t)l1, 0, 0 };
        g_notes[g_note_count++] = { t, (uint8_t)l2, 0, 0 };
        t += step_ms;
      } else if (r < 75 && g_note_count + 3 < MAX_NOTES) {
        // 16th stream burst
        int start = next_rand() % 4;
        int d = (next_rand() % 2 == 0) ? 1 : 3;
        for (int k = 0; k < 4; k++) {
          g_notes[g_note_count++] = { t + k * (step_ms / 2), (uint8_t)((start + k * d) % 4), 0, 0 };
        }
        t += step_ms * 2;
      } else {
        // Mini-jack
        int lane = next_rand() % 4;
        g_notes[g_note_count++] = { t, (uint8_t)lane, 0, 0 };
        if (g_note_count < MAX_NOTES) {
          g_notes[g_note_count++] = { t + step_ms / 2, (uint8_t)lane, 0, 0 };
        }
        t += step_ms;
      }
    } else {
      // 6* Expert+: Freedom Dive style 16th streams + jumpstreams + chordjacks
      if (r < 40 && g_note_count + 2 < MAX_NOTES) {
        int l1 = next_rand() % 4;
        int l2 = (l1 + 1) % 4;
        g_notes[g_note_count++] = { t, (uint8_t)l1, 0, 0 };
        g_notes[g_note_count++] = { t, (uint8_t)l2, 0, 0 };
        t += step_ms / 2;
      } else {
        int lane = (last_lane + 1 + (next_rand() % 3)) % 4;
        g_notes[g_note_count++] = { t, (uint8_t)lane, 0, 0 };
        last_lane = lane;
        t += step_ms / 2;
      }
    }
  }
}

// --- HELPER DRAWING ---
void fill_rect(int x, int y, int w, int h, eadk_color_t c) {
  if (w <= 0 || h <= 0) return;
  if (x < 0) { w += x; x = 0; }
  if (y < 0) { h += y; y = 0; }
  if (x + w > SCREEN_W) w = SCREEN_W - x;
  if (y + h > SCREEN_H) h = SCREEN_H - y;
  if (w <= 0 || h <= 0) return;
  eadk_display_push_rect_uniform({(uint16_t)x, (uint16_t)y, (uint16_t)w, (uint16_t)h}, c);
}

void draw_int(int val, int x, int y, bool large, eadk_color_t fg, eadk_color_t bg) {
  char buf[16];
  int idx = 0;
  if (val < 0) {
    buf[idx++] = '-';
    val = -val;
  }
  if (val == 0) {
    buf[idx++] = '0';
  } else {
    char tmp[16];
    int t = 0;
    while (val > 0 && t < 15) {
      tmp[t++] = '0' + (val % 10);
      val /= 10;
    }
    while (t > 0) buf[idx++] = tmp[--t];
  }
  buf[idx] = '\0';
  eadk_display_draw_string(buf, {(uint16_t)x, (uint16_t)y}, large, fg, bg);
}

// --- GAME STATE ---
enum GameScreen {
  SCREEN_MENU,
  SCREEN_PLAYING,
  SCREEN_RESULTS
};

enum Judgement {
  JUDGE_NONE = 0,
  JUDGE_MARVELOUS,
  JUDGE_PERFECT,
  JUDGE_GREAT,
  JUDGE_GOOD,
  JUDGE_BAD,
  JUDGE_MISS
};

struct PlayStats {
  uint32_t score;
  uint16_t combo;
  uint16_t max_combo;
  uint16_t count_marv;
  uint16_t count_perf;
  uint16_t count_great;
  uint16_t count_good;
  uint16_t count_bad;
  uint16_t count_miss;
  int hp; // 0..100
  Judgement last_judge;
  int32_t last_judge_time;
};

void apply_judgement(PlayStats& st, Judgement j, int32_t now_ms) {
  st.last_judge = j;
  st.last_judge_time = now_ms;

  switch (j) {
    case JUDGE_MARVELOUS:
      st.count_marv++;
      st.combo++;
      st.score += 320 + st.combo * 4;
      st.hp += 3;
      break;
    case JUDGE_PERFECT:
      st.count_perf++;
      st.combo++;
      st.score += 300 + st.combo * 3;
      st.hp += 2;
      break;
    case JUDGE_GREAT:
      st.count_great++;
      st.combo++;
      st.score += 200 + st.combo * 2;
      st.hp += 1;
      break;
    case JUDGE_GOOD:
      st.count_good++;
      st.combo++;
      st.score += 100 + st.combo;
      break;
    case JUDGE_BAD:
      st.count_bad++;
      st.combo = 0;
      st.score += 50;
      st.hp -= 4;
      break;
    case JUDGE_MISS:
      st.count_miss++;
      st.combo = 0;
      st.hp -= 8;
      break;
    default:
      break;
  }
  if (st.combo > st.max_combo) st.max_combo = st.combo;
  if (st.hp > 100) st.hp = 100;
  if (st.hp < 0) st.hp = 0;
}

int compute_accuracy_x100(const PlayStats& st) {
  uint32_t total = st.count_marv + st.count_perf + st.count_great + st.count_good + st.count_bad + st.count_miss;
  if (total == 0) return 10000;
  uint32_t weighted = st.count_marv * 300 + st.count_perf * 300 + st.count_great * 200 + st.count_good * 100 + st.count_bad * 50;
  return (int)((weighted * 10000ULL) / (total * 300ULL));
}

// --- MENU RENDER ---
void draw_menu(int selected_idx) {
  fill_rect(0, 0, SCREEN_W, SCREEN_H, COL_BG);

  // Header Banner
  fill_rect(0, 0, SCREEN_W, 34, COL_DARK_PANEL);
  fill_rect(0, 34, SCREEN_W, 2, COL_PINK);
  eadk_display_draw_string("osu!mania 4K", {12, 8}, true, COL_PINK, COL_DARK_PANEL);
  eadk_display_draw_string("Touches: [7] [8] [(] [)]", {148, 12}, false, COL_CYAN, COL_DARK_PANEL);

  // Beatmap List
  for (int i = 0; i < NUM_BEATMAPS; i++) {
    int y = 44 + i * 28;
    bool sel = (i == selected_idx);
    eadk_color_t row_bg = sel ? 0x2946 : COL_DARK_PANEL;
    fill_rect(12, y, SCREEN_W - 24, 24, row_bg);
    if (sel) {
      fill_rect(12, y, 4, 24, COL_PINK);
    }

    eadk_display_draw_string(BEATMAPS[i].title, {22, (uint16_t)(y + 6)}, false, sel ? COL_WHITE : 0xCE79, row_bg);
    eadk_display_draw_string(BEATMAPS[i].diff_name, {195, (uint16_t)(y + 6)}, false, sel ? COL_GOLD : COL_CYAN, row_bg);
  }

  // Footer
  fill_rect(0, 218, SCREEN_W, 22, COL_DARK_PANEL);
  eadk_display_draw_string("Haut/Bas: Choisir   OK/EXE: Jouer   HOME: Quitter", {14, 223}, false, COL_GRAY, COL_DARK_PANEL);
}

// --- RESULTS RENDER ---
void draw_results(int map_idx, const PlayStats& st, bool failed) {
  fill_rect(0, 0, SCREEN_W, SCREEN_H, COL_BG);

  fill_rect(0, 0, SCREEN_W, 34, COL_DARK_PANEL);
  fill_rect(0, 34, SCREEN_W, 2, failed ? COL_RED : COL_PINK);
  eadk_display_draw_string(failed ? "STAGE FAILED!" : "STAGE CLEAR!", {14, 8}, true, failed ? COL_RED : COL_GOLD, COL_DARK_PANEL);
  eadk_display_draw_string(BEATMAPS[map_idx].diff_name, {195, 11}, false, COL_CYAN, COL_DARK_PANEL);

  int acc = compute_accuracy_x100(st);
  const char* grade = "D";
  eadk_color_t grade_col = COL_RED;
  if (!failed) {
    if (acc == 10000 && st.count_perf == 0) { grade = "SS"; grade_col = COL_GOLD; }
    else if (acc >= 9500) { grade = "S"; grade_col = COL_GOLD; }
    else if (acc >= 9000) { grade = "A"; grade_col = COL_GREEN; }
    else if (acc >= 8000) { grade = "B"; grade_col = COL_BLUE; }
    else if (acc >= 7000) { grade = "C"; grade_col = COL_PINK; }
  } else {
    grade = "F";
  }

  // Grade Box
  fill_rect(225, 52, 75, 75, COL_DARK_PANEL);
  eadk_display_draw_string("RANG", {246, 58}, false, COL_GRAY, COL_DARK_PANEL);
  eadk_display_draw_string(grade, {254, 82}, true, grade_col, COL_DARK_PANEL);

  // Breakdown
  int y = 48;
  eadk_display_draw_string("SCORE:", {18, (uint16_t)y}, false, COL_WHITE, COL_BG);
  draw_int((int)st.score, 95, y, false, COL_GOLD, COL_BG);

  y += 18;
  eadk_display_draw_string("MAX COMBO:", {18, (uint16_t)y}, false, COL_WHITE, COL_BG);
  draw_int(st.max_combo, 110, y, false, COL_CYAN, COL_BG);

  y += 18;
  eadk_display_draw_string("PRECISION:", {18, (uint16_t)y}, false, COL_WHITE, COL_BG);
  draw_int(acc / 100, 110, y, false, COL_GREEN, COL_BG);
  eadk_display_draw_string("%", {138, (uint16_t)y}, false, COL_GREEN, COL_BG);

  y += 24;
  eadk_display_draw_string("MAX 300:", {18, (uint16_t)y}, false, COL_CYAN, COL_BG);
  draw_int(st.count_marv, 105, y, false, COL_WHITE, COL_BG);

  y += 16;
  eadk_display_draw_string("300:", {18, (uint16_t)y}, false, COL_GOLD, COL_BG);
  draw_int(st.count_perf, 105, y, false, COL_WHITE, COL_BG);

  y += 16;
  eadk_display_draw_string("200:", {18, (uint16_t)y}, false, COL_GREEN, COL_BG);
  draw_int(st.count_great, 105, y, false, COL_WHITE, COL_BG);

  y += 16;
  eadk_display_draw_string("100 / 50:", {18, (uint16_t)y}, false, COL_BLUE, COL_BG);
  draw_int(st.count_good + st.count_bad, 105, y, false, COL_WHITE, COL_BG);

  y += 16;
  eadk_display_draw_string("MISS:", {18, (uint16_t)y}, false, COL_RED, COL_BG);
  draw_int(st.count_miss, 105, y, false, COL_RED, COL_BG);

  fill_rect(0, 218, SCREEN_W, 22, COL_DARK_PANEL);
  eadk_display_draw_string("OK/EXE: Menu   BACK: Rejouer", {55, 223}, false, COL_GRAY, COL_DARK_PANEL);
}

// --- MAIN LOOP ---
int main(int argc, char* argv[]) {
  GameScreen screen = SCREEN_MENU;
  int selected_map = 0;
  bool prev_up = false, prev_down = false, prev_ok = false, prev_back = false;

  // Gameplay vars
  uint64_t song_start_ms = 0;
  PlayStats stats;
  bool lane_pressed[LANE_COUNT] = {false, false, false, false};
  bool stage_failed = false;

  draw_menu(selected_map);

  while (true) {
    eadk_keyboard_state_t keys = eadk_keyboard_scan();

    if (eadk_keyboard_key_down(keys, eadk_key_home)) {
      break;
    }

    bool up   = eadk_keyboard_key_down(keys, eadk_key_up);
    bool down = eadk_keyboard_key_down(keys, eadk_key_down);
    bool ok   = eadk_keyboard_key_down(keys, eadk_key_ok) || eadk_keyboard_key_down(keys, eadk_key_exe);
    bool back = eadk_keyboard_key_down(keys, eadk_key_back);

    if (screen == SCREEN_MENU) {
      bool redraw = false;
      if (up && !prev_up) {
        selected_map = (selected_map - 1 + NUM_BEATMAPS) % NUM_BEATMAPS;
        redraw = true;
      }
      if (down && !prev_down) {
        selected_map = (selected_map + 1) % NUM_BEATMAPS;
        redraw = true;
      }
      if (redraw) {
        draw_menu(selected_map);
      }
      if (ok && !prev_ok) {
        generate_beatmap(selected_map);
        memset(&stats, 0, sizeof(stats));
        stats.hp = 100;
        stage_failed = false;
        for (int i = 0; i < LANE_COUNT; i++) lane_pressed[i] = false;
        song_start_ms = eadk_timing_millis();
        screen = SCREEN_PLAYING;

        // Static Playfield Background
        fill_rect(0, 0, SCREEN_W, SCREEN_H, COL_BG);
        fill_rect(FIELD_X - 3, 0, 3, SCREEN_H, COL_BORDER);
        fill_rect(FIELD_X + FIELD_W, 0, 3, SCREEN_H, COL_BORDER);

        // Left & Right HUD static labels
        eadk_display_draw_string("SCORE", {10, 12}, false, COL_PINK, COL_BG);
        eadk_display_draw_string("COMBO", {10, 62}, false, COL_CYAN, COL_BG);
        eadk_display_draw_string("NIVEAU", {10, 115}, false, COL_GRAY, COL_BG);
        eadk_display_draw_string(BEATMAPS[selected_map].diff_name, {10, 130}, false, COL_GOLD, COL_BG);
        eadk_display_draw_string("BACK:", {10, 195}, false, COL_GRAY, COL_BG);
        eadk_display_draw_string("Quitter", {10, 210}, false, COL_GRAY, COL_BG);

        eadk_display_draw_string("HP", {252, 12}, false, COL_GREEN, COL_BG);
      }
    } else if (screen == SCREEN_PLAYING) {
      if (back && !prev_back) {
        screen = SCREEN_MENU;
        draw_menu(selected_map);
        prev_up = up; prev_down = down; prev_ok = ok; prev_back = back;
        continue;
      }

      int32_t now_ms = (int32_t)(eadk_timing_millis() - song_start_ms);
      const BeatmapMeta& meta = BEATMAPS[selected_map];

      // 1. Input Processing per Lane
      for (int lane = 0; lane < LANE_COUNT; lane++) {
        bool down_now = eadk_keyboard_key_down(keys, LANE_KEYS[lane]);
        bool just_pressed = down_now && !lane_pressed[lane];
        bool just_released = !down_now && lane_pressed[lane];
        lane_pressed[lane] = down_now;

        if (just_pressed) {
          // Find earliest active note in this lane within hit window
          int best_idx = -1;
          int32_t best_diff = 999999;
          for (int i = 0; i < g_note_count; i++) {
            if (g_notes[i].lane == lane && g_notes[i].state == 0) {
              int32_t diff = now_ms - g_notes[i].time_ms;
              int32_t abs_diff = (diff < 0) ? -diff : diff;
              if (abs_diff <= WIN_MISS && abs_diff < best_diff) {
                best_diff = abs_diff;
                best_idx = i;
              }
            }
          }

          if (best_idx != -1) {
            Judgement j = JUDGE_MISS;
            if (best_diff <= WIN_MARVELOUS) j = JUDGE_MARVELOUS;
            else if (best_diff <= WIN_PERFECT) j = JUDGE_PERFECT;
            else if (best_diff <= WIN_GREAT) j = JUDGE_GREAT;
            else if (best_diff <= WIN_GOOD) j = JUDGE_GOOD;
            else if (best_diff <= WIN_BAD) j = JUDGE_BAD;
            else j = JUDGE_MISS;

            apply_judgement(stats, j, now_ms);
            if (g_notes[best_idx].duration_ms > 0 && j != JUDGE_MISS) {
              g_notes[best_idx].state = 1; // Holding LN
            } else {
              g_notes[best_idx].state = 2; // Completed
            }
          }
        }

        if (just_released) {
          // Check if holding an LN in this lane
          for (int i = 0; i < g_note_count; i++) {
            if (g_notes[i].lane == lane && g_notes[i].state == 1) {
              int32_t end_time = g_notes[i].time_ms + g_notes[i].duration_ms;
              int32_t diff = end_time - now_ms;
              int32_t abs_diff = (diff < 0) ? -diff : diff;
              if (abs_diff <= WIN_GOOD) {
                apply_judgement(stats, JUDGE_PERFECT, now_ms);
              } else {
                apply_judgement(stats, JUDGE_BAD, now_ms);
              }
              g_notes[i].state = 2;
            }
          }
        }
      }

      // 2. Auto-miss & LN completion check
      int remaining_notes = 0;
      for (int i = 0; i < g_note_count; i++) {
        Note& n = g_notes[i];
        if (n.state == 0) {
          if (now_ms - n.time_ms > WIN_MISS) {
            n.state = 3; // Missed
            apply_judgement(stats, JUDGE_MISS, now_ms);
          } else {
            remaining_notes++;
          }
        } else if (n.state == 1) {
          int32_t end_time = n.time_ms + n.duration_ms;
          if (now_ms >= end_time) {
            n.state = 2;
            apply_judgement(stats, JUDGE_MARVELOUS, now_ms);
          } else {
            remaining_notes++;
          }
        }
      }

      // Check Fail or Clear
      if (stats.hp <= 0) {
        stage_failed = true;
        screen = SCREEN_RESULTS;
        draw_results(selected_map, stats, true);
        continue;
      }
      if (remaining_notes == 0) {
        screen = SCREEN_RESULTS;
        draw_results(selected_map, stats, false);
        continue;
      }

      // 3. Render Playfield
      fill_rect(FIELD_X, 0, FIELD_W, SCREEN_H, COL_FIELD_BG);

      // Lane separators & pressed key receptor glow
      for (int lane = 0; lane < LANE_COUNT; lane++) {
        int lx = FIELD_X + lane * LANE_W;
        if (lane > 0) {
          fill_rect(lx, 0, 1, SCREEN_H, COL_LANE_SEP);
        }
        if (lane_pressed[lane]) {
          fill_rect(lx + 1, HIT_LINE_Y - 18, LANE_W - 2, 18, 0x2967);
        }
      }

      // Render Notes
      for (int i = 0; i < g_note_count; i++) {
        const Note& n = g_notes[i];
        if (n.state == 2 || n.state == 3) continue;

        int32_t dt = n.time_ms - now_ms;
        if (dt > meta.scroll_ms) continue; // Not yet visible on screen

        int head_y = HIT_LINE_Y - (int)((dt * (int32_t)HIT_LINE_Y) / (int32_t)meta.scroll_ms);
        int lx = FIELD_X + n.lane * LANE_W + 3;
        int nw = LANE_W - 6;

        bool outer = (n.lane == 0 || n.lane == 3);
        eadk_color_t head_col = outer ? COL_CYAN : COL_PINK;
        eadk_color_t body_col = outer ? COL_HOLD_CYAN : COL_HOLD_PINK;

        if (n.duration_ms > 0) {
          int32_t end_dt = (n.time_ms + n.duration_ms) - now_ms;
          int tail_y = HIT_LINE_Y - (int)((end_dt * (int32_t)HIT_LINE_Y) / (int32_t)meta.scroll_ms);
          int clamped_head = (n.state == 1) ? HIT_LINE_Y : head_y;
          if (clamped_head > SCREEN_H) clamped_head = SCREEN_H;
          if (tail_y < 0) tail_y = 0;
          if (clamped_head > tail_y) {
            fill_rect(lx + 4, tail_y, nw - 8, clamped_head - tail_y, body_col);
          }
          // Tail cap
          if (tail_y >= 0 && tail_y < SCREEN_H) {
            fill_rect(lx + 2, tail_y, nw - 4, 5, head_col);
          }
        }

        int draw_y = (n.state == 1) ? (HIT_LINE_Y - NOTE_H / 2) : (head_y - NOTE_H / 2);
        if (draw_y >= -NOTE_H && draw_y < SCREEN_H) {
          fill_rect(lx, draw_y, nw, NOTE_H, head_col);
          fill_rect(lx + 2, draw_y + 2, nw - 4, 2, COL_WHITE); // Gloss highlight
        }
      }

      // Hit Line & Key Receptors
      fill_rect(FIELD_X, HIT_LINE_Y - 2, FIELD_W, 4, COL_HIT_LINE);
      for (int lane = 0; lane < LANE_COUNT; lane++) {
        int lx = FIELD_X + lane * LANE_W;
        eadk_color_t key_bg = lane_pressed[lane] ? COL_PINK : COL_DARK_PANEL;
        eadk_color_t key_fg = lane_pressed[lane] ? COL_BLACK : COL_WHITE;
        fill_rect(lx + 2, HIT_LINE_Y + 5, LANE_W - 4, 28, key_bg);
        eadk_display_draw_string(LANE_LABELS[lane], {(uint16_t)(lx + 14), (uint16_t)(HIT_LINE_Y + 12)}, false, key_fg, key_bg);
      }

      // Judgement Popup
      if (stats.last_judge != JUDGE_NONE && (now_ms - stats.last_judge_time) < 350) {
        const char* j_txt = "";
        eadk_color_t j_col = COL_WHITE;
        int j_x = FIELD_X + 42;
        switch (stats.last_judge) {
          case JUDGE_MARVELOUS: j_txt = "MAX 300!"; j_col = COL_CYAN;   j_x = FIELD_X + 36; break;
          case JUDGE_PERFECT:   j_txt = "300!";     j_col = COL_GOLD;   j_x = FIELD_X + 54; break;
          case JUDGE_GREAT:     j_txt = "200";      j_col = COL_GREEN;  j_x = FIELD_X + 58; break;
          case JUDGE_GOOD:      j_txt = "100";      j_col = COL_BLUE;   j_x = FIELD_X + 58; break;
          case JUDGE_BAD:       j_txt = "50";       j_col = COL_ORANGE; j_x = FIELD_X + 62; break;
          case JUDGE_MISS:      j_txt = "MISS";     j_col = COL_RED;    j_x = FIELD_X + 54; break;
          default: break;
        }
        eadk_display_draw_string(j_txt, {(uint16_t)j_x, 128}, false, j_col, COL_FIELD_BG);
      }

      // 4. Update HUD (Score, Combo, HP Bar)
      fill_rect(10, 28, 72, 16, COL_BG);
      draw_int((int)stats.score, 10, 28, false, COL_WHITE, COL_BG);

      fill_rect(10, 78, 72, 22, COL_BG);
      if (stats.combo > 0) {
        draw_int(stats.combo, 10, 78, true, COL_GOLD, COL_BG);
      }

      // Vertical HP Bar on the right
      fill_rect(252, 30, 14, 150, COL_DARK_PANEL);
      int hp_h = (stats.hp * 146) / 100;
      eadk_color_t hp_col = (stats.hp > 50) ? COL_GREEN : ((stats.hp > 25) ? COL_ORANGE : COL_RED);
      if (hp_h > 0) {
        fill_rect(254, 32 + (146 - hp_h), 10, hp_h, hp_col);
      }

      eadk_display_wait_for_vblank();
    } else if (screen == SCREEN_RESULTS) {
      if (ok && !prev_ok) {
        screen = SCREEN_MENU;
        draw_menu(selected_map);
      } else if (back && !prev_back) {
        // Quick Retry
        generate_beatmap(selected_map);
        memset(&stats, 0, sizeof(stats));
        stats.hp = 100;
        stage_failed = false;
        for (int i = 0; i < LANE_COUNT; i++) lane_pressed[i] = false;
        song_start_ms = eadk_timing_millis();
        screen = SCREEN_PLAYING;

        fill_rect(0, 0, SCREEN_W, SCREEN_H, COL_BG);
        fill_rect(FIELD_X - 3, 0, 3, SCREEN_H, COL_BORDER);
        fill_rect(FIELD_X + FIELD_W, 0, 3, SCREEN_H, COL_BORDER);
        eadk_display_draw_string("SCORE", {10, 12}, false, COL_PINK, COL_BG);
        eadk_display_draw_string("COMBO", {10, 62}, false, COL_CYAN, COL_BG);
        eadk_display_draw_string("NIVEAU", {10, 115}, false, COL_GRAY, COL_BG);
        eadk_display_draw_string(BEATMAPS[selected_map].diff_name, {10, 130}, false, COL_GOLD, COL_BG);
        eadk_display_draw_string("BACK:", {10, 195}, false, COL_GRAY, COL_BG);
        eadk_display_draw_string("Quitter", {10, 210}, false, COL_GRAY, COL_BG);
        eadk_display_draw_string("HP", {252, 12}, false, COL_GREEN, COL_BG);
      }
    }

    prev_up = up;
    prev_down = down;
    prev_ok = ok;
    prev_back = back;
  }

  return 0;
}
