const int BUZZER_PIN = 32;

void setup() {
  // Attach PWM to pin and set frequency + resolution
  ledcAttach(BUZZER_PIN, 4000, 8);  // pin, freq, resolution
}

void loop() {
  ledcWrite(BUZZER_PIN, 128); // 50% duty
  delay(300);
  ledcWrite(BUZZER_PIN, 0);   // off
  delay(700);
}
