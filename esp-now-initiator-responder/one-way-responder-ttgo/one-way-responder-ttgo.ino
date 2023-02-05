// BY NU IOT LAB //
// REFERENCES:
// rssi: https://github.com/TenoTrash/ESP32_ESPNOW_RSSI/blob/main/Modulo_Receptor_OLED_SPI_RSSI.ino

// Include Libraries
#include <esp_wifi.h>
#include <esp_now.h>
#include <WiFi.h>
#include <FS.h>
#include <SD.h>
#include <SPI.h>

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define BUTTON_PIN 33
#define VIBRO_PIN 12

#define BUZZER_PIN 25
#define FREQ 5000
#define CB_CHANNEL 1
#define RES 8


//////////////////////////////// OLED ////////////////////////////////////////////////////

#define OLED_SDA 4
#define OLED_SCL 15
#define OLED_RST 16
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RST);

void prepareDisplay(){

  //reset OLED display via software
  pinMode(OLED_RST, OUTPUT);
  digitalWrite(OLED_RST, LOW);
  delay(20);
  digitalWrite(OLED_RST, HIGH);

  //initialize OLED
  Wire.begin(OLED_SDA, OLED_SCL);
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3c, false, false)) { // Address 0x3C for 128x32
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(0,0);
  display.print("it Works!");
  display.display();

  return;
}


///////////////////////// MAC STUFF, ENCRYPTION AND DATA /////////////////////////////


// Set the MASTER MAC Address
uint8_t masterAddress[] = {0x0C,0xB8,0x15,0xD7,0x8F,0x2C}; // e0
int packetRecieved = 0;

// PMK and LMK keys
static const char* PMK_KEY_STR = "PLEASE_CHANGE_ME";
static const char* LMK_KEY_STR = "DONT_BE_LAZY_OK?";

esp_now_peer_info_t masterInfo;

// Define a data structure with fixed size
typedef struct struct_message {
  unsigned long time;
  unsigned int packetNumber;
} struct_message;
int dataSize = 8;

// Create a structured object
struct_message myData;

/////////////////////////////////////   RSSI  //////////////////////////////////////

int rssi_display;

// Estructuras para calcular los paquetes, el RSSI, etc
typedef struct {
  unsigned frame_ctrl: 16;
  unsigned duration_id: 16;
  uint8_t addr1[6]; /* receiver address */
  uint8_t addr2[6]; /* sender address */
  uint8_t addr3[6]; /* filtering address */
  unsigned sequence_ctrl: 16;
  uint8_t addr4[6]; /* optional */
} wifi_ieee80211_mac_hdr_t;

typedef struct {
  wifi_ieee80211_mac_hdr_t hdr;
  uint8_t payload[0]; /* network data ended with 4 bytes csum (CRC32) */
} wifi_ieee80211_packet_t;

//La callback que hace la magia
void promiscuous_rx_cb(void *buf, wifi_promiscuous_pkt_type_t type) {
  // All espnow traffic uses action frames which are a subtype of the mgmnt frames so filter out everything else.
  if (type != WIFI_PKT_MGMT)
    return;

  const wifi_promiscuous_pkt_t *ppkt = (wifi_promiscuous_pkt_t *)buf;
  const wifi_ieee80211_packet_t *ipkt = (wifi_ieee80211_packet_t *)ppkt->payload;
  const wifi_ieee80211_mac_hdr_t *hdr = &ipkt->hdr;

  int rssi = ppkt->rx_ctrl.rssi;
  rssi_display = rssi;
}

//////////////////////////////////// END RSSI /////////////////////////////////

const char * path = "/responder_logs.txt";
char str[100];

void appendFile(fs::FS &fs, const char * path, const char * message){
  Serial.printf("Appending to file: %s\n", path);

  File file = fs.open(path, FILE_APPEND);
  if(!file){
    Serial.println("Failed to open file for appending");
    return;
  }
  if(file.print(message)){
    // Serial.println("Message appended");
  } else {
    Serial.println("Append failed");
  }
  file.close();
}

// Callback function executed when data is received
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {

  packetRecieved++;
  //display.clearDisplay();
  
  memcpy(&myData, incomingData, sizeof(myData));
  
  Serial.print(packetRecieved);
  Serial.print('\t');
  Serial.print(myData.time);
  Serial.print('\t');
  Serial.print(rssi_display);
  Serial.print('\t');
  Serial.print(myData.packetNumber);
  Serial.print('\t');
  Serial.println(len);

  sprintf(str, "%d\t", packetReceived);
  appendFile(SD, path, str);

  sprintf(str, "%lu\t", myData.time);
  appendFile(SD, path, str);

  sprintf(str, "%i\t", rssi);
  appendFile(SD, path, str);

  sprintf(str, "%i\t", myData.packetNumber);
  appendFile(SD, path, str);

  sprintf(str, "%i\n", len);
  appendFile(SD, path, str);

  //display.setCursor(0,0);
  //display.print("numpack:" + String(packetRecieved));
  
  //display.setCursor(0,10);

  //display.print("rssi: " + String(rssi_display));
  digitalWrite(VIBRO_PIN, HIGH);
  ledcWriteTone(CB_CHANNEL, 2000);
  
  delay(500);
  digitalWrite(VIBRO_PIN, LOW);
  ledcWriteTone(CB_CHANNEL, 0);

  //display.display();
}


void setup() {
  // Set up Serial Monitor
  Serial.begin(115200);

  //Init sd card
  if(!SD.begin()){
    Serial.println("Card Mount Failed");
    return;
  }

  ///////////////////////////////////////////////////////
  // set up the BUZZER settings
  ledcSetup(CB_CHANNEL, FREQ, RES);
  ledcAttachPin(BUZZER_PIN, CB_CHANNEL);


  //prepareDisplay();

  //set up VIBRO and button
  pinMode(VIBRO_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT);
  /////////////////////////////////////////////////////


  // Set ESP32 as a Wi-Fi Station
  WiFi.mode(WIFI_STA);
  
  int a = esp_wifi_set_protocol( WIFI_IF_AP, WIFI_PROTOCOL_LR);
  esp_wifi_config_espnow_rate(WIFI_IF_AP, WIFI_PHY_RATE_LORA_250K);
  
  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("There was an error initializing ESP-NOW");
    return;
  }

  // Setting the PMK key
  esp_now_set_pmk((uint8_t *)PMK_KEY_STR);

  // Register the master
  memcpy(masterInfo.peer_addr, masterAddress, 6);
  masterInfo.channel = 0;
  // Setting the master device LMK key
  for (uint8_t i = 0; i < 16; i++) {
    masterInfo.lmk[i] = LMK_KEY_STR[i];
  }
  masterInfo.encrypt = true;
  
  // Add master        
  if (esp_now_add_peer(&masterInfo) != ESP_OK){
    Serial.println("There was an error registering the master");
    return;
  }
  
  // Register callback function
  esp_now_register_recv_cb(OnDataRecv);

  esp_wifi_set_promiscuous(true);
  esp_wifi_set_promiscuous_rx_cb(&promiscuous_rx_cb);

}


void loop() {
  // read button state
  int buttonState = digitalRead(BUTTON_PIN);

  Serial.println("WAAAAALLLL-EEEEEEEEEEEEEE");

  // if pressed, then beep and ALARM on screen
  while(buttonState){

    // beep and text
    digitalWrite(VIBRO_PIN, HIGH);
    ledcWriteTone(CB_CHANNEL, 2000);
    //lcd.print("ALARM");    
    
    delay(500);
    
    digitalWrite(VIBRO_PIN, LOW);
    ledcWriteTone(CB_CHANNEL, 0);
    //lcd.clear();
    
    delay(200);

    // check if continue
    buttonState = digitalRead(BUTTON_PIN);
  }

  delay(500);
  // safeguard clear
}
