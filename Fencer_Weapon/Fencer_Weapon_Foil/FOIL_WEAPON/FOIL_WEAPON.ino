// THIS IS FOR FOIL WEAPON CODING
#include <Arduino.h>

// ========= PINS =========
const int A_PIN = 27;              // Line A
const int B_PIN = 18;              // Line B

const int VALID_LED_PIN = 26;      // On-target LED
const int OFF_LED_PIN   = 33;      // Off-target LED
const int BUZZER_PIN    = 32;      // Buzzer

// ========= TIMING =========
const unsigned long DEBOUNCE_MS  = 5;
const unsigned long MIN_HIT_MS   = 15;    // minimum contact time
const unsigned long LED_ON_MS    = 2000;  // LED hold time
const unsigned long BUZZER_ON_MS = 300;   // buzzer duration
const unsigned long REARM_MS     = 50;    // rearm delay once back to idle

// ========= STATE =========
bool armed = true;
bool hitLatched = false;

bool validLedOn = false;
bool offLedOn = false;
bool buzzerOn = false;

unsigned long validLedOffAtMs = 0;
unsigned long offLedOffAtMs = 0;
unsigned long buzzerOffAtMs = 0;

unsigned long validStartMs = 0;
unsigned long offStartMs = 0;
unsigned long releaseTimeMs = 0;

// Debounce states
int lastARaw = LOW;
int aStable = LOW;
unsigned long aLastChangeMs = 0;

int lastBRaw = LOW;
int bStable = LOW;
unsigned long bLastChangeMs = 0;

// ========= FUNCTIONS =========
void startValidHit(unsigned long now) {
  armed = false;
  hitLatched = true;

  validLedOn = true;
  validLedOffAtMs = now + LED_ON_MS;
  digitalWrite(VALID_LED_PIN, HIGH);

  offLedOn = false;
  digitalWrite(OFF_LED_PIN, LOW);

  buzzerOn = true;
  buzzerOffAtMs = now + BUZZER_ON_MS;
  tone(BUZZER_PIN, 4000);

  Serial.println("ON TARGET");
}

void startOffTargetHit(unsigned long now) {
  armed = false;
  hitLatched = true;

  offLedOn = true;
  offLedOffAtMs = now + LED_ON_MS;
  digitalWrite(OFF_LED_PIN, HIGH);

  validLedOn = false;
  digitalWrite(VALID_LED_PIN, LOW);

  buzzerOn = true;
  buzzerOffAtMs = now + BUZZER_ON_MS;
  tone(BUZZER_PIN, 2500);

  Serial.println("OFF TARGET");
}

void updateOutputs(unsigned long now) {
  if (validLedOn && now >= validLedOffAtMs) {
    validLedOn = false;
    digitalWrite(VALID_LED_PIN, LOW);
  }

  if (offLedOn && now >= offLedOffAtMs) {
    offLedOn = false;
    digitalWrite(OFF_LED_PIN, LOW);
  }

  if (buzzerOn && now >= buzzerOffAtMs) {
    buzzerOn = false;
    noTone(BUZZER_PIN);
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(A_PIN, INPUT);          
  pinMode(B_PIN, INPUT_PULLUP);   // so that B stays high when pressed 

  pinMode(VALID_LED_PIN, OUTPUT);
  pinMode(OFF_LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  digitalWrite(VALID_LED_PIN, LOW);
  digitalWrite(OFF_LED_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);

  Serial.println("Foil scoring ready");
}

void loop() {
  unsigned long now = millis();

  updateOutputs(now);

  // ===== RAW READS =====
  int aRaw = digitalRead(A_PIN);
  int bRaw = digitalRead(B_PIN);

  // ===== DEBOUNCE A =====
  if (aRaw != lastARaw) {
    lastARaw = aRaw;
    aLastChangeMs = now;
  }
  if ((now - aLastChangeMs) >= DEBOUNCE_MS) {
    aStable = aRaw;
  }

  // ===== DEBOUNCE B =====
  if (bRaw != lastBRaw) {
    lastBRaw = bRaw;
    bLastChangeMs = now;
  }
  if ((now - bLastChangeMs) >= DEBOUNCE_MS) {
    bStable = bRaw;
  }

  // ===== STATES =====
  bool idleState      = (aStable == LOW  && bStable == LOW);
  bool offTargetState = (aStable == LOW  && bStable == HIGH);
  bool onTargetState  = (aStable == HIGH && bStable == HIGH);

  // ===== HIT LOGIC =====
  if (armed && !hitLatched) {
    if (onTargetState) {
      if (validStartMs == 0) {
        validStartMs = now;
      }

      if ((now - validStartMs) >= MIN_HIT_MS) {
        startValidHit(now);
      }
    } else {
      validStartMs = 0;
    }

    if (offTargetState) {
      if (offStartMs == 0) {
        offStartMs = now;
      }

      if ((now - offStartMs) >= MIN_HIT_MS) {
        startOffTargetHit(now);
      }
    } else {
      offStartMs = 0;
    }
  } else {
    validStartMs = 0;
    offStartMs = 0;
  }

  // ===== REARM =====
  if (!armed) {
    if (idleState) {
      if (releaseTimeMs == 0) {
        releaseTimeMs = now;
      }

      if ((now - releaseTimeMs) >= REARM_MS) {
        armed = true;
        hitLatched = false;
        releaseTimeMs = 0;
        Serial.println("Rearmed");
      }
    } else {
      releaseTimeMs = 0;
    }
  }

  delay(1);
}
