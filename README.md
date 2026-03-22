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

ส่วนที่5: ปุ่มเปิด/ปิด ระบบ กดปุ่มledอันแรกจะติด ระบบทำงาน กดอีกรอบ ledอันแรกจะดับ ระบบไม่ทำงาน
*แต่ละส่วน จะมีเสียงbuzzerต่างกัน* *มีการเรียงpiority outputไปlcd 1,2,3,4*
1. ใกล้ → ติ๊ดเร็ว
2. งอ → 2 ที
3. เวลา → ยาว
4. แสง → ไม่มีเสียง

Data

<img width="968" height="547" alt="image" src="https://github.com/user-attachments/assets/42921296-61e8-4e4c-b07b-d424fdc67a0b" />

<img width="1047" height="577" alt="image" src="https://github.com/user-attachments/assets/f8efc02f-da89-4fd4-83ec-0966862663e1" />

<img width="975" height="684" alt="image" src="https://github.com/user-attachments/assets/d55cd18c-de69-40b5-8e13-e6e297e15653" />

<img width="1045" height="732" alt="image" src="https://github.com/user-attachments/assets/03889d98-222f-4853-b1a5-55b00c37a7d9" />

<img width="1040" height="727" alt="image" src="https://github.com/user-attachments/assets/5165b20f-7c90-400a-9fd5-f905ad96e695" />
