// Standalone ESP32 Epee (no Wi-Fi / no ESP-NOW)
// Based on the structure of the internet code: debounce + score() function

const int LED_PIN = 26;   // your LED GPIO
const int TIP_PIN = 27;   // LINE_A from your weapon/tip

// Timing (ms)
const unsigned long DEBOUNCE_MS   = 10;    // like your debounceDelay
const unsigned long MIN_PRESS_MS  = 20;    // require stable press this long to count as hit
const unsigned long HIT_ON_MS     = 2500;  // LED on duration after hit
const unsigned long REARM_MS      = 50;    // small delay after release before re-arming

// Debounce state (similar idea to your code)
int lastRaw = HIGH;
int stableState = HIGH;
unsigned long lastDebounceTime = 0;

// Hit/score state
bool armed = true;
unsigned long pressStartMs = 0;
unsigned long lastReleaseMs = 0;

void score() {
  digitalWrite(LED_PIN, HIGH);
  Serial.println("Hit Registered");
  delay(HIT_ON_MS);
  digitalWrite(LED_PIN, LOW);
}

void setup() {
  Serial.begin(115200);
  delay(300);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // Internal pull-up: idle HIGH, pressed LOW (tip shorts LINE_A to GND/LINE_B)
  pinMode(TIP_PIN, INPUT_PULLUP);

  Serial.println("Standalone epee ready (no Wi-Fi)");
  Serial.println("Press tip to score");
}

void loop() {
  unsigned long now = millis();

  int raw = digitalRead(TIP_PIN);

  // Debounce: if it changed, reset timer
  if (raw != lastRaw) {
    lastRaw = raw;
    lastDebounceTime = now;
  }

  // If stable long enough, accept as stableState
  if ((now - lastDebounceTime) >= DEBOUNCE_MS) {
    if (raw != stableState) {
      stableState = raw;

      if (stableState == LOW) {
        // Pressed
        pressStartMs = now;
        Serial.println("Tip pressed");
      } else {
        // Released
        lastReleaseMs = now;
        Serial.println("Tip released");
      }
    }
  }

  // Rearm after release
  if (!armed && stableState == HIGH && (now - lastReleaseMs) >= REARM_MS) {
    armed = true;
  }

  // Register a hit if pressed long enough and armed
  if (armed && stableState == LOW) {
    if ((now - pressStartMs) >= MIN_PRESS_MS) {
      armed = false;     // lockout until released
      score();           // light LED + print
    }
  }

  delay(1);
}
