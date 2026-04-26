#define BUZZER_PIN 33

void setup() {
  tone(BUZZER_PIN, 4000);  // 4 kHz tone
  delay(4000);             // play for 4 seconds
  noTone(BUZZER_PIN);      // stop the sound
}

void loop() {
}
