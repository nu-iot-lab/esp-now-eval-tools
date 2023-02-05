#include <WebServer.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include "esp_wifi.h"

//set up to connect to an existing network (e.g. mobile hotspot from laptop that will run the python code)
const char* ssid = "esp-now";
const char* password = "esp-now-pass";
WiFiUDP Udp;
unsigned int localUdpPort = 4210;  //  port to listen on
char incomingPacket[255];  // buffer for incoming packets

unsigned int packetReceived = 0;
unsigned int prevPckt = 0;
char str[100];

void writeFile(fs::FS &fs, const char * path, const char * message){
    File file = fs.open(path, FILE_WRITE);
    
    if(!file){
        Serial.println("Failed to open file for writing");
        return;
    }

    file.print(message);
    file.close();
}

void appendFile(fs::FS &fs, const char * path, const char * message){

    File file = fs.open(path, FILE_APPEND);
    if(!file){
        Serial.println("Failed to open file for appending");
        return;
    }
    file.print(message);
    file.close();
}

void setup(){
  WiFi.mode(WIFI_STA);
  
  // esp_wifi_start();
  // ESP_ERROR_CHECK( esp_wifi_set_protocol(WIFI_IF_STA, (WIFI_PROTOCOL_11B| WIFI_PROTOCOL_11G| WIFI_PROTOCOL_11N)) );
  int a = esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_11N);
  Serial.println(a);

  int status = WL_IDLE_STATUS;
  Serial.begin(115200);
  WiFi.begin(ssid, password);

  if(!SD.begin()){
      Serial.println("Card Mount Failed");
      return;
  }

  do {
    delay(100);
    Serial.println("connecting to wifi");
  } while (WiFi.status() != WL_CONNECTED);
  
  Udp.begin(localUdpPort);
}

void loop()
{
  int packetSize = Udp.parsePacket();
  if (packetSize)
   {
    packetReceived++;
    int len = Udp.read(incomingPacket, 8);

    Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
    Udp.write('a');
    Udp.endPacket();

    union {
      byte c[4];
      unsigned int pckt;
    } pckt;

    union {
      byte t[4];
      unsigned long time;
    } time;

    pckt.c[0] = incomingPacket[0];
    pckt.c[1] = incomingPacket[1];
    pckt.c[2] = incomingPacket[2];
    pckt.c[3] = incomingPacket[3];

    time.t[0] = incomingPacket[4];
    time.t[1] = incomingPacket[5];
    time.t[2] = incomingPacket[6];
    time.t[3] = incomingPacket[7];

    if(prevPckt != pckt.pckt){

      unsigned int rssi = WiFi.RSSI();

      Serial.print(packetReceived);
      Serial.print('\t');
      Serial.print(time.time);
      Serial.print('\t');
      Serial.print(rssi);
      Serial.print('\t');
      Serial.print(pckt.pckt);
      Serial.print('\t');
      Serial.println(len);

      sprintf(str, "%d\t", packetReceived);
      appendFile(SD, "/logsResponder.txt", str);

      sprintf(str, "%lu\t", time.time);
      appendFile(SD, "/logsResponder.txt", str);

      sprintf(str, "%i\t", rssi);
      appendFile(SD, "/logsResponder.txt", str);

      sprintf(str, "%i\t", pckt.pckt);
      appendFile(SD, "/logsResponder.txt", str);

      sprintf(str, "%i\n", len);
      appendFile(SD, "/logsResponder.txt", str);
    } else {
      packetReceived--;
    }

    prevPckt = pckt.pckt;
  }


  delay(100);
}
