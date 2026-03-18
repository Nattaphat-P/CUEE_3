ESP32 อ่านค่า Analog (0–4095) แล้วแปลงเป็น Lux แบบประมาณ

#define TEMT_PIN 34   // GPIO34 (Analog only)

void setup() {
  Serial.begin(115200);
// ตั้งค่าการลดทอนเป็น 11dB (ทำให้อ่านแรงดันได้ตั้งแต่ 0-3.3V)
  analogSetAttenuation(ADC_11db);
}

void loop() {
  int raw = analogRead(TEMT_PIN);

  // แปลงเป็นแรงดัน (ESP32 = 3.3V, 12-bit)
  float voltage = raw * (3.3 / 4095.0);

  // TEMT6000: ประมาณ 1V ≈ 1000 lux (ค่าโดยประมาณ)
  float lux = voltage * 1000.0;

  Serial.print("Raw: ");
  Serial.print(raw);
  Serial.print(" | Voltage: ");
  Serial.print(voltage);
  Serial.print(" V | Lux: ");
  Serial.println(lux);

  delay(500);
}
