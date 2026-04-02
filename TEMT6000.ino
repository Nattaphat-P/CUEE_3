/***************************************************
 * TEMT6000 Light Sensor Test (ESP32)
 * มาตรฐานการคำนวณ: 2 uA = 1 lux
 * https://devices.esphome.io/devices/temt6000/
 * https://github.com/CraftzAdmin/esp32/blob/main/Sensors/TEMT6000/README.md
 * Pin: GPIO 34 (Analog)
 ***************************************************/

#define LIGHT_SENSOR_PIN 34

void setup() {
  Serial.begin(115200);
  pinMode(LIGHT_SENSOR_PIN, INPUT);
  Serial.println("--- TEMT6000 Lux Test (2uA = 1lx) ---");
}

void loop() {
  // 1. อ่านค่า Raw จาก ADC (0 - 4095 สำหรับ ESP32)
  int rawValue = analogRead(LIGHT_SENSOR_PIN);

  // 2. แปลงค่าเป็นแรงดันไฟฟ้า (Voltage)
  // ESP32 ADC ทำงานที่ 3.3V
  float voltage = (rawValue / 4095.0) * 3.3;

  // 3. คำนวณกระแส (Current) ในหน่วย Microamps (uA)
  // สูตร: I = V / R โดยที่ R บนโมดูลส่วนใหญ่คือ 10,000 Ohm (10k)
  float microAmps = (voltage / 10000.0) * 1000000.0;
  
  // 4. แปลง uA เป็น Lux ตามมาตรฐาน 2 uA = 1 lux
  float lux = microAmps * 2.0;

  // แสดงผลออกทาง Serial Monitor
  Serial.print("Raw: ");
  Serial.print(rawValue);
  Serial.print(" | Voltage: ");
  Serial.print(voltage, 2);
  Serial.print("V | Lux: ");
  Serial.print(lux, 2);
  Serial.println(" lx");

  delay(500); // อ่านค่าทุกๆ 0.5 วินาที
}
