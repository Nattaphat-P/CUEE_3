/******************** BLYNK CONFIG ********************/
// login เข้าแอป

#define BLYNK_TEMPLATE_ID "TMPL6TEgwENGm" 
// ID ของ template ที่เราสร้างใน Blynk (เหมือน project ID)

#define BLYNK_TEMPLATE_NAME "Smart Workspace"
// ชื่อโปรเจค (เอาไว้โชว์ในระบบ)

#define BLYNK_AUTH_TOKEN "ใส่tokenตรงนี้นะจ๊ะ"
// ถ้า token ผิดจะต่อ Blynk ไม่ติด

#define BLYNK_PRINT Serial
// ให้ Blynk print log ออก Serial Monitor (ใช้ debug ได้)


/******************** LIBRARY ********************/
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

/******************** WIFI ********************/
char ssid[] = "Redmi";   // ชื่อ WiFi
char pass[] = "asdfghjkl"; // รหัส WiFi

/******************** PIN CONFIG ********************/
// อุปกรณ์แต่ละตัวต่อกับ pin ไหน

// ===== SENSOR ระยะ (Ultrasonic) =====

// 1. ตัวนี้ไว้ตรวจ "หน้าใกล้จอ"
#define TRIG_EYE 5
#define ECHO_EYE 18

// 2. ตัวนี้ไว้ดู "หลังส่วนบน"
#define TRIG_BACK_UP 12
#define ECHO_BACK_UP 14

// 3. ตัวนี้ไว้ดู "หลังส่วนล่าง"
#define TRIG_BACK_LOW 25
#define ECHO_BACK_LOW 26

// Logic สำคัญ:
// เราใช้ 2 sensor หลัง → เพื่อ "เทียบความต่าง"
// ถ้าหลังงอ → ระยะบนกับล่างจะต่างกัน


// ===== SENSOR แสง =====
#define PIN_LIGHT 34
// เป็น analog → อ่านค่า 0-4095


// ===== OUTPUT =====
#define BUZZER 13      // เสียงเตือน
#define LED_SYS 2      // บอกว่าระบบ ON/OFF
#define LED_ALERT_2 4  // เตือน posture / eye / work
#define LED_ALERT_3 15 // เตือนแสง


// ===== INPUT =====
#define BTN 27
// ปุ่มกด (ใช้ INPUT_PULLUP → กด = LOW/0)


// ================= LCD =================
LiquidCrystal_I2C lcd(0x27, 16, 2);
// address = 0x27
// ขนาด 16 ตัวอักษร 2 แถว


/******************** SYSTEM STATE ********************/
// ตัวแปรกลุ่มนี้ = "จำสถานะของระบบ"

bool systemActive = false;
// false = ปิดระบบ
// true = เปิดระบบ


/******************** WORK (เวลานั่ง) ********************/
bool isSitting = false;
// ตอนนี้ผู้ใช้ "นั่งอยู่ไหม"

unsigned long sessionStart = 0;
// เวลาที่เริ่มนั่งครั้งล่าสุด

unsigned long totalWorkAccumulated = 0;
// เวลานั่งสะสมทั้งหมด (รวมทุก session)


// Logic:
// - เริ่มนั่ง → จำเวลา
// - ลุก → เอาเวลาที่นั่งไปบวกสะสม


/******************** EYE (เข้าใกล้จอ) ********************/
unsigned long closeStart = 0;
// เวลาเริ่ม "เข้าใกล้จอ"

unsigned long closeTotal = 0;
// เวลาที่อยู่ใกล้จอสะสม

bool isClose = false;
// ตอนนี้อยู่ใกล้จอไหม


// Logic:
// ถ้าเข้าใกล้ → เริ่มจับเวลา
// ถ้าออก → เอาเวลามาบวกสะสม


/******************** HUNCH (หลังโค้งงอไหม) ********************/
unsigned long hunchStart = 0;
// เวลาเริ่มค่อม

unsigned long hunchCount = 0;
// จำนวนครั้งที่ค่อม

bool isHunch = false;
// ตอนนี้ค่อมอยู่ไหม

unsigned long lastHunchAlert = 0;
// เวลาที่เตือนล่าสุด (กันเตือนรัว)


// Logic:
// - ถ้าค่อมเกิน 5 วิ → นับ 1 ครั้ง
// - แต่ต้องไม่เตือนซ้ำภายใน 15 วิ (cooldown)


/******************** BLYNK ********************/
unsigned long lastSend = 0;
// ใช้จับเวลา "ส่งข้อมูลทุก 5 วิ"


/******************** ALERT FLAGS ********************/
// flag = ตัวบอกว่า "ตอนนี้ควรเตือนไหม"

bool alertEye = false;
bool alertHunch = false;
bool alertLight = false;
bool alertWork = false;

// Logic:
// แยก detection กับ output ออกจากกัน
// → ทำให้จัด priority ได้ง่าย


/******************** THRESHOLD ********************/
// ค่าเกณฑ์ทั้งหมดของระบบ

#define CLOSE_THRESHOLD 40
// ถ้าตาน้อยกว่า 40 cm = ใกล้เกิน

#define CLOSE_DELAY 3000
// ต้องใกล้เกิน 3 วิ ถึงเตือน (กัน false alarm)

#define HUNCH_DIFF 15
// ถ้าระยะบน - ล่าง > 15 = ค่อม

#define HUNCH_COOLDOWN 15000
// เตือนค่อมแล้วต้องรอ 15 วิ

#define LIGHT_THRESHOLD 500
// ต่ำกว่า 500 = มืด

#define WORK_LIMIT 3600000
// 1 ชั่วโมง → เตือนพัก


/******************** LCD SYSTEM ********************/
String lastL1 = "", lastL2 = "";
// เก็บค่าล่าสุด → ป้องกันเขียนซ้ำ

unsigned long lastLCDSwitch = 0;
int lcdIndex = 0;

#define LCD_INTERVAL 2000
// เปลี่ยนข้อความทุก 2 วิ


// Logic:
// ถ้ามีหลาย alert → สลับโชว์ทีละอัน


/******************** BUZZER PATTERN ********************/
// enum = กำหนดประเภทเสียง

enum BeepType { NONE, EYE, HUNCH, WORK };

BeepType currentBeep = NONE;

unsigned long beepStart = 0;
// ใช้สร้าง "pattern เสียง" โดยไม่ใช้ delay


/******************** BUZZER LOGIC ********************/
void handleBuzzer(BeepType type) {

  unsigned long now = millis();
  // millis = เวลาที่บอร์ดทำงานมา (ms)

  // Logic สำคัญ:
  // ใช้ millis แทน delay ทำให้โปรแกรม "ไม่ค้าง"

  switch (type) {

    case EYE:
      // เสียงสั้นๆ วนไปเรื่อย
      if (now - beepStart < 300) digitalWrite(BUZZER, HIGH);
      else if (now - beepStart < 600) digitalWrite(BUZZER, LOW);
      else beepStart = now;
      break;

    case HUNCH:
      // ติ๊ด 2 ที
      if (now - beepStart < 200) digitalWrite(BUZZER, HIGH);
      else if (now - beepStart < 400) digitalWrite(BUZZER, LOW);
      else if (now - beepStart < 600) digitalWrite(BUZZER, HIGH);
      else if (now - beepStart < 800) digitalWrite(BUZZER, LOW);
      else beepStart = now;
      break;

    case WORK:
      // เสียงยาว
      if (now - beepStart < 800) digitalWrite(BUZZER, HIGH);
      else if (now - beepStart < 1600) digitalWrite(BUZZER, LOW);
      else beepStart = now;
      break;

    default:
      digitalWrite(BUZZER, LOW);
      break;
  }
}


/******************** ULTRASONIC ********************/
float getDistance(int trig, int echo) {

  // ส่ง pulse ออกไป
  digitalWrite(trig, LOW); 
  delayMicroseconds(2);

  digitalWrite(trig, HIGH); 
  delayMicroseconds(10);
  digitalWrite(trig, LOW);

  // รอ echo กลับมา
  long duration = pulseIn(echo, HIGH, 25000);

  // ถ้าไม่มี echo → ถือว่าไกลมาก
  if (duration == 0) return 400;

  // แปลงเวลา → ระยะ (cm)
  return duration * 0.034 / 2;
}


/******************** LCD ********************/
void updateLCD(String l1, String l2) {

  // เขียนเฉพาะตอนข้อความเปลี่ยน
  if (l1 != lastL1 || l2 != lastL2) {

    // ล้าง + เขียนใหม่
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
}


/******************** LOOP (หัวใจระบบ) ********************/
void loop() {

  // ให้ Blynk ทำงานตลอด
  Blynk.run();


  /******** BUTTON ********/
  // Logic: edge detection (HIGH → LOW = กด)

  static bool lastBtn = HIGH;
  bool nowBtn = digitalRead(BTN);

  if (lastBtn == HIGH && nowBtn == LOW) {

    // toggle ระบบ
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

    delay(250); // debounce กันเด้ง
  }

  lastBtn = nowBtn;

  // ถ้าระบบปิด → หยุดทำงานทันที
  if (!systemActive) return;


  /******** RESET FLAG ********/
  // ทุก loop ต้อง reset แล้วคำนวณใหม่
  alertEye = false;
  alertHunch = false;
  alertLight = false;
  alertWork = false;


  /******** READ SENSOR ********/
  // อ่านค่าทุก sensor

  float eyeDist = getDistance(TRIG_EYE, ECHO_EYE);
  delay(50);

  float backUp = getDistance(TRIG_BACK_UP, ECHO_BACK_UP);
  delay(50);

  float backLow = getDistance(TRIG_BACK_LOW, ECHO_BACK_LOW);

  int lightRaw = analogRead(PIN_LIGHT);

  float lux = (lightRaw / 4095.0) * 3.3 * 1000;


  /******** EYE LOGIC ********/
  // ถ้าเข้าใกล้จอ

  if (eyeDist < CLOSE_THRESHOLD && eyeDist > 2) {

    if (!isClose) {
      // เพิ่งเริ่มเข้าใกล้ → จับเวลา
      closeStart = millis();
      isClose = true;
    }

    // ถ้าอยู่นานเกิน threshold ให้ alert
    if (millis() - closeStart > CLOSE_DELAY) {
      alertEye = true;
    }

  } else {

    // ถ้าเคยใกล้ → เอาเวลามาบวก
    if (isClose) {
      closeTotal += (millis() - closeStart);
    }

    isClose = false;
  }


  /******** POSTURE + WORK ********/
  // ใช้ sensor ตัวล่าง (3.) ตรวจว่า "นั่งอยู่ไหม"

  if (backLow < 50) {

    if (!isSitting) {
      sessionStart = millis();
      isSitting = true;
    }

    // ===== HUNCH =====
    if (backUp - backLow > HUNCH_DIFF) {

      if (!isHunch) {
        hunchStart = millis();
        isHunch = true;
      }

      if (millis() - hunchStart > 5000 &&
          millis() - lastHunchAlert > HUNCH_COOLDOWN) {

        hunchCount++;
        alertHunch = true;
        lastHunchAlert = millis();
        isHunch = false;
      }

    } else {
      isHunch = false;
    }

    // ===== WORK =====
    if (millis() - sessionStart > WORK_LIMIT) {
      alertWork = true;
    }

  } else {

    // ลุกออก
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


  /******** PRIORITY SYSTEM ********/
  // รวม alert ทั้งหมด → เอามาเรียงแสดง

  String msg1[4];
  String msg2[4];
  int count = 0;

  // เก็บข้อความทุก alert
  // count = จำนวน alert ที่เกิด

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


  /******** OUTPUT ********/
  if (count > 0) {

    // เลือกเสียงตาม "priority"
    if (alertEye) currentBeep = EYE;
    else if (alertHunch) currentBeep = HUNCH;
    else if (alertWork) currentBeep = WORK;
    else currentBeep = NONE;

    // LED
    if (alertEye || alertHunch || alertWork)
      digitalWrite(LED_ALERT_2, HIGH);
    else
      digitalWrite(LED_ALERT_2, LOW);

    // เสียง
    handleBuzzer(currentBeep);

    // สลับข้อความ LCD
    if (millis() - lastLCDSwitch > LCD_INTERVAL) {
      lastLCDSwitch = millis();
      lcdIndex++;
      if (lcdIndex >= count) lcdIndex = 0;
    }

    updateLCD(msg1[lcdIndex], msg2[lcdIndex]);

  } else {
    // ไม่มี alert → ปิดทุกอย่าง
    digitalWrite(LED_ALERT_2, LOW);
    currentBeep = NONE;
    handleBuzzer(NONE);
    lcdIndex = 0;
  }


  /******** BLYNK ********/
  if (millis() - lastSend > 5000) {

    lastSend = millis();

    // รวมเวลานั่ง
    unsigned long currentTotalWork =
      totalWorkAccumulated + (isSitting ? (millis() - sessionStart) : 0);

    // รวมเวลาใกล้จอ
    unsigned long currentCloseTotal =
      closeTotal + (isClose ? (millis() - closeStart) : 0);

    // ส่งไปBlynk
    Blynk.virtualWrite(V0, currentTotalWork / 60000);
    Blynk.virtualWrite(V1, currentCloseTotal / 60000);
    Blynk.virtualWrite(V2, hunchCount);
  }
}
