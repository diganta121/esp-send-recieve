#include <esp_now.h>
#include <WiFi.h>

const char* MY_UNIQUE_ID = "TX_UNIT_01"; // Change this for each transmitter
const int ANALOG_PIN = 0; 
bool active = false;

void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  String cmd = String((char*)incomingData).substring(0, len);
  if (cmd == "START_SENDING") {
    active = true;
    Serial.println("Received START command from Master.");
  }
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) return;
  esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataRecv));

  // Register broadcast to send ID and receive commands
  uint8_t broadcast[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, broadcast, 6);
  esp_now_add_peer(&peerInfo);

  // Announce presence for discovery
  esp_now_send(broadcast, (uint8_t *)MY_UNIQUE_ID, strlen(MY_UNIQUE_ID));
}

void loop() {
  if (active) {
    int sensorVal = analogRead(ANALOG_PIN);
    uint8_t broadcast[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    esp_now_send(broadcast, (uint8_t *)&sensorVal, sizeof(sensorVal));
    delay(100); // Fast continuous transmission
  }
}