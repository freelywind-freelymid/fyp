//pinout
#define I2C_SDA              21
#define I2C_SCL              22

#define I2C_GND_address      0x48  //GND
#define I2C_VDD_address      0x49  //VDD
#define I2C_SDA_address      0x4A  //SDA
#define I2C_SCL_address      0x4B  //SCL

#define I2C_speed            3400000

// 0  0x0000  2/3x gain +/- 6.144V  1 bit = 3mV
// 1  0x0200  1x gain   +/- 4.096V  1 bit = 2mV
// 2  0x0400  2x gain   +/- 2.048V  1 bit = 1mV
// 4  0x0600  4x gain   +/- 1.024V  1 bit = 0.5mV
// 8  0x0800  8x gain   +/- 0.512V  1 bit = 0.25mV
// 16 0x0A00  16x gain  +/- 0.256V  1 bit = 0.125mV
#define adc_gain             2
// 0  Continus mode
// 1  Single mode
#define adc_mode             0
#define number_channel       3

#include <Arduino.h>

#include <math.h>
#include <arduinoFFT.h>

#include <esp_now.h>
#include <WiFi.h>
#include <WebServer.h>

#include <ADS1X15.h>

arduinoFFT FFT = arduinoFFT();

//  0  128sps
//  1  250sps
//  2  490sps
//  3  920sps
//  4  1600sps
//  5  2400sps
//  6  3300sps
//  7  3300sps
#define sampling_rate        7
const uint16_t samples = 64;
const double sampling = 3300;
const int fft_freq = 10;
bool ready_for_sampling = true;
bool ready_for_fft = false;
bool isFinished_fft = false;
bool enable_read_data = true; 

TaskHandle_t read_adcs_task;
TaskHandle_t fft_all_channel_task;
TaskHandle_t esp_now_send_task;

double vReals[number_channel][samples];
double vImags[number_channel][samples];

ADS1015 adc0(I2C_GND_address);

//bool adc0_enable_flag = false;

uint8_t broadcastAddress[] = {0x58, 0xBF, 0x25, 0x18, 0xB4, 0x84};
const char* ssid = "yourNetworkName";
const char* password = "yourNetworkPassword";

typedef struct struct_msg_in {
  float data[8];
} struct_msg_in;

typedef struct struct_msg_out {
  float data[number_channel][samples/4];
} struct_msg_out;

struct_msg_in package_in;
struct_msg_out package_out;

void updateMotorsPWM(){

}

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&package_in, incomingData, sizeof(package_in));
  /*Serial.print("Bytes received: ");
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
  Serial.println("");*/

  //feedback part
  updateMotorsPWM();
}

void esp32_now_send(){
  /*Serial.println("Sent data: ");
  for(int r = 0; r < number_channel; r++){
    Serial.print("channel ");
    Serial.print(r);
    Serial.println(":");

    for(int i = 0; i < samples/4; i++){
      Serial.print(package_out.data[r][i]);
      Serial.print(", ");
    }
    Serial.println("");
  }*/

  // Send message via ESP-NOW
  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &package_out, sizeof(package_out));
}

void adcs_init(){
  adc0.begin();
  if(adc0.isConnected()){
    adc0.setWireClock(I2C_speed);
    adc0.setGain(adc_gain);
    adc0.setDataRate(sampling_rate);
    adc0.setMode(adc_mode);

    //adc0_enable_flag = true;
    Serial.println("adc0 init. OK");
  }
  else{
    Serial.println("adc0 init. Fail");
  }
}

void read_adcs(void * pvParameters){
  while(1){
    int counter = sampling/fft_freq/number_channel;
    if(counter > samples){
      counter = samples;
    }

    //reset
    for(int i=0;i<number_channel;i++){
      for(int r=0;r<samples;r++){
          vReals[i][r]=0;
      }
    }
    for(int i=0;i<number_channel;i++){
      for(int r=0;r<samples;r++){
          vImags[i][r]=0;
      }
    }
  
    unsigned long previousMicros = 0;   //us
    float interval = 1000000/fft_freq/samples;
    unsigned long start_time = micros();

    for(int i=0; i<counter; i++){
      previousMicros = micros();

      for(int r=0; r<number_channel; r++){
        adc0.requestADC(r);

        double temp = adc0.getValue();
        if(temp < 0){
          temp *= -1;
        }       
        vReals[r][i] = temp;
        vImags[r][i] = 0;
      }
      
      //waiting next sampling
      while(micros() - previousMicros < interval){
        delayMicroseconds(1);
      }
    }
   
    //start to run fft part
    ready_for_fft = true;

    //waiting next sampling
    while(micros() - start_time < samples*interval){
      delayMicroseconds(10);
    }
  }
}

void fft_all_channel(void * pvParameters){
  while(1){
    if(ready_for_fft){
      ready_for_fft = false;

      for(int i = 0; i < number_channel; i++){
        FFT.Windowing(vReals[i], samples, FFT_WIN_TYP_HAMMING, FFT_FORWARD);  //Weigh data
        FFT.Compute(vReals[i], vImags[i], samples, FFT_FORWARD);  //compute FFT
        FFT.ComplexToMagnitude(vReals[i], vImags[i], samples);

        for(int r = 0; r < samples/2; r+=2){      
          package_out.data[i][r/2] = (float)vReals[i][r];
        }
      }

      isFinished_fft = true;

      /*View all these three lines in serial terminal to see which frequencies has which amplitudes*/
      /*for(int r = 0; r<number_channel; r++){
        for(int i=0; i<(samples/2); i+=2)
        {        
          Serial.print((i * 1.0 * sampling/number_channel) / samples, 1);
          Serial.print("Hz ");
          Serial.println(vReals[r][i], 1);    //View only this line in serial plotter to visualize the bins
        }
      }*/
    }
    else{
      delay(1);
    }
  }
}

void esp_now_send(void * pvParameters){
  while(1){
    if(isFinished_fft){
      isFinished_fft = false;
    
      esp32_now_send();  
    }
    else{
      delay(10);
    } 
  }
}

void setup() {
  Serial.begin(115200);

  adcs_init();

  WiFi.mode(WIFI_STA);
  Serial.print("STA MAC: "); Serial.println(WiFi.macAddress());

  //esp_now init
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

  xTaskCreatePinnedToCore(
             read_adcs, /* Task function. */
             "read_adcs",   /* name of task. */
             10000,     /* Stack size of task */
             NULL,      /* parameter of the task */
             1,         /* priority of the task */
             &read_adcs_task,    /* Task handle to keep track of created task */
             0);        /* pin task to core 0 */

  xTaskCreatePinnedToCore(
             fft_all_channel, /* Task function. */
             "fft_all_channel",   /* name of task. */
             10000,     /* Stack size of task */
             NULL,      /* parameter of the task */
             1,         /* priority of the task */
             &fft_all_channel_task,    /* Task handle to keep track of created task */
             1);        /* pin task to core 1 */
             
  xTaskCreatePinnedToCore(
             esp_now_send, /* Task function. */
             "esp_now_send",   /* name of task. */
             10000,     /* Stack size of task */
             NULL,      /* parameter of the task */
             1,         /* priority of the task */
             &esp_now_send_task,    /* Task handle to keep track of created task */
             0);        /* pin task to core 0 */
}

void loop() {

}
