#include "init_helper_ELYOS.h"
#include <lgfx/v1/platforms/esp32s3/Panel_RGB.hpp>
#include <lgfx/v1/platforms/esp32s3/Bus_RGB.hpp>

/* ---------- Backlight pin ---------- */
#define TFT_BL 2

/* ---------- Display resolution ---------- */
#define ELYOS_WIDTH  800
#define ELYOS_HEIGHT 480

/* ---------- LVGL buffers ---------- */
static lv_color_t elyos_disp_buf[ELYOS_WIDTH * ELYOS_HEIGHT / 15];

lv_disp_draw_buf_t elyos_draw_buf;
lv_disp_drv_t elyos_disp_drv;

uint32_t elyos_screen_width;
uint32_t elyos_screen_height;

/* ---------- LovyanGFX Display Class ---------- */
class LGFX : public lgfx::LGFX_Device
{
public:
    lgfx::Bus_RGB   _bus;
    lgfx::Panel_RGB _panel;

    LGFX(void)
    {
        {
            auto cfg = _bus.config();
            cfg.panel = &_panel;

            cfg.pin_d0  = GPIO_NUM_15;
            cfg.pin_d1  = GPIO_NUM_7;
            cfg.pin_d2  = GPIO_NUM_6;
            cfg.pin_d3  = GPIO_NUM_5;
            cfg.pin_d4  = GPIO_NUM_4;

            cfg.pin_d5  = GPIO_NUM_9;
            cfg.pin_d6  = GPIO_NUM_46;
            cfg.pin_d7  = GPIO_NUM_3;
            cfg.pin_d8  = GPIO_NUM_8;
            cfg.pin_d9  = GPIO_NUM_16;
            cfg.pin_d10 = GPIO_NUM_1;

            cfg.pin_d11 = GPIO_NUM_14;
            cfg.pin_d12 = GPIO_NUM_21;
            cfg.pin_d13 = GPIO_NUM_47;
            cfg.pin_d14 = GPIO_NUM_48;
            cfg.pin_d15 = GPIO_NUM_45;

            cfg.pin_henable = GPIO_NUM_41;
            cfg.pin_vsync   = GPIO_NUM_40;
            cfg.pin_hsync   = GPIO_NUM_39;
            cfg.pin_pclk    = GPIO_NUM_0;

            cfg.freq_write = 15000000;

            cfg.hsync_polarity     = 0;
            cfg.hsync_front_porch  = 40;
            cfg.hsync_pulse_width  = 48;
            cfg.hsync_back_porch   = 40;

            cfg.vsync_polarity     = 0;
            cfg.vsync_front_porch  = 1;
            cfg.vsync_pulse_width  = 31;
            cfg.vsync_back_porch   = 13;

            cfg.pclk_active_neg = 1;

            _bus.config(cfg);
        }

        {
            auto cfg = _panel.config();
            cfg.panel_width  = ELYOS_WIDTH;
            cfg.panel_height = ELYOS_HEIGHT;
            cfg.memory_width  = ELYOS_WIDTH;
            cfg.memory_height = ELYOS_HEIGHT;
            _panel.config(cfg);
        }

        _panel.setBus(&_bus);
        setPanel(&_panel);
    }
};

LGFX lcd;

/* ---------- LVGL Flush Callback ---------- */
static void elyos_disp_flush(lv_disp_drv_t *disp,
                            const lv_area_t *area,
                            lv_color_t *color_p)
{
    uint32_t w = area->x2 - area->x1 + 1;
    uint32_t h = area->y2 - area->y1 + 1;

    lcd.pushImageDMA(area->x1, area->y1, w, h,
                     (lgfx::rgb565_t *)&color_p->full);

    lv_disp_flush_ready(disp);
}

/* ---------- Touch Callback (external driver) ---------- */
static void elyos_touch_read(lv_indev_drv_t *indev,
                             lv_indev_data_t *data)
{
    if (touch_has_signal() && touch_touched())
    {
        data->state = LV_INDEV_STATE_PR;
        data->point.x = touch_last_x;
        data->point.y = touch_last_y;
    }
    else
    {
        data->state = LV_INDEV_STATE_REL;
    }
}

/* ---------- Init Functions ---------- */

void elyos_display_init(void)
{
    lcd.begin();
    lcd.fillScreen(TFT_BLACK);

    elyos_screen_width  = lcd.width();
    elyos_screen_height = lcd.height();
}

void elyos_lvgl_init(void)
{
    lv_init();

    lv_disp_draw_buf_init(&elyos_draw_buf,
                          elyos_disp_buf,
                          NULL,
                          ELYOS_WIDTH * ELYOS_HEIGHT / 15);

    lv_disp_drv_init(&elyos_disp_drv);
    elyos_disp_drv.hor_res  = elyos_screen_width;
    elyos_disp_drv.ver_res  = elyos_screen_height;
    elyos_disp_drv.flush_cb = elyos_disp_flush;
    elyos_disp_drv.draw_buf = &elyos_draw_buf;

    lv_disp_drv_register(&elyos_disp_drv);
}

void elyos_touch_init(void)
{
    touch_init();

    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = elyos_touch_read;
    lv_indev_drv_register(&indev_drv);
}

void elyos_backlight_init(uint8_t brightness)
{
    ledcSetup(1, 300, 8);
    ledcAttachPin(TFT_BL, 1);
    ledcWrite(1, brightness);
}
