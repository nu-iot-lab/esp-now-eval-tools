#include <WiFi.h>
#include "esp_wifi.h"
#include <WiFiUdp.h>
#include <FS.h>
#include <SD.h>
#include <SPI.h>

const char* ssid     = "esp-now";
const char* password = "esp-now-pass";
const int   channel  = 1;

WiFiUDP Udp;
const unsigned int localUdpPort = 4210;

unsigned int pck_num = 1;

const char *path = "/intiator_logs.txt";
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

void setup() {
  Serial.begin(115200);
  if(!SD.begin()){
    Serial.println("Card Mount Failed");
    return;
  }
  WiFi.mode(WIFI_AP);
  esp_wifi_start();
  // ESP_ERROR_CHECK( esp_wifi_set_protocol(WIFI_IF_AP, (WIFI_PROTOCOL_11B| WIFI_PROTOCOL_11G| WIFI_PROTOCOL_11N)) );
  int a = esp_wifi_set_protocol(WIFI_IF_AP, WIFI_PROTOCOL_11N);
  Serial.println(a);
  WiFi.softAP(ssid, password, channel, 0, 5);
  sprintf(str, "[+] AP Created with IP Gateway \n");
  Serial.printf(str);
  appendFile(SD, path, str);
  Serial.println(WiFi.softAPIP());
  Udp.begin(localUdpPort);
  // delay(1000*60*5);
  for (int a = 0; a < 1; a++) {
    wifi_sta_list_t wifi_sta_list;
    tcpip_adapter_sta_list_t adapter_sta_list;
    esp_wifi_ap_get_sta_list(&wifi_sta_list);
    tcpip_adapter_get_sta_list(&wifi_sta_list, &adapter_sta_list);
    for (int i = 0; i < adapter_sta_list.num; i++) {
      IPAddress ip = IPAddress(adapter_sta_list.sta[i].ip.addr);
      Serial.println(ip);
    }
    delay(1000*10);
  }
  sprintf(str, "Begin transmission\n");
  Serial.printf(str);
  appendFile(SD, path, str);
}

int send_int(unsigned int n, IPAddress ip) {
  union {
    byte b[4];
    unsigned int v;
  } un_int;
  union {
    byte b[4];
    unsigned long v;
  } un_lon;
  sprintf(str, "packet #: %d\n", n); 
  Serial.printf(str);
  appendFile(SD, path, str);
  un_int.v = n;
  un_lon.v = millis();
  sprintf(str, "time: %lu\n", un_lon.v);
  Serial.printf(str);
  appendFile(SD, path, str);
  Udp.beginPacket(ip, localUdpPort);
  for (int i = 0; i < 4; i++) {
    Udp.write(un_int.b[i]);
  }
  for (int i = 0; i < 4; i++) {
    Udp.write(un_lon.b[i]);
  }
  Udp.endPacket();
  sprintf(str, "Sent a upd packet\n");
  Serial.printf(str);
  appendFile(SD, path, str);
  char ack[1];
  int pack_size = Udp.parsePacket();
  if (pack_size) {
    Udp.read(ack, 1);
    Serial.println(pack_size);
    Serial.println(ack[0]);
  }
  return pack_size;
}

void loop() {
  // send 1000 packets
  while (pck_num <= 1000) {
    sprintf(str, "%d\n", pck_num);
    Serial.printf(str);
    appendFile(SD, path, str);
    wifi_sta_list_t wifi_sta_list;
    tcpip_adapter_sta_list_t adapter_sta_list;
    esp_wifi_ap_get_sta_list(&wifi_sta_list);
    tcpip_adapter_get_sta_list(&wifi_sta_list, &adapter_sta_list);
    for (int i = 0; i < adapter_sta_list.num; i++) {
      IPAddress ip = IPAddress(adapter_sta_list.sta[i].ip.addr);
      Serial.println(ip);
      // char* c = &(ip.toString()[0]);
      // sprintf(str, "%zu", ip);
      // Serial.printf(str);
      // appendFile(SD, path, str);
      for (int i = 0; i < 5 && !send_int(pck_num, ip); i++) {
        delay(10);
      }
      delay(5000);
    }
    Serial.println();
    appendFile(SD, path, "\n");
    pck_num++;
    // delay(100);
  }
}