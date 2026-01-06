#include <Arduino.h>
#include <ezButton.h>

// ================= UART =================
#define UART_TX_PIN 18
#define UART_RX_PIN 19
#define UART_BAUD   115200

HardwareSerial DisplaySerial(1);

// ================= SYNC / COMMANDS =================
#define SYNC_BYTE 0xAA

#define CMD_FAST      0x01
#define CMD_AWARENESS 0x02
#define CMD_GRAPH     0x03
#define CMD_HEARTBEAT 0x04
#define CMD_MESSAGE 0x05

// ================= SIMULATION =================
#define LAPS_BUTTON_PIN 23
#define ADC_VEL_PIN     4
#define wheelR 0.5f
#define rpm_k  2.65f

#define MAX_CUSTOM_MSG_LEN 32
char custom_msg_buf[MAX_CUSTOM_MSG_LEN + 1];
uint8_t custom_msg_len = 0;
bool send_custom_msg = false;

ezButton button(LAPS_BUTTON_PIN);
uint16_t rpm_aux = 0;
uint16_t vel_aux10 = 0;
uint8_t pressCount = 0;

// ================= MESSAGE =================
typedef enum {
  IDLE_MSG,
  RESET_MSG,
  CUSTOM_MSG,
} Message;
Message sent_message = IDLE_MSG;

// ================= DATA =================
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

DisplayData sent_data;

// ================= TIMERS =================
unsigned long t_fast = 0;
unsigned long t_awareness = 0;
unsigned long t_graph = 0;
unsigned long t_heartbeat = 0;

// ================= HELPERS =================
void writeFloat(float v) {
  union { float f; uint8_t b[4]; } u;
  u.f = v;
  DisplaySerial.write(u.b, 4);
}

void writeU16(uint16_t v) {
  DisplaySerial.write((v >> 8) & 0xFF);
  DisplaySerial.write(v & 0xFF);
}

void generate_telemetry(DisplayData *data) {
  float raw_vel = analogRead(ADC_VEL_PIN);

  vel_aux10 = (uint16_t)((raw_vel / 4095.0f) * 999);
  rpm_aux   = (uint16_t)((raw_vel / 4095.0f) * 99.9f * rpm_k / wheelR);

  data->battery_voltage = random(220, 260) / 10.0f;
  data->current_amps    = random(70, 100) / 10.0f;
  data->laps            = pressCount;
  data->consumption     = random(12000, 15000) / 100.0f;
  data->efficiency      = random(16000, 20000) / 100.0f;
  data->rpms            = rpm_aux;
  data->velocity        = vel_aux10 / 10.0f;
  data->tx_message      = sent_message;
}

// ----- Serial input parser for sent_message -----
void handleSerialInput() {
  static char line[64];
  static uint8_t idx = 0;

  while (Serial.available()) {
    char c = Serial.read();

    if (c == '\n' || c == '\r') {
      line[idx] = '\0';
      idx = 0;

      // ---- Parse commands ----
      if (strcmp(line, "idle") == 0) {
        sent_message = IDLE_MSG;
      }
      else if (strncmp(line, "msg ", 4) == 0) {
        sent_message = CUSTOM_MSG;

        strncpy(custom_msg_buf, line + 4, MAX_CUSTOM_MSG_LEN);
        custom_msg_buf[MAX_CUSTOM_MSG_LEN] = '\0';
        custom_msg_len = strlen(custom_msg_buf);

        send_custom_msg = true;   // trigger UART send once
      }

    } else if (idx < sizeof(line) - 1) {
      line[idx++] = c;
    }
  }
}


// ================= SETUP =================
void setup() {
  Serial.begin(115200);

  DisplaySerial.begin(UART_BAUD, SERIAL_8N1, UART_RX_PIN, UART_TX_PIN);

  analogReadResolution(12);
  analogSetPinAttenuation(ADC_VEL_PIN, ADC_11db);
  button.setDebounceTime(50);

  Serial.println("UART Telemetry TX started");
}

void loop() {
  uint32_t now = millis();
  // -------- Buttons & serial input (always) --------
  button.loop();
  handleSerialInput();

  if (button.isPressed()) {
    pressCount++;
  }
  // -------- Telemetry update @10ms --------
  static uint32_t t_telemetry = 0;
  if (now - t_telemetry >= 10) {
    t_telemetry += 10;   // avoids drift
    generate_telemetry(&sent_data);
  }

  uint8_t byteIn = DisplaySerial.read();
  if (byteIn == SYNC_BYTE){ //Button Press Recieved
    pressCount = 0;
    sent_message = RESET_MSG;
  }

  // ================= CMD 0x01 (FAST – 10ms) =================
  if (now - t_fast >= 10) {
    t_fast += 10;
    DisplaySerial.write(SYNC_BYTE);
    DisplaySerial.write(CMD_FAST);
    DisplaySerial.write(6);
    writeU16(sent_data.rpms);
    writeFloat(sent_data.velocity);
  }

  // ================= CMD 0x02 (AWARENESS – 100ms) =================
  if (now - t_awareness >= 100) {
    t_awareness += 100;
    DisplaySerial.write(SYNC_BYTE);
    DisplaySerial.write(CMD_AWARENESS);
    DisplaySerial.write(9);
    DisplaySerial.write(sent_data.laps);
    writeFloat(sent_data.consumption);
    writeFloat(sent_data.efficiency);
  }

  // ================= CMD 0x03 (GRAPH – 1s) =================
  if (now - t_graph >= 1000) {
    t_graph += 1000;
    DisplaySerial.write(SYNC_BYTE);
    DisplaySerial.write(CMD_GRAPH);
    DisplaySerial.write(8);
    writeFloat(sent_data.battery_voltage);
    writeFloat(sent_data.current_amps);
  }

  // ================= CMD 0x04 (HEARTBEAT – 200ms) =================
  if (now - t_heartbeat >= 200) {
    t_heartbeat += 500;
    DisplaySerial.write(SYNC_BYTE);
    DisplaySerial.write(CMD_HEARTBEAT);
    DisplaySerial.write(1);
    DisplaySerial.write(sent_data.tx_message);
  }

  // ================= CMD 0x05 (CUSTOM MESSAGE – on demand) =================
  if (send_custom_msg) {
    send_custom_msg = false;

    DisplaySerial.write(SYNC_BYTE);
    DisplaySerial.write(CMD_MESSAGE);
    DisplaySerial.write(custom_msg_len);
    DisplaySerial.write((uint8_t *)custom_msg_buf, custom_msg_len);
    //Brute force it twice
    DisplaySerial.write(SYNC_BYTE);
    DisplaySerial.write(CMD_MESSAGE);
    DisplaySerial.write(custom_msg_len);
    DisplaySerial.write((uint8_t *)custom_msg_buf, custom_msg_len);

  }
}

