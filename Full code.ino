/******************** BLYNK CONFIG ********************/
#define BLYNK_TEMPLATE_ID "TMPL6TEgwENGm"
#define BLYNK_TEMPLATE_NAME "Smart Workspace"
#define BLYNK_AUTH_TOKEN "ใส่tokenตรงนี้นะจ๊ะ"
//#define BLYNK_PRINT Serial ไว้ก่อน

/******************** LIBRARY ********************/
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

/******************** WIFI ********************/
char ssid[] = "Redmi";
char pass[] = "asdfghjkl";

/******************** PIN CONFIG (ESP32) ********************/
#define TRIG_EYE 5
#define ECHO_EYE 18

#define TRIG_BACK_UP 12
#define ECHO_BACK_UP 14

#define TRIG_BACK_LOW 25
#define ECHO_BACK_LOW 26

#define PIN_LIGHT 34   // TEMT6000 (Analog)

#define BUZZER 13
#define LED_SYS 2
#define LED_ALERT_2 4
#define LED_ALERT_3 15

#define BTN 27

/******************** LCD ********************/
LiquidCrystal_I2C lcd(0x27, 16, 2);

/******************** SYSTEM STATE ********************/
bool systemActive = false;

// การนั่งทำงาน
bool isSitting = false;
unsigned long sessionStart = 0;
unsigned long totalWorkAccumulated = 0;

// การเข้าใกล้จอ
unsigned long closeStart = 0;
unsigned long closeTotal = 0;
bool isClose = false;

// หลังงอ
unsigned long hunchStart = 0;
unsigned long hunchCount = 0;
bool isHunch = false;

// ส่งข้อมูล Blynk
unsigned long lastSend = 0;

/******************** ALERT FLAGS (PRIORITY SYSTEM) ********************/
// ใช้ flag แทนการสั่ง buzzer ตรง ๆ
bool alertEye = false;    // Priority 1
bool alertHunch = false;  // Priority 2
bool alertLight = false;  // Priority 3
bool alertWork = false;   // Priority 4

/******************** THRESHOLD ********************/
#define CLOSE_THRESHOLD 40     // cm
#define CLOSE_DELAY 3000       // ms

#define HUNCH_DIFF 15          // cm
#define LIGHT_THRESHOLD 500    // lux approx

#define WORK_LIMIT 3600000     // 60 นาที

/******************** LCD SMOOTH SYSTEM ********************/
// เก็บข้อความล่าสุด (กันกระพริบ)
String lastL1 = "", lastL2 = "";

// ใช้สำหรับสลับหลายข้อความ
unsigned long lastLCDSwitch = 0;
int lcdIndex = 0;
#define LCD_INTERVAL 2000   // เปลี่ยนทุก 2 วิ

/******************** FUNCTION: ULTRASONIC ********************/
float getDistance(int trig, int echo) {
  digitalWrite(trig, LOW); 
  delayMicroseconds(2);

  digitalWrite(trig, HIGH); 
  delayMicroseconds(10);
  digitalWrite(trig, LOW);

  long duration = pulseIn(echo, HIGH, 25000); // timeout

  if (duration == 0) return 400; // ไม่มี echo

  return duration * 0.034 / 2; // cm
}

/******************** FUNCTION: LCD UPDATE (NO FLICKER) ********************/
void updateLCD(String l1, String l2) {
  // เปลี่ยนเฉพาะเมื่อข้อความต่างจากเดิม
  if (l1 != lastL1 || l2 != lastL2) {
    lcd.clear();
    lcd.setCursor(0, 0); lcd.print(l1);
    lcd.setCursor(0, 1); lcd.print(l2);

    lastL1 = l1;
    lastL2 = l2;
  }
}

/******************** SETUP ********************/
void setup() {
  Serial.begin(115200);

  lcd.init();
  lcd.backlight();

  // ตั้งค่า pin
  pinMode(TRIG_EYE, OUTPUT);
  pinMode(ECHO_EYE, INPUT);

  pinMode(TRIG_BACK_UP, OUTPUT);
  pinMode(ECHO_BACK_UP, INPUT);

  pinMode(TRIG_BACK_LOW, OUTPUT);
  pinMode(ECHO_BACK_LOW, INPUT);

  pinMode(BUZZER, OUTPUT);
  pinMode(LED_SYS, OUTPUT);
  pinMode(LED_ALERT_2, OUTPUT);
  pinMode(LED_ALERT_3, OUTPUT);

  pinMode(BTN, INPUT_PULLUP);

  // เริ่ม Blynk
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
}

/******************** LOOP ********************/
void loop() {
  Blynk.run();

  /******** BUTTON TOGGLE SYSTEM ********/
  static bool lastBtn = HIGH;
  bool nowBtn = digitalRead(BTN);

  // กดปุ่ม = toggle
  if (lastBtn == HIGH && nowBtn == LOW) {
    systemActive = !systemActive;

    digitalWrite(LED_SYS, systemActive);

    if (systemActive) {
      updateLCD("System: ON", "Monitoring...");
    } else {
      updateLCD("System: OFF", "Good Bye!");

      digitalWrite(BUZZER, LOW);
      digitalWrite(LED_ALERT_2, LOW);
      digitalWrite(LED_ALERT_3, LOW);
    }

    delay(250); // debounce
  }

  lastBtn = nowBtn;

  if (!systemActive) return;

  /******** RESET ALERT FLAGS ********/
  alertEye = false;
  alertHunch = false;
  alertLight = false;
  alertWork = false;

  /******** READ SENSOR ********/
  float eyeDist = getDistance(TRIG_EYE, ECHO_EYE);
  delay(50);

  float backUp = getDistance(TRIG_BACK_UP, ECHO_BACK_UP);
  delay(50);

  float backLow = getDistance(TRIG_BACK_LOW, ECHO_BACK_LOW);

  int lightRaw = analogRead(PIN_LIGHT);
  float lux = (lightRaw / 4095.0) * 3.3 * 1000;

  /******** 1. EYE DISTANCE ********/
  if (eyeDist < CLOSE_THRESHOLD && eyeDist > 2) {

    if (!isClose) {
      closeStart = millis();
      isClose = true;
    }

    if (millis() - closeStart > CLOSE_DELAY) {
      alertEye = true;
    }

  } else {
    if (isClose) {
      closeTotal += (millis() - closeStart);
    }
    isClose = false;
  }

  /******** 2. POSTURE + 4. WORK ********/
  if (backLow < 50) {  // มีคนนั่ง

    if (!isSitting) {
      sessionStart = millis();
      isSitting = true;
    }

    // -------- HUNCH --------
    if (backUp - backLow > HUNCH_DIFF) {

      if (!isHunch) {
        hunchStart = millis();
        isHunch = true;
      }

      if (millis() - hunchStart > 5000) {
        hunchCount++;
        alertHunch = true;
        isHunch = false;
      }

    } else {
      isHunch = false;
    }

    // -------- WORK TIME --------
    if (millis() - sessionStart > WORK_LIMIT) {
      alertWork = true;
    }

  } else {
    // ลุกจากที่นั่ง
    if (isSitting) {
      totalWorkAccumulated += (millis() - sessionStart);
      isSitting = false;
    }
  }

  /******** 3. LIGHT ********/
  if (lux < LIGHT_THRESHOLD) {
    alertLight = true;
    digitalWrite(LED_ALERT_3, HIGH);
  } else {
    digitalWrite(LED_ALERT_3, LOW);
  }

  /******** PRIORITY + MULTI DISPLAY ********/
  String msg1[4];
  String msg2[4];
  int count = 0;

  // เรียงตาม priority
  if (alertEye) {
    msg1[count] = "Warning!";
    msg2[count] = "Too Close Screen";
    count++;
  }

  if (alertHunch) {
    msg1[count] = "Fix Posture!";
    msg2[count] = "Don't Slouch";
    count++;
  }

  if (alertLight) {
    msg1[count] = "Low Light!";
    msg2[count] = "Turn on lamp";
    count++;
  }

  if (alertWork) {
    msg1[count] = "Break Time!";
    msg2[count] = "Stand Up!";
    count++;
  }

  // -------- OUTPUT --------
  if (count > 0) {

    // Buzzer: ใช้เฉพาะ event สำคัญ
    if (alertEye || alertHunch || alertWork) {
      digitalWrite(BUZZER, HIGH);
      digitalWrite(LED_ALERT_2, HIGH);
    } else {
      digitalWrite(BUZZER, LOW);
      digitalWrite(LED_ALERT_2, LOW);
    }

    // สลับข้อความ
    if (millis() - lastLCDSwitch > LCD_INTERVAL) {
      lastLCDSwitch = millis();
      lcdIndex++;
      if (lcdIndex >= count) lcdIndex = 0;
    }

    updateLCD(msg1[lcdIndex], msg2[lcdIndex]);

  } else {
    // ไม่มี alert
    digitalWrite(BUZZER, LOW);
    digitalWrite(LED_ALERT_2, LOW);
    lcdIndex = 0;
  }

  /******** BLYNK UPDATE ********/
  if (millis() - lastSend > 5000) {
    lastSend = millis();

    unsigned long currentTotalWork =
      totalWorkAccumulated + (isSitting ? (millis() - sessionStart) : 0);

    unsigned long currentCloseTotal =
      closeTotal + (isClose ? (millis() - closeStart) : 0);

    Blynk.virtualWrite(V0, currentTotalWork / 60000); // นาที
    Blynk.virtualWrite(V1, currentCloseTotal / 60000);
    Blynk.virtualWrite(V2, hunchCount);
  }
}


ส่วนที่ 1: การตรวจจับระยะหน้าจอ (Eye Distance)
ใช้ TRIG_EYE และ ECHO_EYE วัดระยะห่างระหว่างใบหน้ากับจอ
Logic: ถ้าระยะน้อยกว่า 40 cm นานเกิน 3 วินาที ระบบจะถือว่า "ใกล้เกินไป"
Output: Buzzer ดัง, LED 2 ติด, และ LCD ขึ้นเตือน
Blynk: เก็บเวลาสะสมที่สะสมจากการ "เผลอเข้าใกล้" โดยนับหน่วยเป็นนาที

ส่วนที่ 2: การตรวจจับท่านั่งหลังงอ (Posture Check)
ใช้ Ultrasonic 2 ตัวเปรียบเทียบระยะห่างจากพนักพิง
Logic: วัดระยะหลังส่วนล่าง (backLow) และหลังส่วนบน (backUp) ถ้า backUp มีค่ามากกว่า backLow เกิน 15 cm (แปลว่าไหล่ห่อห่างจากพนักพิงมากกว่าปกติ) นานเกิน 5 วินาที จะถือว่า "หลังงอ"
Output: Buzzer ดัง, LED 2 ติด, และ LCD ขึ้นเตือน
Blynk: ส่งค่า hunchCount (นับจำนวนครั้งที่โดนเตือน)

ส่วนที่ 3: ตรวจจับแสงสว่าง (Light Monitoring) (lux)
ใช้เซนเซอร์ TEMT6000 ซึ่งเป็น Analog Light Sensor
Logic: อ่านค่าจากขา 34 แปลงเป็นค่าความเข้มแสงคร่าวๆ ถ้าต่ำกว่าเกณฑ์ (500) จะเตือนให้เปิดไฟ
Output: LED 3 ติด และ LCD ขึ้นเตือน ไม่มีbuzzer

ส่วนที่ 4: การนับเวลาทำงานและคนนั่ง (Work Timer)
ใช้ backLow ตัวเดิมทำหน้าที่เป็น Occupancy Sensor
Logic: ถ้าระยะห่างน้อยกว่า 50 cm แสดงว่า "มีคนนั่ง" ให้เริ่มนับเวลา currentSessionTime ถ้าเกิน 60 นาทีให้เตือนลุก
Output: Buzzer ดัง และ LCD ขึ้นเตือน
Cumulative Time: ระบบจะจำเวลาสะสมไว้ด้วย แม้จะลุกไปแล้วกลับมานั่งใหม่เวลารวมก็จะเพิ่มขึ้นเรื่อยๆ (เหมือนไมล์รถ) 
