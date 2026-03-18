const int speaker = 8;   // ขาที่ต่อลำโพง

void setup() {
}

void loop() {
  tone(speaker, 1000);   // ส่งคลื่น 1000 Hz
  delay(500);            // ดัง 0.5 วิ

  noTone(speaker);       // หยุดเสียง
  delay(500);            // เงียบ 0.5 วิ
}
<img width="550" height="557" alt="image" src="https://github.com/user-attachments/assets/4914df52-23f4-4b14-92f7-3372056798d1" />

---------------------------------------------------------------------------------------------------------------------------------

const int trig = 10;
const int echo = 9;

void setup() {
  Serial.begin(9600);
  pinMode(trig, OUTPUT);
  pinMode(echo, INPUT);
}

void loop() {
  digitalWrite(trig, LOW);
  delayMicroseconds(2);

  digitalWrite(trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig, LOW);

  long duration = pulseIn(echo, HIGH);
  float distance = duration * 0.034 / 2;

  Serial.println(distance); //วิธี1
  Serial.print("Distance in CM: ");
  Serial.println(duration / 58); //วิธี2
  delay(500);
}

<img width="529" height="458" alt="image" src="https://github.com/user-attachments/assets/ef9061b9-caef-4a37-a4a4-470f9213813f" />
