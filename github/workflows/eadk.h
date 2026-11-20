#ifndef EADK_H
#define EADK_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// NumWorks Screen Resolution: 320 x 240
#define EADK_SCREEN_WIDTH 320
#define EADK_SCREEN_HEIGHT 240

typedef uint16_t eadk_color_t;

static inline eadk_color_t eadk_color_rgb(uint8_t r, uint8_t g, uint8_t b) {
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

#define EADK_COLOR_BLACK 0x0000
#define EADK_COLOR_WHITE 0xFFFF
#define EADK_COLOR_RED   0xF800
#define EADK_COLOR_GREEN 0x07E0
#define EADK_COLOR_BLUE  0x001F

typedef struct {
  uint16_t x;
  uint16_t y;
  uint16_t width;
  uint16_t height;
} eadk_rect_t;

typedef struct {
  uint16_t x;
  uint16_t y;
} eadk_point_t;

// Keyboard bitmask
typedef uint64_t eadk_keyboard_state_t;

typedef enum {
  eadk_key_left = 0,
  eadk_key_up = 1,
  eadk_key_down = 2,
  eadk_key_right = 3,
  eadk_key_ok = 4,
  eadk_key_back = 5,
  eadk_key_home = 6,
  eadk_key_on_off = 8,
  eadk_key_shift = 12,
  eadk_key_alpha = 13,
  eadk_key_xnt = 14,
  eadk_key_var = 15,
  eadk_key_toolbox = 16,
  eadk_key_backspace = 17,
  eadk_key_exp = 18,
  eadk_key_ln = 19,
  eadk_key_log = 20,
  eadk_key_imaginary = 21,
  eadk_key_comma = 22,
  eadk_key_power = 23,
  eadk_key_sine = 24,
  eadk_key_cosine = 25,
  eadk_key_tangent = 26,
  eadk_key_pi = 27,
  eadk_key_sqrt = 28,
  eadk_key_square = 29,
  eadk_key_seven = 30,
  eadk_key_eight = 31,
  eadk_key_nine = 32,
  eadk_key_left_parenthesis = 33,
  eadk_key_right_parenthesis = 34,
  eadk_key_four = 36,
  eadk_key_five = 37,
  eadk_key_six = 38,
  eadk_key_multiplication = 39,
  eadk_key_division = 40,
  eadk_key_one = 42,
  eadk_key_two = 43,
  eadk_key_three = 44,
  eadk_key_plus = 45,
  eadk_key_minus = 46,
  eadk_key_zero = 48,
  eadk_key_dot = 49,
  eadk_key_ee = 50,
  eadk_key_ans = 51,
  eadk_key_exe = 52
} eadk_key_t;

static inline bool eadk_keyboard_key_down(eadk_keyboard_state_t state, eadk_key_t key) {
  return (state >> (uint8_t)key) & 1;
}

// EADK External Symbols (resolved by NumWorks OS at load time)
extern const char eadk_app_name[];
extern const uint32_t eadk_api_level;

void eadk_display_push_rect(eadk_rect_t rect, const eadk_color_t *pixels);
void eadk_display_push_rect_uniform(eadk_rect_t rect, eadk_color_t color);
void eadk_display_pull_rect(eadk_rect_t rect, eadk_color_t *pixels);
void eadk_display_draw_string(const char *text, eadk_point_t point, bool large_font, eadk_color_t text_color, eadk_color_t background_color);
bool eadk_display_wait_for_vblank(void);

eadk_keyboard_state_t eadk_keyboard_scan(void);

void eadk_timing_usleep(uint32_t us);
void eadk_timing_msleep(uint32_t ms);
uint64_t eadk_timing_millis(void);

uint32_t eadk_random(void);

#ifdef __cplusplus
}
#endif

#endif
