#pragma once

#include <lvgl.h>
#include <LovyanGFX.hpp>

/* ---------- Display object ---------- */
extern lgfx::LGFX_Device& lcd;

/* ---------- Screen parameters ---------- */
extern uint32_t elyos_screen_width;
extern uint32_t elyos_screen_height;

/* ---------- LVGL objects ---------- */
extern lv_disp_draw_buf_t elyos_draw_buf;
extern lv_disp_drv_t elyos_disp_drv;

/* ---------- Public API ---------- */
void elyos_display_init(void);
void elyos_lvgl_init(void);
void elyos_touch_init(void);
void elyos_backlight_init(uint8_t brightness);

#include <stdint.h>
#include <stdbool.h>

/* ---------- Touch driver API ---------- */
void touch_init(void);
bool touch_has_signal(void);
bool touch_touched(void);

/* ---------- Last touch point ---------- */
extern uint16_t touch_last_x;
extern uint16_t touch_last_y;