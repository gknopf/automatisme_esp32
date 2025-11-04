#include <Arduino.h>
// programme sonnenkraft pt100

#include <ADS1X15.h>
//#include <Wire.h>
#include "painlessMesh.h"
#include <ArduinoJson.h>
#include <OneWire.h>
#include <WiFi.h>
#include <AsyncTCP.h>






 ADS1115 ADS(0x48);

#define   MESH_PREFIX     "knobuntumesh"
#define   MESH_PASSWORD   "pechvogel"
#define   MESH_PORT       5555
//initialisation materiel
float E = 5.0; //volts
int R1 = 138; // ohm
float Rpt100; // resistance sonde PT100
float Tpt100; // temperature sonde PT100

Scheduler userScheduler; // Controluer 
painlessMesh  mesh;

#define ONE_WIRE_BUS 4
OneWire oneWire(ONE_WIRE_BUS);


void sendMessage();  //prototype 

Task taskSendMessage( TASK_SECOND * 5 , TASK_FOREVER, &sendMessage ); //esecute la lecture des temperatures toutes les 5 secondes


void sendMessage() {
  JsonDocument doc;
  doc["recepteur"] ="Sonnenkraft";

  JsonArray PT100 = doc["Tp100"].to<JsonArray>();
  //JsonArray relaiPT100 = doc["relai"].to<JsonArray>();
  String jsonstringP;
  int16_t mesure_0=ADS.readADC(0);
  for(int i=0;i<4; i++){ 
    int16_t mesure_Tp100=ADS.readADC(i);
    float tension=ADS.toVoltage(mesure_Tp100);
    Rpt100 = tension*R1/(E-tension);
    Tpt100 = (Rpt100-100)/0.385;
    PT100.add(Tpt100); 
    
  } 
 
 
  serializeJson(doc,jsonstringP);
   mesh.sendBroadcast(jsonstringP);
  Serial.println(jsonstringP);
  
    
}


void receivedCallback( uint32_t from, String &msg ) {
 //activation  des relais
 JsonDocument doc;
 deserializeJson (doc,msg.c_str());
 
}

void newConnectionCallback(uint32_t nodeId) {
    //Serial.printf("--> startHere: New Connection, nodeId = %u\n", nodeId);
}

void changedConnectionCallback() {
  //Serial.printf("Changed connections\n");
}

void nodeTimeAdjustedCallback(int32_t offset) {
    ////Serial.printf("Adjusted time %u. Offset = %d\n", mesh.getNodeTime(),offset);
}




void setup() {

 Serial.begin(115200);
 
 
 //Serial.println("pont diviseur");
 Wire.begin();
 ADS.begin();
ADS.setGain(1);

 
 mesh.setDebugMsgTypes( ERROR | STARTUP );  // set before init() so that you can see startup messages

 mesh.init( MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT , WIFI_AP_STA, 6);
//mesh.init( MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT );
 mesh.onReceive(&receivedCallback);
 mesh.onNewConnection(&newConnectionCallback);
 mesh.onChangedConnections(&changedConnectionCallback);
mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);
userScheduler.addTask(taskSendMessage);
taskSendMessage.enable();
 

}


void loop() {

userScheduler.execute();
mesh.update();

}