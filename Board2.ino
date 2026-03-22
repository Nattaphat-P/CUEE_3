//Credit made by xin only
#include <ESP8266WiFi.h>
#include <espnow.h>

uint8_t esp32Mac[] = {0xB0, 0xCB, 0xD8, 0xCD, 0xA2, 0x8C};

#define TRIG_UP  5
#define ECHO_UP  4
#define TRIG_LOW 14
#define ECHO_LOW 12

typedef struct __attribute__((packed)) {
  float distUp;
  float distLow;
} BackData;

BackData dataToSend;

float getDistance(int trig, int echo) {
  digitalWrite(trig, LOW);
  delayMicroseconds(2);
  digitalWrite(trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig, LOW);
  long duration = pulseIn(echo, HIGH, 25000);
  if (duration == 0) return 400;
  return duration * 0.034 / 2.0;
}

void onSent(uint8_t *mac, uint8_t status) {
  Serial.println(status == 0 ? "Send: OK" : "Send: FAIL");
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(TRIG_UP,  OUTPUT); digitalWrite(TRIG_UP,  LOW);
  pinMode(ECHO_UP,  INPUT);
  pinMode(TRIG_LOW, OUTPUT); digitalWrite(TRIG_LOW, LOW);
  pinMode(ECHO_LOW, INPUT);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  Serial.print("MAC: "); Serial.println(WiFi.macAddress());

  if (esp_now_init() != 0) {
    Serial.println("ESP-NOW FAILED"); return;
  }

  esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
  esp_now_register_send_cb(onSent);
  esp_now_add_peer(esp32Mac, ESP_NOW_ROLE_SLAVE, 1, NULL, 0);

  Serial.println("ESP8266 Ready");
}

void loop() {
  dataToSend.distUp  = getDistance(TRIG_UP,  ECHO_UP);
  delay(30);
  dataToSend.distLow = getDistance(TRIG_LOW, ECHO_LOW);

  Serial.printf("UP: %.1f | LOW: %.1f\n", dataToSend.distUp, dataToSend.distLow);
  esp_now_send(esp32Mac, (uint8_t *)&dataToSend, sizeof(dataToSend));

  delay(500);
}
