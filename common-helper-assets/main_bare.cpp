#include <Arduino.h>
#include "init_helper_ELYOS.h"
#include "ui.h"
#include <stdlib.h>

void setup()
{
    Serial.begin(115200);

    elyos_display_init();
    elyos_lvgl_init();
    elyos_touch_init();
    elyos_backlight_init(200); 

    ui_init();
}

void loop()
{
    lv_timer_handler();
    delay(5);
}
