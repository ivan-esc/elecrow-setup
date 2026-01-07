#include <Arduino.h>
#include "init_helper_ELYOS.h"
#include "ui.h"
#include <stdlib.h>

// ================= UART CONFIG =================
#define DISPLAY_RX_PIN 44
#define DISPLAY_TX_PIN 43
#define DISPLAY_UART   1
#define BAUDRATE       115200

HardwareSerial DisplaySerial(DISPLAY_UART);

// ================= PROTOCOL =================
#define SYNC_BYTE 0xAA

#define CMD_FAST       0x01
#define CMD_AWARENESS  0x02
#define CMD_GRAPH      0x03
#define CMD_HEARTBEAT  0x04
#define CMD_MESSAGE    0x05

// ================= MESSAGE =================
typedef enum {
  IDLE_MSG,
  RESET_MSG,
  CUSTOM_MSG,
} Message;
Message sent_message = IDLE_MSG;

// ================= DATA STRUCT =================
typedef struct {
    float battery_voltage;
    float current_amps;
    uint8_t laps;
    float consumption;
    float efficiency;
    uint16_t rpms;
    float velocity;
    uint8_t tx_message;
} DisplayData;

DisplayData received_data = {0};

// ================= RX STATE MACHINE =================
enum RxState {
    WAIT_SYNC,
    WAIT_CMD,
    WAIT_LEN,
    WAIT_PAYLOAD
};
RxState rxState = WAIT_SYNC;

uint8_t rxCmd = 0;
uint8_t rxLen = 0;
uint8_t rxIndex = 0;
uint8_t rxBuffer[32];

// ================= MESSAGE RECEIVER ===========
#define MAX_CUSTOM_MSG_LEN 32

static char custom_msg_buf[MAX_CUSTOM_MSG_LEN + 1];
static uint8_t last_message_type = 0xFF;   // force first update
static bool message_dirty = false;

// ================= TIME BASE =================
static uint32_t attempt_start_ms = 0;
static uint32_t lap_start_ms     = 0;
static uint8_t  last_laps        = 0;

// ================= UPDATE SCHEDULING =================
static uint32_t last_10ms_update  = 0;
static uint32_t last_100ms_update = 0;
static uint32_t last_2s_update    = 0;

// ================= HELPERS =================
float bytesToFloat(uint8_t *b) {
    union {
        float f;
        uint8_t u8[4];
    } v;
    memcpy(v.u8, b, 4);
    return v.f;
}

uint16_t bytesToU16(uint8_t *b) {
    return (b[0] << 8) | b[1];
}

static void format_time_mmsshh(uint32_t elapsed_ms, char *buf, size_t len)
{
    uint32_t hundredths = (elapsed_ms / 10) % 100;
    uint32_t seconds    = (elapsed_ms / 1000) % 60;
    uint32_t minutes    = elapsed_ms / 60000;
    if (minutes > 99) minutes = 99;

    snprintf(buf, len, "%02lu:%02lu.%02lu",
             minutes, seconds, hundredths);
}

// ================= FRAME DECODER =================
void decodeFrame(uint8_t cmd, uint8_t *buf, uint8_t len) {

    switch (cmd) {

        case CMD_FAST:
            if (len == 6) {
                received_data.rpms = bytesToU16(&buf[0]);
                received_data.velocity = bytesToFloat(&buf[2]);
            }
            break;

        case CMD_AWARENESS:
            if (len == 9) {
                received_data.laps = buf[0];
                received_data.consumption = bytesToFloat(&buf[1]);
                received_data.efficiency  = bytesToFloat(&buf[5]);
            }
            break;

        case CMD_GRAPH:
            if (len == 8) {
                received_data.battery_voltage = bytesToFloat(&buf[0]);
                received_data.current_amps    = bytesToFloat(&buf[4]);
            }
            break;

        case CMD_HEARTBEAT:
            if (len == 1) {
                received_data.tx_message = buf[0];
            }
            break;
        case CMD_MESSAGE:
            if (len > 0 && len <= MAX_CUSTOM_MSG_LEN) {
                memcpy(custom_msg_buf, buf, len);
                custom_msg_buf[len] = '\0';   // null terminate
                message_dirty = true;
            }
            break;
        default:
            break;
    }
}

// ================= UART POLLER =================
void DisplayUART() {
    while (DisplaySerial.available()) {
        uint8_t byteIn = DisplaySerial.read();
        switch (rxState) {
            case WAIT_SYNC:
                if (byteIn == SYNC_BYTE) {
                    rxState = WAIT_CMD;
                }
                break;
            case WAIT_CMD:
                rxCmd = byteIn;
                rxState = WAIT_LEN;
                break;
            case WAIT_LEN:
                rxLen = byteIn;
                rxIndex = 0;
                if (rxLen > sizeof(rxBuffer)) {
                    rxState = WAIT_SYNC; // invalid length
                } else {
                    rxState = WAIT_PAYLOAD;
                }
                break;
            case WAIT_PAYLOAD:
                rxBuffer[rxIndex++] = byteIn;
                if (rxIndex >= rxLen) {
                    decodeFrame(rxCmd, rxBuffer, rxLen);
                    rxState = WAIT_SYNC;
                }
                break;
        }    
        if(button_aux){ //Trigger when button pressed TX
            DisplaySerial.write(SYNC_BYTE);
            button_aux = 0;
        }
    }
}

static void update_10ms(void)
{
    uint32_t now = millis();

    // ---------- RPM ----------
    static uint16_t last_rpm = 0;
    if (received_data.rpms != last_rpm) {
        last_rpm = received_data.rpms;
        char buf[8];
        snprintf(buf, sizeof(buf), "%3u rpm", last_rpm);
        lv_label_set_text(ui_rpmLabel, buf);
    }

    // ---------- Velocity (1 decimal) ----------
    static float last_velocity = -1.0f;
    if (received_data.velocity != last_velocity) {
        last_velocity = received_data.velocity;
        char buf[8];
        snprintf(buf, sizeof(buf), "%.1f", last_velocity);
        lv_label_set_text(ui_velocityLabel, buf);
    }
    // ---------- Arc (velocity 0–100 → arc 0–99) ----------
    uint16_t arc_val = (uint16_t)(received_data.velocity);
    if (arc_val > 99) arc_val = 99;

    //Low pass for smoother animation
    static float arc_display = 0.0f;   // persistent displayed value
    arc_display += (arc_val - arc_display) * 0.18f;
    lv_arc_set_value(ui_Arc1, (uint16_t)arc_display);


    // ---------- Lap change detection ----------
    if (received_data.laps != last_laps) {
        last_laps = received_data.laps;
        lap_start_ms = now;

        // First lap → reset full attempt timer
        if (received_data.laps == 1) {
            attempt_start_ms = now;
        }
        // Reset button pressed
        else if(received_data.laps == 0){
            lv_label_set_text(ui_fullattemptLabel, "00:00.00");
            lv_label_set_text(ui_laptimeLabel, "00:00.00");
            attempt_start_ms = 0;
            lap_start_ms = 0;
        }

        char buf[4];
        snprintf(buf, sizeof(buf), "%u", received_data.laps);
        lv_label_set_text(ui_lapsLabel, buf);
    }

    // ---------- Full attempt timer ----------
    if (attempt_start_ms != 0) {
        char buf[9];
        format_time_mmsshh(now - attempt_start_ms, buf, sizeof(buf));
        lv_label_set_text(ui_fullattemptLabel, buf);
    }

    // ---------- Lap timer ----------
    if (lap_start_ms != 0) {
        char buf[9];
        format_time_mmsshh(now - lap_start_ms, buf, sizeof(buf));
        lv_label_set_text(ui_laptimeLabel, buf);
    }
}

static void update_100ms(void)
{
    // ---------- Arc color swapping ----------
    float v = received_data.velocity;
    lv_color_t c;
    if (v <= 50.0f) {
        c = lv_palette_main(LV_PALETTE_BLUE);
    } else if (v <= 90.0f) {
        uint8_t t = (uint8_t)((v - 50.0f) * 255.0f / 40.0f);
        c = lv_color_mix(lv_palette_main(LV_PALETTE_PURPLE),
                         lv_palette_main(LV_PALETTE_BLUE), t);
    } else {
        uint8_t t = (uint8_t)((v - 90.0f) * 255.0f / 10.0f);
        c = lv_color_mix(lv_palette_main(LV_PALETTE_RED),
                         lv_palette_main(LV_PALETTE_PURPLE), t);
    }
    lv_obj_set_style_arc_color(ui_Arc1, c, LV_PART_INDICATOR);

    // ---------- Consumption (2 decimals) ----------
    char buf[10];
    snprintf(buf, sizeof(buf), "%.2f", received_data.consumption);
    lv_label_set_text(ui_ConsumptionLabel, buf);

    // ---------- Efficiency (2 decimals) ----------
    snprintf(buf, sizeof(buf), "%.2f", received_data.efficiency);
    lv_label_set_text(ui_EfficiencyLabel, buf);
}

static void update_2s(void)
{
    //Cambiar ejes para mejorar resolucion a 1 decimal en vez de int
    int32_t amps_val = (int32_t)(received_data.current_amps * 10.0f + 0.5f);
    int32_t volt_val = (int32_t)(received_data.battery_voltage * 10.0f + 0.5f);

    // Mover previas
    memmove(&ui_Chart1_series_1_array[0],
            &ui_Chart1_series_1_array[1],
            (60 - 1) * sizeof(lv_coord_t));
    memmove(&ui_Chart1_series_2_array[0],
            &ui_Chart1_series_2_array[1],
            (60 - 1) * sizeof(lv_coord_t));
    // Mas reciente a la derecha
    ui_Chart1_series_1_array[60 - 1] = amps_val;
    ui_Chart1_series_2_array[60 - 1] = volt_val;

    lv_chart_refresh(ui_Chart1);

    static lv_obj_t *bat_clip = NULL;
    static lv_coord_t full_h;
    static lv_coord_t base_y;

    /* ---------- One-time lazy setup ---------- */
    if (!bat_clip) {
        bat_clip = lv_obj_create(lv_obj_get_parent(ui_batFull));
        lv_obj_remove_style_all(bat_clip);
        lv_obj_set_pos(bat_clip,lv_obj_get_x(ui_batFull),lv_obj_get_y(ui_batFull));
        lv_obj_set_size(bat_clip, lv_obj_get_width(ui_batFull),lv_obj_get_height(ui_batFull));

        lv_obj_set_style_clip_corner(bat_clip, true, 0);

        /* Move image inside clip */
        lv_obj_set_parent(ui_batFull, bat_clip);
        lv_obj_clear_flag(ui_batFull, LV_OBJ_FLAG_FLOATING);
        lv_obj_align(ui_batFull, LV_ALIGN_BOTTOM_MID, 0, 0);

        full_h = lv_obj_get_height(ui_batFull);
        base_y = lv_obj_get_y(bat_clip);
    }

    /* ---------- Battery fill logic ---------- */
    float v = received_data.velocity;
    if (v < 0)   v = 0;
    if (v > 100) v = 100;

    lv_coord_t vis_h = (lv_coord_t)(full_h * (v / 100.0f));
    if (vis_h < 1) vis_h = 1;

    lv_obj_set_height(bat_clip, vis_h);
    lv_obj_set_y(bat_clip, 339 + base_y + (full_h - vis_h));    
}

void update_message_label(void)
{
    if (!message_dirty && received_data.tx_message == last_message_type)
        return;

    last_message_type = received_data.tx_message;
    message_dirty = false;

    switch (received_data.tx_message) {

        case IDLE_MSG:
            lv_label_set_text(ui_messageLabel, "Waiting for message...");
            lv_obj_set_style_text_color(ui_messageLabel, lv_color_hex(0x7D7E82), LV_PART_MAIN | LV_STATE_DEFAULT);
            break;

        case RESET_MSG:
            lv_label_set_text(ui_messageLabel, "Full attempt timer reset.");
            lv_obj_set_style_text_color(ui_messageLabel, lv_color_hex(0x7D7E82), LV_PART_MAIN | LV_STATE_DEFAULT);
            break; 

        case CUSTOM_MSG:
            lv_label_set_text(ui_messageLabel, custom_msg_buf);
            lv_obj_set_style_text_color(ui_messageLabel, lv_color_hex(0xD4D3D6), LV_PART_MAIN | LV_STATE_DEFAULT);
            break;

        default:
            break;
    }
}

    
// ================= SETUP =================
void setup()
{
    //Serial.begin(115200); //Debug Data RX prints
    DisplaySerial.begin(
        BAUDRATE,
        SERIAL_8N1,
        DISPLAY_RX_PIN,
        DISPLAY_TX_PIN
    );

    elyos_display_init();
    elyos_lvgl_init();
    elyos_touch_init();
    elyos_backlight_init(200);

    ui_init();
}

// ================= LOOP =================
void loop()
{
    DisplayUART();   // Non-blocking RX-TX

    uint32_t now = millis();

    // ---------- LVGL ----------
    static uint32_t last_lvgl_ms = 0;
    if (now - last_lvgl_ms >= 5) {
        last_lvgl_ms = now;
        lv_timer_handler();
    }

    update_message_label(); 

    // ---------- 10 ms ----------
    if (now - last_10ms_update >= 10) {
        last_10ms_update = now;
        update_10ms();
    }

    // ---------- 100 ms ----------
    if (now - last_100ms_update >= 100) {
        last_100ms_update = now;
        update_100ms();
    }

    // ---------- 2 seconds ----------
    if (now - last_2s_update >= 2000) {
        last_2s_update = now;
        update_2s();
    }
}
