#include <esp_now.h>
#include <WiFi.h>
#include <vector>

// Network Config
IPAddress local_IP(192, 168, 1, 100);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);

const int DISCOVERY_WINDOW = 10000; // 10 seconds
unsigned long startTime;
bool isDiscoveryMode = true;

struct DeviceEntry {
  uint8_t mac[6];
  String id;
};
std::vector<DeviceEntry> knownSlaves;

// Callback when data is received
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  if (isDiscoveryMode) {
    String receivedID = String((char*)incomingData).substring(0, len);
    
    // Check for ID Clashes
    for (const auto& dev : knownSlaves) {
      if (dev.id == receivedID) {
        Serial.printf("!!! ALERT: ID Clash detected for [%s] !!!\n", receivedID.c_str());
        return;
      }
    }

    // Register Slave
    DeviceEntry newSlave;
    memcpy(newSlave.mac, mac, 6);
    newSlave.id = receivedID;
    knownSlaves.push_back(newSlave);
    
    // Add as Peer
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, mac, 6);
    esp_now_add_peer(&peerInfo);
    
    Serial.printf("New Device Verified: %s\n", receivedID.c_str());
  } else {
    // Printing analog data continuously
    int sensorValue;
    memcpy(&sensorValue, incomingData, sizeof(sensorValue));
    Serial.printf("ID: ? | Analog Value: %d\n", sensorValue);
  }
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);

  // Set hardcoded IP
  if (!WiFi.config(local_IP, gateway, subnet)) {
    Serial.println("Static IP Configuration Failed");
  }

  if (esp_now_init() != ESP_OK) return;
  esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataRecv));

  // Add Broadcast Peer to send "START" command later
  uint8_t broadcast[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  esp_now_peer_info_t bcastPeer = {};
  memcpy(bcastPeer.peer_addr, broadcast, 6);
  esp_now_add_peer(&bcastPeer);

  startTime = millis();
  Serial.println(">>> DISCOVERY MODE: Waiting for Transmitters...");
}

void loop() {
  if (isDiscoveryMode && (millis() - startTime > DISCOVERY_WINDOW)) {
    isDiscoveryMode = false;
    Serial.println(">>> DISCOVERY ENDED. Sending START command...");
    
    const char* startCmd = "START_SENDING";
    uint8_t broadcast[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    esp_now_send(broadcast, (uint8_t *)startCmd, strlen(startCmd));
  }
}