/******************** BLYNK CONFIG ********************/
#define BLYNK_TEMPLATE_ID "TMPL6TEgwENGm"
#define BLYNK_TEMPLATE_NAME "Smart Workspace"
#define BLYNK_AUTH_TOKEN "HV4QRqU0J1fyCdLAEwOCiwAECP3dBmfB"
#define BLYNK_PRINT Serial

/******************** LIBRARY ********************/
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <Wire.h>
#include <LiquidCrystal_PCF8574.h>

/******************** WIFI ********************/
char ssid[] = "Redmi";
char pass[] = "asdfghjkl";

/******************** PIN CONFIG ********************/
#define TRIG_EYE      5
#define ECHO_EYE      18
#define TRIG_BACK_UP  32
#define ECHO_BACK_UP  33
#define TRIG_BACK_LOW 25
#define ECHO_BACK_LOW 26
#define PIN_LIGHT     34
#define BUZZER        13
#define LED_SYS       2
#define LED_ALERT_2   4
#define LED_ALERT_3   15
#define BTN           27

/******************** LCD ********************/
LiquidCrystal_PCF8574 lcd(0x27);

/******************** SYSTEM STATE ********************/
bool systemActive = false;

bool isSitting   = false;
unsigned long sessionStart          = 0;
unsigned long totalWorkAccumulated  = 0;

bool isClose = false;
unsigned long closeStart = 0;
unsigned long closeTotal = 0;

bool isHunch = false;
unsigned long hunchStart = 0;
unsigned long hunchCount = 0;

unsigned long lastSend = 0;

/******************** ALERT FLAGS ********************/
bool alertEye   = false;
bool alertHunch = false;
bool alertLight = false;
bool alertWork  = false;

/******************** THRESHOLD ********************/
#define CLOSE_THRESHOLD 40
#define CLOSE_DELAY     3000
#define HUNCH_DIFF      15
#define LIGHT_THRESHOLD 500
#define WORK_LIMIT      3600000UL

/******************** LCD SYSTEM ********************/
String lastL1 = "", lastL2 = "";

/******************** BUZZER ********************/
enum BeepType { NONE, EYE, HUNCH, WORK };

// ===== ตรวจสอบก่อนใช้งาน =====
// ถ้า buzzer เป็นแบบ ACTIVE  → เปลี่ยน ACTIVE_BUZZER เป็น true
// ถ้า buzzer เป็นแบบ PASSIVE → เปลี่ยน ACTIVE_BUZZER เป็น false
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
    case EYE:
      // บีบสั้น 1 ครั้ง (ใกล้จอ)
      beepOnce(2000, 300);
      delay(300);
      break;

    case HUNCH:
      // บีบสั้น 2 ครั้ง (หลังค่อม)
      beepOnce(1500, 150);
      delay(150);
      beepOnce(1500, 150);
      delay(150);
      break;

    case WORK:
      // บีบยาว 1 ครั้ง (ทำงานนานเกิน)
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

/******************** LCD ********************/
void updateLCD(String l1, String l2) {
  if (l1 == lastL1 && l2 == lastL2) return;
  lcd.setCursor(0, 0);
  lcd.print("                ");
  lcd.setCursor(0, 0);
  lcd.print(l1);
  lcd.setCursor(0, 1);
  lcd.print("                ");
  lcd.setCursor(0, 1);
  lcd.print(l2);
  lastL1 = l1;
  lastL2 = l2;
}

/******************** SERIAL DEBUG ********************/
void printDebug(float eyeDist, float backUp, float backLow, float lux) {
  Serial.println("====== SYSTEM STATUS ======");
  Serial.print("System: ");      Serial.println(systemActive ? "ON" : "OFF");
  Serial.print("Eye Dist: ");    Serial.print(eyeDist);  Serial.println(" cm");
  Serial.print("BackUp: ");      Serial.print(backUp);
  Serial.print(" | BackLow: ");  Serial.println(backLow);
  Serial.print("Light: ");       Serial.println(lux);
  Serial.print("Sitting: ");     Serial.println(isSitting ? "YES" : "NO");
  Serial.print("Close: ");       Serial.println(isClose   ? "YES" : "NO");
  Serial.print("HunchCount: ");  Serial.println(hunchCount);
  Serial.print("Alert: ");
  if (alertEye)   Serial.print("[EYE] ");
  if (alertHunch) Serial.print("[HUNCH] ");
  if (alertLight) Serial.print("[LIGHT] ");
  if (alertWork)  Serial.print("[WORK] ");
  if (!alertEye && !alertHunch && !alertLight && !alertWork)
    Serial.print("NONE");
  Serial.println("\n===========================\n");
  delay(1000);
}

/******************** SETUP ********************/
void setup() {
  Serial.begin(9600);
  Serial.println("System Booting...");

  // LCD
  Wire.begin();
  lcd.begin(16, 2);
  lcd.setBacklight(255);
  updateLCD("Booting...", "Please wait");

  // Pins
  pinMode(TRIG_EYE,      OUTPUT); digitalWrite(TRIG_EYE,      LOW);
  pinMode(ECHO_EYE,      INPUT);
  pinMode(TRIG_BACK_UP,  OUTPUT); digitalWrite(TRIG_BACK_UP,  LOW);
  pinMode(ECHO_BACK_UP,  INPUT);
  pinMode(TRIG_BACK_LOW, OUTPUT); digitalWrite(TRIG_BACK_LOW, LOW);
  pinMode(ECHO_BACK_LOW, INPUT);
  pinMode(BUZZER,        OUTPUT); digitalWrite(BUZZER,        LOW);
  pinMode(LED_SYS,       OUTPUT); digitalWrite(LED_SYS,       LOW);
  pinMode(LED_ALERT_2,   OUTPUT); digitalWrite(LED_ALERT_2,   LOW);
  pinMode(LED_ALERT_3,   OUTPUT); digitalWrite(LED_ALERT_3,   LOW);
  pinMode(BTN,           INPUT_PULLUP);

  // ทดสอบ Buzzer ตอน boot
  Serial.println("Testing buzzer...");
  beepOnce(1000, 200);
  delay(100);
  beepOnce(1000, 200);
  Serial.println("Buzzer test done");

  // WiFi + Blynk
  updateLCD("Connecting", "WiFi...");
  WiFi.begin(ssid, pass);
  unsigned long t = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t < 10000) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi Connected!");
    updateLCD("WiFi: OK", "Blynk connecting");
    Blynk.config(BLYNK_AUTH_TOKEN);
    Blynk.connect(3000);
  } else {
    Serial.println("\nWiFi Failed - Offline mode");
    updateLCD("WiFi: FAILED", "Offline mode");
    delay(1500);
  }

  updateLCD("System: OFF", "Press BTN");
  Serial.println("Boot complete.");
}

/******************** LOOP ********************/
void loop() {
  if (Blynk.connected()) Blynk.run();

  /******** BUTTON ********/
  static bool lastBtn = HIGH;
  bool nowBtn = digitalRead(BTN);

  if (lastBtn == HIGH && nowBtn == LOW) {
    delay(50); // debounce
    systemActive = !systemActive;
    digitalWrite(LED_SYS, systemActive);
    Serial.print("System: "); Serial.println(systemActive ? "ON" : "OFF");

    if (systemActive) {
      updateLCD("System: ON", "Monitoring...");
      beepOnce(1500, 100); // บีบ 1 ครั้งตอนเปิด
    } else {
      updateLCD("System: OFF", "Good Bye!");
      digitalWrite(BUZZER,      LOW);
      digitalWrite(LED_ALERT_2, LOW);
      digitalWrite(LED_ALERT_3, LOW);
      if (!ACTIVE_BUZZER) noTone(BUZZER);
    }
    delay(200);
  }
  lastBtn = nowBtn;

  if (!systemActive) return;

  /******** SENSOR READ ********/
  alertEye = alertHunch = alertLight = alertWork = false;

  float eyeDist = getDistance(TRIG_EYE, ECHO_EYE);
  delay(30);
  float backUp  = getDistance(TRIG_BACK_UP,  ECHO_BACK_UP);
  delay(30);
  float backLow = getDistance(TRIG_BACK_LOW, ECHO_BACK_LOW);

  int   lightRaw = analogRead(PIN_LIGHT);
  float lux      = (lightRaw / 4095.0) * 3.3 * 1000.0;

  /******** EYE DISTANCE ********/
  if (eyeDist > 2 && eyeDist < CLOSE_THRESHOLD) {
    if (!isClose) {
      closeStart = millis();
      isClose    = true;
    }
    if (millis() - closeStart > CLOSE_DELAY) {
      alertEye = true;
    }
  } else {
    if (isClose) closeTotal += (millis() - closeStart);
    isClose = false;
  }

  /******** POSTURE ********/
  if (backLow < 50) {
    if (!isSitting) {
      sessionStart = millis();
      isSitting    = true;
    }

    float diff = backUp - backLow;
    if (diff > HUNCH_DIFF) {
      if (!isHunch) {
        hunchStart = millis();
        isHunch    = true;
      }
      if (millis() - hunchStart > 5000) {
        hunchCount++;
        alertHunch = true;
        isHunch    = false;
        hunchStart = millis();
      }
    } else {
      isHunch = false;
    }

    if (millis() - sessionStart > WORK_LIMIT) {
      alertWork = true;
    }

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

  /******** LCD UPDATE ********/
  if (alertEye) {
    updateLCD("! Too Close!", "Move back pls");
  } else if (alertHunch) {
    updateLCD("! Bad Posture!", "Sit straight");
  } else if (alertWork) {
    updateLCD("! Take a break!", "1hr work done");
  } else if (alertLight) {
    updateLCD("! Low Light!", "Check lighting");
  } else {
    updateLCD("Status: OK", "All good :)");
  }

  /******** OUTPUT ********/
  digitalWrite(LED_ALERT_2, (alertEye || alertHunch || alertWork) ? HIGH : LOW);

  BeepType currentBeep = NONE;
  if      (alertEye)   currentBeep = EYE;
  else if (alertHunch) currentBeep = HUNCH;
  else if (alertWork)  currentBeep = WORK;

  handleBuzzer(currentBeep);

  /******** DEBUG ********/
  printDebug(eyeDist, backUp, backLow, lux);

  /******** BLYNK ********/
  if (millis() - lastSend > 5000 && Blynk.connected()) {
    lastSend = millis();

    unsigned long currentTotalWork =
      totalWorkAccumulated + (isSitting ? (millis() - sessionStart) : 0);
    unsigned long currentCloseTotal =
      closeTotal + (isClose ? (millis() - closeStart) : 0);

    Blynk.virtualWrite(V0, currentTotalWork  / 60000UL);
    Blynk.virtualWrite(V1, currentCloseTotal / 60000UL);
    Blynk.virtualWrite(V2, hunchCount);
    Blynk.virtualWrite(V3, alertEye   ? 1 : 0);
    Blynk.virtualWrite(V4, alertHunch ? 1 : 0);
    Blynk.virtua
