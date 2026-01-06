#pragma once

#include <lvgl.h>
#include <LovyanGFX.hpp>
#include <lgfx/v1/platforms/esp32s3/Panel_RGB.hpp>
#include <lgfx/v1/platforms/esp32s3/Bus_RGB.hpp>

/* ---------- Display resolution ---------- */
#define ELYOS_WIDTH  800
#define ELYOS_HEIGHT 480

class LGFX : public lgfx::LGFX_Device
{
public:
    lgfx::Bus_RGB   _bus;
    lgfx::Panel_RGB _panel;

    LGFX();   // constructor declaration ONLY
};

/* Global display instance */
extern LGFX lcd;

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
