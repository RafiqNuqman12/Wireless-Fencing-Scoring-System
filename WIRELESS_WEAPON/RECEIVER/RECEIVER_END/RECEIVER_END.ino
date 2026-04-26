#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>

const int VALID_LED_PIN = 26;
const int OFF_LED_PIN   = 27;
const int BUZZER_PIN    = 33;

const unsigned long LED_ON_MS    = 2000;
const unsigned long BUZZER_ON_MS = 300;

typedef struct struct_message {
  int hitType;              // 0=idle, 1=off-target, 2=on-target
  unsigned long timeMs;
} struct_message;

struct_message incomingData;

bool validLedOn = false;
bool offLedOn = false;
bool buzzerOn = false;

unsigned long validLedOffAtMs = 0;
unsigned long offLedOffAtMs = 0;
unsigned long buzzerOffAtMs = 0;

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

void triggerOnTarget(unsigned long now) {
  validLedOn = true;
  validLedOffAtMs = now + LED_ON_MS;
  digitalWrite(VALID_LED_PIN, HIGH);

  offLedOn = false;
  digitalWrite(OFF_LED_PIN, LOW);

  buzzerOn = true;
  buzzerOffAtMs = now + BUZZER_ON_MS;
  tone(BUZZER_PIN, 4000);

  Serial.println("RECEIVED: ON TARGET");
}

void triggerOffTarget(unsigned long now) {
  offLedOn = true;
  offLedOffAtMs = now + LED_ON_MS;
  digitalWrite(OFF_LED_PIN, HIGH);

  validLedOn = false;
  digitalWrite(VALID_LED_PIN, LOW);

//  buzzerOn = true;
//  buzzerOffAtMs = now + BUZZER_ON_MS;
//  tone(BUZZER_PIN, 2500);

  Serial.println("RECEIVED: OFF TARGET");
}

void clearDisplay() {
  validLedOn = false;
  offLedOn = false;
  buzzerOn = false;

  digitalWrite(VALID_LED_PIN, LOW);
  digitalWrite(OFF_LED_PIN, LOW);
  noTone(BUZZER_PIN);

  Serial.println("RECEIVED: IDLE");
}

void onDataRecv(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  memcpy(&incomingData, data, sizeof(incomingData));

  unsigned long now = millis();

  Serial.print("HitType: ");
  Serial.print(incomingData.hitType);
  Serial.print(" | Sender time: ");
  Serial.println(incomingData.timeMs);

  if (incomingData.hitType == 2) {
    triggerOnTarget(now);
  } else if (incomingData.hitType == 1) {
    triggerOffTarget(now);
  } else if (incomingData.hitType == 0) {
    Serial.println("RECEIVED: IDLE");
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(VALID_LED_PIN, OUTPUT);
  pinMode(OFF_LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  digitalWrite(VALID_LED_PIN, LOW);
  digitalWrite(OFF_LED_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);

  WiFi.mode(WIFI_STA);

  Serial.print("Receiver MAC: ");
  Serial.println(WiFi.macAddress());

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    return;
  }

  esp_now_register_recv_cb(onDataRecv);

  Serial.println("Scoring box receiver ready");
}

void loop() {
  updateOutputs(millis());
  delay(1);
}
