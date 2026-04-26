/*
 This is an Epee Fencing Code
  Two-fencer ESP32 Epee Scorer (A pull-up, B pull-down, C = GND)
  - Valid press (A-B closes) makes B go HIGH while A stays HIGH
  - Guard/lame (A touches C=GND) makes A go LOW => prevents B rising => ignored
  - 50ms window for double touches
  - Lockout until LEDs off AND both tips released

  Wiring (per weapon):
    LINE_C -> GND
    LINE_A -> GPIO (INPUT_PULLUP or external 10k pull-up)
    LINE_B -> GPIO (INPUT_PULLDOWN or external 10k pull-down)
*/
#include <Arduino.h>

struct Weapon; 

enum SenseState {
  S_IDLE,
  S_VALID,
  S_GUARD
};

SenseState senseABvsAC(const Weapon &w);
int computeRawForDebounce(Weapon &w, SenseState sense);
bool updateWeapon(Weapon &w, unsigned long now);

// Weapon Structure Definition
struct Weapon {
  int LED_PIN;
  int LINE_A_PIN;
  int LINE_B_PIN;

  // Debounce
  int lastRaw = HIGH;
  int stableState = HIGH;
  unsigned long lastDebounceTime = 0;

  // Hit state
  bool armed = true;
  unsigned long pressStartMs = 0;
  unsigned long lastReleaseMs = 0;

  // LED hold
  bool ledOn = false;
  unsigned long ledOffAtMs = 0;

  // Guard latch
  bool guardContact = false;

  const char* name;
};

// ---------------- Timing ----------------
const unsigned long EPEE_WINDOW_MS = 50;

const unsigned long DEBOUNCE_MS   = 10;
const unsigned long MIN_PRESS_MS  = 50;    // epee min press
const unsigned long HIT_ON_MS     = 2000;  // LED on time
const unsigned long REARM_MS      = 50;    // rearm after release

// ---------------- Window / lockout ----------------
bool windowActive = false;
unsigned long windowStart = 0;
bool resultPrinted = false;

bool f1Latched = false;
bool f2Latched = false;
unsigned long f1Time = 0;
unsigned long f2Time = 0;

bool lockOutActive = false;
unsigned long lockoutUntil = 0;

// ---------------- Buzzer (LEDC PWM) ----------------
const int BUZZER_PIN = 32;
const int BUZZER_CH  = 0;
const int BUZZER_FREQ = 4000;
const int BUZZER_RES  = 8;

const unsigned long BUZZ_MS_SINGLE = 3000;
const unsigned long BUZZ_MS_DOUBLE = 3000;

bool buzOn = false;
unsigned long buzOffAt = 0;

void buzzerStart(unsigned long now, unsigned long durationMs) {
  buzOn = true;
  buzOffAt = now + durationMs;
  tone(BUZZER_PIN, 4000);
}

void buzzerUpdate(unsigned long now) {
  if (buzOn && now >= buzOffAt) {
    buzOn = false;
    noTone(BUZZER_PIN);
  }
}


// ===== Choose your pins here =====
// NOTE: In your file you had W1 {26,27,18 meaning LED=26, A=27, B=18}.
Weapon W1 { 26, 27, 18, HIGH, HIGH, 0, true, 0, 0, false, 0, false, "FENCER1" };
Weapon W2 { 33, 14, 19, HIGH, HIGH, 0, true, 0, 0, false, 0, false, "FENCER2" };

// ---------------- Sensing logic ----------------
//
// With A pull-up, B pull-down:
//
// Idle:      A=HIGH, B=LOW
// Valid hit: A=HIGH, B=HIGH   (A-B closed, B rises)
// Guard:     A=LOW,  B=LOW    (A grounded via C; B cannot rise)  => ignore
//
//enum SenseState { S_IDLE, S_VALID, S_GUARD };

SenseState senseABvsAC(const Weapon &w) {
  bool A = (digitalRead(w.LINE_A_PIN) == HIGH);
  bool B = (digitalRead(w.LINE_B_PIN) == HIGH);

  if (A && !B) return S_IDLE;
  if (A && B)  return S_VALID;

  // If A is LOW, something is pulling it down (most likely guard/lame contact via C)
  // In this topology that should be treated as GUARD (ignored).
  return S_GUARD;
}

// raw for debounce: LOW = valid pressed, HIGH = not-valid-pressed
int computeRawForDebounce(Weapon &w, SenseState sense) {
  // Latch guardContact any time we see GUARD while there's contact
  if (sense == S_GUARD) {
    w.guardContact = true;
    // disarm until release so it can't "recover" mid-press and accidentally score
    w.armed = false;
    return HIGH; // not a valid press
  }

  // Valid press (A=HIGH,B=HIGH) => raw LOW
  if (sense == S_VALID) return LOW;

  // Idle
  return HIGH;
}

bool updateWeapon(Weapon &w, unsigned long now) {
  // Sense current state
  SenseState sense = senseABvsAC(w);
  int raw = computeRawForDebounce(w, sense);

  // Debounce edge tracking
  if (raw != w.lastRaw) {
    w.lastRaw = raw;
    w.lastDebounceTime = now;
  }

  if ((now - w.lastDebounceTime) >= DEBOUNCE_MS) {
    if (raw != w.stableState) {
      w.stableState = raw;

      if (w.stableState == LOW) {
        // Valid pressed started
        w.pressStartMs = now;
      } else {
        // Released / not valid press
        w.lastReleaseMs = now;
        w.guardContact = false;   // clear guard latch on release
        Serial.print(w.name);
        Serial.println(" released");
      }
    }
  }

  // Rearm after release
  if (!w.armed && w.stableState == HIGH && (now - w.lastReleaseMs) >= REARM_MS) {
    w.armed = true;
  }

  // LED timeout
  if (w.ledOn && now >= w.ledOffAtMs) {
    w.ledOn = false;
    digitalWrite(w.LED_PIN, LOW);
  }

  // Global lockout: ignore registrations
  if (lockOutActive) return false;

  // Register valid hit
  if (w.armed && w.stableState == LOW && !w.guardContact) {
    if ((now - w.pressStartMs) >= MIN_PRESS_MS) {
      // Confirm still valid at scoring moment
      if (senseABvsAC(w) != S_VALID) return false;

      w.armed = false;

      w.ledOn = true;
      w.ledOffAtMs = now + HIT_ON_MS;
      digitalWrite(w.LED_PIN, HIGH);

      return true;
    }
  }

  return false;
}

void setup() {
  Serial.begin(115200);
  delay(300);

  pinMode(W1.LED_PIN, OUTPUT);
  pinMode(W2.LED_PIN, OUTPUT);
  digitalWrite(W1.LED_PIN, LOW);
  digitalWrite(W2.LED_PIN, LOW);

  // A default HIGH, B default LOW
  pinMode(W1.LINE_A_PIN, INPUT_PULLUP);
  pinMode(W1.LINE_B_PIN, INPUT);

  pinMode(W2.LINE_A_PIN, INPUT_PULLUP);
  pinMode(W2.LINE_B_PIN, INPUT);
  
  pinMode(BUZZER_PIN, OUTPUT);

  Serial.println("Ready: A pull-up, B pull-down, C=GND. Guard/lame ignored.");
}

void loop() {
  unsigned long now = millis();
  buzzerUpdate(now);

  bool f1Hit = updateWeapon(W1, now);
  bool f2Hit = updateWeapon(W2, now);

  // Latch first hits + start 50ms window
  if (f1Hit && !f1Latched) {
    f1Latched = true;
    f1Time = now;
    if (!windowActive) {
      windowActive = true;
      windowStart = now;
      resultPrinted = false;
    }
  }

  if (f2Hit && !f2Latched) {
    f2Latched = true;
    f2Time = now;
    if (!windowActive) {
      windowActive = true;
      windowStart = now;
      resultPrinted = false;
    }
  }

  // Finalize after window
  if (windowActive && !resultPrinted && (now - windowStart >= EPEE_WINDOW_MS)) {
    resultPrinted = true;
    windowActive = false;

    bool f1Scores = false;
    bool f2Scores = false;

    if (f1Latched && f2Latched) {
      long dt = (long)f2Time - (long)f1Time;
      if (abs(dt) <= (long)EPEE_WINDOW_MS) {
        f1Scores = true;
        f2Scores = true;
      } else if (dt > 0) {
        f1Scores = true;
      } else {
        f2Scores = true;
      }
    } else if (f1Latched) {
      f1Scores = true;
    } else if (f2Latched) {
      f2Scores = true;
    }

    if (f1Scores && f2Scores) {
      Serial.println("DOUBLE TOUCH");
      buzzerStart(now, BUZZ_MS_DOUBLE);
    }
    if (f1Latched) {
      Serial.println(f1Scores ? "FENCER1 SCORES" : "FENCER1 NO SCORE");
      if (f1Scores) buzzerStart(now, BUZZ_MS_SINGLE);
    }
    if (f2Latched) {
      Serial.println(f2Scores ? "FENCER2 SCORES" : "FENCER2 NO SCORE");
      if (f2Scores) buzzerStart(now, BUZZ_MS_SINGLE);
    }

    // Lockout until LEDs finish
    lockOutActive = true;
    lockoutUntil = max(W1.ledOffAtMs, W2.ledOffAtMs);
  }

  // Reset after lockout AND both released
  if (lockOutActive) {
    if (now >= lockoutUntil && W1.stableState == HIGH && W2.stableState == HIGH) {
      lockOutActive = false;
      f1Latched = false;
      f2Latched = false;
      resultPrinted = false;
    }
  }

  delay(1);
}
