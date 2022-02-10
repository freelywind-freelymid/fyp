//pinout
#define I2C_SDA             21
#define I2C_SCL             22

#define debug_pin           4
#define intranet_pin        16
#define wifi_pin            17

#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include "PCA9685.h"

PCA9685 pwmController;
PCA9685_ServoEval servoSet(128,324,526);  //limit the range from +90° to -90°
float angle[5][3] = {{0, 0, 0},
                      {0, 0, 0},
                      {0, 0, 0},
                      {0, 0, 0},
                      {0, 0, 0}};

uint8_t broadcastAddress[] = {0x58, 0xBF, 0x25, 0x18, 0xC7, 0x0C};
const char* ssid = "yourNetworkName";
const char* password = "yourNetworkPassword";

String success;

typedef struct struct_msg {
  float data[8];
} struct_msg;

struct_msg package_out;
struct_msg package_in;

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");

  if (status ==0){
    success = "Delivery Success :)";

    Serial.print("Sent data: ");
    Serial.print(package_out.data[0]);
    Serial.print(", ");
    Serial.print(package_out.data[1]);
    Serial.print(", ");
    Serial.print(package_out.data[2]);
    Serial.print(", ");
    Serial.print(package_out.data[3]);
    Serial.print(", ");
    Serial.print(package_out.data[4]);
    Serial.print(", ");
    Serial.print(package_out.data[5]);
    Serial.print(", ");
    Serial.print(package_out.data[6]);
    Serial.print(", ");
    Serial.println(package_out.data[7]);
    Serial.println("");
  }
  else{
    success = "Delivery Fail :(";
  }
}

void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&package_in, incomingData, sizeof(package_in));
  Serial.print("Bytes received: ");
  Serial.println(len);
  
  Serial.print("Received data: ");
  Serial.print(package_in.data[0]);
  Serial.print(", ");
  Serial.print(package_in.data[1]);
  Serial.print(", ");
  Serial.print(package_in.data[2]);
  Serial.print(", ");
  Serial.print(package_in.data[3]);
  Serial.print(", ");
  Serial.print(package_in.data[4]);
  Serial.print(", ");
  Serial.print(package_in.data[5]);
  Serial.print(", ");
  Serial.print(package_in.data[6]);
  Serial.print(", ");
  Serial.println(package_in.data[7]);
  Serial.println("");
}

void servoSet_setup(){
  pwmController.resetDevices();
  pwmController.init();

  //1ms servo phase length
  pwmController.setPWMFrequency(1000);  // Set PWM freq to 100Hz (default is 200Hz, supports 24Hz to 1526Hz)
}

void action(){
  for(int i = 0; i < 5; i++){
    for(int r = 0; r < 3; r++){
      pwmController.setChannelPWM(i*3+r, servoSet.pwmForAngle(angle[i][r]));
    }
  }
}

void allServos_moveToMidPosition(){
  memset(angle, 0, sizeof(angle));

  action();
}

void setup() {
  Serial.begin(115200);
  Wire.begin();

  WiFi.mode(WIFI_STA);
  Serial.print("STA MAC: "); Serial.println(WiFi.macAddress());

  //init
  if(esp_now_init() != ESP_OK) {
    Serial.println("ESPNow Init Failed");
    ESP.restart();
  }

  esp_now_register_send_cb(OnDataSent);

  // Register peer
  esp_now_peer_info_t peerInfo;
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  
  // Add peer        
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }
  
  esp_now_register_recv_cb(OnDataRecv);

  servoSet_setup();
  allServos_moveToMidPosition();
}

void loop() {
  //updateStatus();
}
