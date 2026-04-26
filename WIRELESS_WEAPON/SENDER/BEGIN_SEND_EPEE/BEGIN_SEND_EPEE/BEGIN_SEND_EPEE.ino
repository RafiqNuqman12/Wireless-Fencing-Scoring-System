#include <WiFi.h>
#include <esp_now.h>

const int LINE_A_PIN = 27;
const int LINE_B_PIN = 18;

// Replace with your receiver MAC
uint8_t receiverMac[] = {0x1C, 0xC3, 0xAB, 0xC2, 0x1D, 0x74};

struct HitMessage {
  char msg[16];
};

HitMessage data;

void onDataSent(const esp_now_send_info_t *info, esp_now_send_status_t status) {
  Serial.print("Send status: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");
}

void setup() {
  Serial.begin(115200);

  pinMode(LINE_A_PIN, INPUT_PULLUP);
  pinMode(LINE_B_PIN, INPUT);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    return;
  }

  esp_now_register_send_cb(onDataSent);

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, receiverMac, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }

  Serial.println("Weapon transmitter ready");
}

void loop() {
  bool A = digitalRead(LINE_A_PIN);
  bool B = digitalRead(LINE_B_PIN);

  // Valid epee hit condition: A HIGH and B HIGH
  if (A == HIGH && B == HIGH) {
    strcpy(data.msg, "HIT");
    esp_now_send(receiverMac, (uint8_t *)&data, sizeof(data));
    Serial.println("VALID HIT SENT");
    delay(3000); // simple lockout
  }

  delay(5);
}
