/*
  Arduino Uno - HC-SR04 obstacle warning
  TRIG -> D9
  ECHO -> D10
  Buzzer (+) -> D6
  LED anode -> D6 through 220 ohm resistor
  Buzzer (-), LED cathode -> GND

  The buzzer and LED share ONE output pin (D6).
  This Uno is independent of the ESP32-CAM; no communication is used.
*/

#define TRIG_PIN 9
#define ECHO_PIN 10
#define ALERT_PIN 6

const float OBSTACLE_LIMIT_CM = 20.0;

void setup() {
  Serial.begin(9600);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(ALERT_PIN, OUTPUT);

  digitalWrite(TRIG_PIN, LOW);
  digitalWrite(ALERT_PIN, LOW);

  Serial.println(F("Arduino Uno obstacle warning started"));
}

void loop() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  unsigned long duration = pulseIn(ECHO_PIN, HIGH, 30000UL);

  if (duration == 0) {
    digitalWrite(ALERT_PIN, LOW);
    Serial.println(F("No echo / out of range"));
  } else {
    float distanceCm = duration * 0.0343f / 2.0f;

    Serial.print(F("Distance: "));
    Serial.print(distanceCm, 1);
    Serial.println(F(" cm"));

    if (distanceCm <= OBSTACLE_LIMIT_CM) {
      digitalWrite(ALERT_PIN, HIGH);
      Serial.println(F("OBSTACLE: LED and buzzer ON"));
    } else {
      digitalWrite(ALERT_PIN, LOW);
    }
  }

  delay(200);
}
