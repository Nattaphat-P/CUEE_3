/******************** ENUM ********************/
enum BeepType { BT_NONE, BT_EYE, BT_HUNCH, BT_WORK };

/******************** LIBRARY ********************/
#include <WiFi.h>
#include <esp_now.h>
#include <Wire.h>
#include <LiquidCrystal_PCF8574.h>

/******************** PIN CONFIG ********************/
#define TRIG_EYE      5
#define ECHO_EYE      18
#define PIN_LIGHT     34
#define BUZZER        13
#define LED_SYS       2
#define LED_ALERT_2   4
#define LED_ALERT_3   15
#define BTN           27

/******************** LCD ********************/
LiquidCrystal_PCF8574 lcd(0x27);

/******************** ESP-NOW DATA STRUCT ********************/
typedef struct __attribute__((packed)) {
  float distUp;
  float distLow;
} BackData;

BackData backData;
bool newBackData = false;

/******************** SYSTEM STATE ********************/
bool systemActive = false;
bool isSitting    = false;
unsigned long sessionStart         = 0;
unsigned long totalWorkAccumulated = 0;
bool isClose = false;
unsigned long closeStart = 0;
unsigned long closeTotal = 0;
bool isHunch = false;
unsigned long hunchStart = 0;
unsigned long hunchCount = 0;

/******************** ALERT FLAGS ********************/
bool alertEye   = false;
bool alertHunch = false;
bool alertLight = false;
bool alertWork  = false;

/******************** THRESHOLD ********************/
#define CLOSE_THRESHOLD  40
#define CLOSE_DELAY      3000
#define HUNCH_DIFF       15
#define LIGHT_THRESHOLD  500
#define WORK_LIMIT       3600000UL

/******************** LCD ********************/
String lastL1 = "", lastL2 = "";

void updateLCD(String l1, String l2) {
  if (l1 == lastL1 && l2 == lastL2) return;
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(l1);
  lcd.setCursor(0, 1);
  lcd.print(l2);
  lastL1 = l1;
  lastL2 = l2;
} 

/******************** BUZZER ********************/
#define ACTIVE_BUZZER true

void beepOnce(int freq, int duration) {
  if (ACTIVE_BUZZER) {
    digitalWrite(BUZZER, HIGH);
    delay(duration);
    digitalWrite(BUZZER, LOW);
  } else {
    tone(BUZZER, freq, duration);
    delay(duration + 50);
  }
}

void handleBuzzer(BeepType type) {
  switch (type) {
    case BT_EYE:
      beepOnce(2000, 300);
      delay(300);
      break;
    case BT_HUNCH:
      beepOnce(1500, 150);
      delay(150);
      beepOnce(1500, 150);
      delay(150);
      break;
    case BT_WORK:
      beepOnce(1000, 800);
      delay(400);
      break;
    default:
      if (ACTIVE_BUZZER) digitalWrite(BUZZER, LOW);
      else noTone(BUZZER);
      break;
  }
}

/******************** ULTRASONIC ********************/
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

/******************** ESP-NOW CALLBACK ********************/
void onDataReceived(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  if (len == sizeof(BackData)) {
    memcpy(&backData, data, sizeof(BackData));
    newBackData = true;
    Serial.printf("[ESP-NOW] UP: %.1f | LOW: %.1f\n",
      backData.distUp, backData.distLow);
  }
}

/******************** DEBUG ********************/

void printDebug(float eyeDist, float lux) {
  Serial.println("====== STATUS ======");
  Serial.printf("System : %s\n",  systemActive ? "ON" : "OFF");
  Serial.printf("Eye    : %.1f cm\n", eyeDist);
  Serial.printf("BackUp : %.1f cm\n", backData.distUp);
  Serial.printf("BackLow: %.1f cm\n", backData.distLow);
  Serial.printf("Light  : %.1f lx\n", lux);
  Serial.printf("Sitting: %s\n",  isSitting ? "YES" : "NO");
  Serial.printf("Hunch# : %lu\n", hunchCount);
  Serial.print("Alerts : ");
  if (alertEye)   Serial.print("[EYE] ");
  if (alertHunch) Serial.print("[HUNCH] ");
  if (alertLight) Serial.print("[LIGHT] ");
  if (alertWork)  Serial.print("[WORK] ");
  if (!alertEye && !alertHunch && !alertLight && !alertWork)
    Serial.print("NONE");
  Serial.println("\n====================\n");
}

/******************** SETUP ********************/
void setup() {
  Serial.begin(115200);
  Serial.println("ESP32 Booting...");

  Wire.begin();
  lcd.begin(16, 2);
  lcd.setBacklight(255);
  updateLCD("Booting...", "Please wait");

  pinMode(TRIG_EYE,    OUTPUT); digitalWrite(TRIG_EYE,    LOW);
  pinMode(ECHO_EYE,    INPUT);
  pinMode(PIN_LIGHT,   INPUT);
  pinMode(BUZZER,      OUTPUT); digitalWrite(BUZZER,      LOW);
  pinMode(LED_SYS,     OUTPUT); digitalWrite(LED_SYS,     LOW);
  pinMode(LED_ALERT_2, OUTPUT); digitalWrite(LED_ALERT_2, LOW);
  pinMode(LED_ALERT_3, OUTPUT); digitalWrite(LED_ALERT_3, LOW);
  pinMode(BTN,         INPUT_PULLUP);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW FAILED"); return;
  }
  esp_now_register_recv_cb(onDataReceived);

  updateLCD("System: OFF", "Press BTN");
}

/******************** LOOP ********************/
void loop() {

  /******** BUTTON ********/
  static bool lastBtn = HIGH;
  bool nowBtn = digitalRead(BTN);

  if (lastBtn == HIGH && nowBtn == LOW) {
    delay(50);
    systemActive = !systemActive;
    digitalWrite(LED_SYS, systemActive);

    if (systemActive) {
      updateLCD("System: ON", "Monitoring...");
      beepOnce(1500, 100);
    } else {
      updateLCD("System: OFF", "Good Bye!");
      digitalWrite(BUZZER,      LOW);
      digitalWrite(LED_ALERT_2, LOW);
      digitalWrite(LED_ALERT_3, LOW);
    }
    delay(200);
  }
  lastBtn = nowBtn;

  if (!systemActive) return;

  /******** SENSOR READ ********/
  alertEye = alertHunch = alertLight = alertWork = false;

  // อ่านระยะห่างใบหน้า
  float eyeDist  = getDistance(TRIG_EYE, ECHO_EYE);

  // --- ส่วนแก้ไข TEMT6000 ---
  int lightRaw = analogRead(PIN_LIGHT);
  float voltage = (lightRaw / 4095.0) * 3.3;                // แปลงค่า ADC เป็นแรงดัน
  float microAmps = (voltage / 10000.0) * 1000000.0;       // I = V/R (R=10k), แปลงเป็น uA
  float lux = microAmps * 0.5;                             // สูตร 2 uA = 1 lx (หรือ microAmps / 2.0)
  // -------------------------

  /******** EYE ********/
  if (eyeDist > 2 && eyeDist < CLOSE_THRESHOLD) {
    if (!isClose) { closeStart = millis(); isClose = true; }
    if (millis() - closeStart > CLOSE_DELAY) alertEye = true;
  } else {
    if (isClose) closeTotal += (millis() - closeStart);
    isClose = false;
  }

  /******** POSTURE ********/
  if (backData.distLow < 50) {
    if (!isSitting) { sessionStart = millis(); isSitting = true; }

    float diff = backData.distUp - backData.distLow;
    if (diff > HUNCH_DIFF) {
      if (!isHunch) { hunchStart = millis(); isHunch = true; }
      if (millis() - hunchStart > 5000) {
        hunchCount++;
        alertHunch = true;
        isHunch    = false;
        hunchStart = millis();
      }
    } else { isHunch = false; }

    if (millis() - sessionStart > WORK_LIMIT) alertWork = true;

  } else {
    if (isSitting) {
      totalWorkAccumulated += (millis() - sessionStart);
      isSitting = false;
    }
  }

  /******** LIGHT ********/
  if (lux < LIGHT_THRESHOLD) {
    alertLight = true;
    digitalWrite(LED_ALERT_3, HIGH);
  } else {
    digitalWrite(LED_ALERT_3, LOW);
  }

  /******** LCD DISPLAY (Priority) ********/
  if      (alertEye)   updateLCD("! Too Close!",    "Move back pls");
  else if (alertHunch) updateLCD("! Bad Posture!",  "Sit straight");
  else if (alertWork)  updateLCD("! Take a break!", "1hr work done");
  else if (alertLight) updateLCD("! Low Light!",    "Check lighting");
  else                 updateLCD("Status: OK",      "All good :)");

  /******** OUTPUT ********/
  digitalWrite(LED_ALERT_2, (alertEye || alertHunch || alertWork) ? HIGH : LOW);

  BeepType currentBeep = BT_NONE;
  if      (alertEye)   currentBeep = BT_EYE;
  else if (alertHunch) currentBeep = BT_HUNCH;
  else if (alertWork)  currentBeep = BT_WORK;

  handleBuzzer(currentBeep);

  /******** DEBUG ********/
  static unsigned long lastDebug = 0;
  if (millis() - lastDebug > 1000) {
    printDebug(eyeDist, lux);
    lastDebug = millis();
  }
}
