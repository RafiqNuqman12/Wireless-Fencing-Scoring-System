#include <WiFi.h>
#include <esp_now.h>

const int LED_PIN = 26;
const int BUZZER_PIN = 33;

const unsigned long HIT_ON_MS = 2000;
const unsigned long BUZZ_MS   = 1000;

bool ledOn = false;
bool buzOn = false;
unsigned long ledOffAt = 0;
unsigned long buzOffAt = 0;

struct HitMessage {
  char msg[16];
};

void triggerHit(unsigned long now) {
  digitalWrite(LED_PIN, HIGH);
  tone(BUZZER_PIN, 4000);

  ledOn = true;
  buzOn = true;
  ledOffAt = now + HIT_ON_MS;
  buzOffAt = now + BUZZ_MS;

  Serial.println("HIT RECEIVED -> LED ON, BUZZER ON");
}

void onDataRecv(const esp_now_recv_info_t *info, const uint8_t *incomingData, int len) {
  HitMessage data;
  memcpy(&data, incomingData, sizeof(data));

  if (strcmp(data.msg, "HIT") == 0) {
    triggerHit(millis());
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  digitalWrite(LED_PIN, LOW);
  noTone(BUZZER_PIN);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    return;
  }

  esp_now_register_recv_cb(onDataRecv);

  Serial.println("Scoring box receiver ready");
  Serial.print("Receiver MAC: ");
  Serial.println(WiFi.macAddress());
}

void loop() {
  unsigned long now = millis();

  if (ledOn && now >= ledOffAt) {
    ledOn = false;
    digitalWrite(LED_PIN, LOW);
  }

  if (buzOn && now >= buzOffAt) {
    buzOn = false;
    noTone(BUZZER_PIN);
  }

  delay(1);
}
