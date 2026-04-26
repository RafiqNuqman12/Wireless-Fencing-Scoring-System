#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
// THIS CODE IS FOR FOIL WEAPON

// ========= PINS =========
const int A_PIN = 27;              // Line A
const int B_PIN = 18;              // Line B

// ========= TIMING =========
const unsigned long DEBOUNCE_MS  = 5;
const unsigned long MIN_HIT_MS   = 15;
const unsigned long REARM_MS     = 50;

// ========= ESPNOW =========
uint8_t receiverAddress[] = {0x1C, 0xC3, 0xAB, 0xC2, 0x1D, 0x74};

typedef struct struct_message {
  int hitType;              // 0=idle, 1=off-target, 2=on-target
  unsigned long timeMs;     // timestamp
} struct_message;

struct_message msg;

// ========= STATE =========
bool armed = true;
bool hitLatched = false;

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

// ========= SEND FUNCTION =========
void sendHit(int hitType, unsigned long now) {
  msg.hitType = hitType;
  msg.timeMs = now;

  esp_err_t result = esp_now_send(receiverAddress, (uint8_t *)&msg, sizeof(msg));

  Serial.print("Sent hitType = ");
  Serial.print(hitType);
  Serial.print(" | result = ");
  Serial.println(result == ESP_OK ? "OK" : "FAIL");
}

void setup() {
  Serial.begin(115200);

  pinMode(A_PIN, INPUT);
  pinMode(B_PIN, INPUT_PULLUP);

  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    return;
  }

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, receiverAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }

  Serial.println("Foil wireless sender ready");
}

void loop() {
  unsigned long now = millis();

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
      if (validStartMs == 0) validStartMs = now;

      if ((now - validStartMs) >= MIN_HIT_MS) {
        armed = false;
        hitLatched = true;
        sendHit(2, now);
        Serial.println("ON TARGET");
      }
    } else {
      validStartMs = 0;
    }

    if (offTargetState) {
      if (offStartMs == 0) offStartMs = now;

      if ((now - offStartMs) >= MIN_HIT_MS) {
        armed = false;
        hitLatched = true;
        sendHit(1, now);
        Serial.println("OFF TARGET");
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
      if (releaseTimeMs == 0) releaseTimeMs = now;

      if ((now - releaseTimeMs) >= REARM_MS) {
        armed = true;
        hitLatched = false;
        releaseTimeMs = 0;
        sendHit(0, now);   // tell scoring box we're back to idle
        Serial.println("Rearmed");
      }
    } else {
      releaseTimeMs = 0;
    }
  }

  delay(1);
}
