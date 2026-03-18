/******************** BLYNK CONFIG ********************/
#define BLYNK_TEMPLATE_ID "TMPL6TEgwENGm"
#define BLYNK_TEMPLATE_NAME "Smart Workspace"
#define BLYNK_AUTH_TOKEN "ใส่tokenตรงนี้นะจ๊ะ"
#define BLYNK_PRINT Serial

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

#define PIN_LIGHT 34

#define BUZZER 13
#define LED_SYS 2
#define LED_ALERT_2 4
#define LED_ALERT_3 15

#define BTN 27

/******************** LCD ********************/
LiquidCrystal_I2C lcd(0x27, 16, 2);

/******************** SYSTEM STATE ********************/
bool systemActive = false;

// Work
bool isSitting = false;
unsigned long sessionStart = 0;
unsigned long totalWorkAccumulated = 0;

// Eye
unsigned long closeStart = 0;
unsigned long closeTotal = 0;
bool isClose = false;

// Hunch
unsigned long hunchStart = 0;
unsigned long hunchCount = 0;
bool isHunch = false;
unsigned long lastHunchAlert = 0;   // cooldown

// Blynk
unsigned long lastSend = 0;

/******************** ALERT FLAGS ********************/
bool alertEye = false;
bool alertHunch = false;
bool alertLight = false;
bool alertWork = false;

/******************** THRESHOLD ********************/
#define CLOSE_THRESHOLD 40
#define CLOSE_DELAY 3000

#define HUNCH_DIFF 15
#define HUNCH_COOLDOWN 15000   // 15 วิ

#define LIGHT_THRESHOLD 500

#define WORK_LIMIT 3600000

/******************** LCD SYSTEM ********************/
String lastL1 = "", lastL2 = "";
unsigned long lastLCDSwitch = 0;
int lcdIndex = 0;
#define LCD_INTERVAL 2000

/******************** BUZZER PATTERN ********************/
enum BeepType { NONE, EYE, HUNCH, WORK };
BeepType currentBeep = NONE;

unsigned long beepStart = 0;

void handleBuzzer(BeepType type) {
  unsigned long now = millis();

  switch (type) {

    case EYE: // ติ๊ดเร็ว
      if (now - beepStart < 300) digitalWrite(BUZZER, HIGH);
      else if (now - beepStart < 600) digitalWrite(BUZZER, LOW);
      else beepStart = now;
      break;

    case HUNCH: // ติ๊ด 2 ที
      if (now - beepStart < 200) digitalWrite(BUZZER, HIGH);
      else if (now - beepStart < 400) digitalWrite(BUZZER, LOW);
      else if (now - beepStart < 600) digitalWrite(BUZZER, HIGH);
      else if (now - beepStart < 800) digitalWrite(BUZZER, LOW);
      else beepStart = now;
      break;

    case WORK: // ติ๊ดยาว
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
  digitalWrite(trig, LOW); 
  delayMicroseconds(2);

  digitalWrite(trig, HIGH); 
  delayMicroseconds(10);
  digitalWrite(trig, LOW);

  long duration = pulseIn(echo, HIGH, 25000);
  if (duration == 0) return 400;

  return duration * 0.034 / 2;
}

/******************** LCD UPDATE ********************/
void updateLCD(String l1, String l2) {
  if (l1 != lastL1 || l2 != lastL2) {

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

/******************** SETUP ********************/
void setup() {
  Serial.begin(115200);

  lcd.init();
  lcd.backlight();

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

  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
}

/******************** LOOP ********************/
void loop() {
  Blynk.run();

  /******** BUTTON ********/
  static bool lastBtn = HIGH;
  bool nowBtn = digitalRead(BTN);

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

    delay(250);
  }
  lastBtn = nowBtn;

  if (!systemActive) return;

  /******** RESET FLAGS ********/
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

  /******** EYE ********/
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

  /******** POSTURE + WORK ********/
  if (backLow < 50) {

    if (!isSitting) {
      sessionStart = millis();
      isSitting = true;
    }

    // HUNCH
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

    // WORK
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

  /******** PRIORITY ********/
  String msg1[4];
  String msg2[4];
  int count = 0;

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

    // เลือกเสียงตาม priority
    if (alertEye) currentBeep = EYE;
    else if (alertHunch) currentBeep = HUNCH;
    else if (alertWork) currentBeep = WORK;
    else currentBeep = NONE;

    // LED
    if (alertEye || alertHunch || alertWork)
      digitalWrite(LED_ALERT_2, HIGH);
    else
      digitalWrite(LED_ALERT_2, LOW);

    // buzzer
    handleBuzzer(currentBeep);

    // LCD switch
    if (millis() - lastLCDSwitch > LCD_INTERVAL) {
      lastLCDSwitch = millis();
      lcdIndex++;
      if (lcdIndex >= count) lcdIndex = 0;
    }

    updateLCD(msg1[lcdIndex], msg2[lcdIndex]);

  } else {
    digitalWrite(LED_ALERT_2, LOW);
    currentBeep = NONE;
    handleBuzzer(NONE);
    lcdIndex = 0;
  }

  /******** BLYNK ********/
  if (millis() - lastSend > 5000) {
    lastSend = millis();

    unsigned long currentTotalWork =
      totalWorkAccumulated + (isSitting ? (millis() - sessionStart) : 0);

    unsigned long currentCloseTotal =
      closeTotal + (isClose ? (millis() - closeStart) : 0);

    Blynk.virtualWrite(V0, currentTotalWork / 60000);
    Blynk.virtualWrite(V1, currentCloseTotal / 60000);
    Blynk.virtualWrite(V2, hunchCount);
  }
}
