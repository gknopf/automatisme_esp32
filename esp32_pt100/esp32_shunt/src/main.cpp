#include <Arduino.h>
// programme bouilleur pt100

#include <ADS1X15.h>
//#include <Wire.h>
#include "painlessMesh.h"
#include <ArduinoJson.h>
#include <OneWire.h>
#include <WiFi.h>
#include <WiFiClient.h>

//#include <ElegantOTA.h>

//const char* ssid = "knobuntufree";
//const char* password = "Pech_Vogel_free123";

String localip;
 ADS1115 ADS(0x48);

#define   MESH_PREFIX     "knobuntumesh"
#define   MESH_PASSWORD   "pechvogel"
#define   MESH_PORT       5555






Scheduler userScheduler; // Controluer 
painlessMesh  mesh;







void sendMessage();  //prototype 

Task taskSendMessage( TASK_SECOND * 5 , TASK_FOREVER, &sendMessage ); //esecute la lecture des temperatures toutes les 5 secondes

float tension_batterie(){
//vs= ve(r4/(r4+v3)
//ve=vs(1+r3/r4)
//avec r4=100, r3 2000 ohm  vs voltage  A0;
 int16_t mesure_A0=ADS.readADC(0);
 float vs=ADS.toVoltage(0);
 float ve=vs*(1+2000/100);
 return ve;

}
float intensite_batterie(){
//ddp (vsh) 75mv pour 100A
//vsh=2.5+ ddp(vsh)
//d'ou ibat=100*10³ /75*(vsh-2.5)
 int16_t mesure_A1=ADS.readADC(1);
 float vsh=ADS.toVoltage(1);
 float ibat=100*1000/75*(vsh-2.5);
 return ibat;

}
void sendMessage() {
  JsonDocument doc;
  doc["recepteur"] ="shunt";

  
  // JsonArray relaiPT100 = doc["relai"].to<JsonArray>();
  String jsonstringP;
  int16_t mesure_0=ADS.readADC(0);
  float ve =tension_batterie();
  doc["tension_batterie"]=ve;
  doc["intensite_batterie"]=intensite_batterie();
  int16_t mesure_A2=ADS.readADC(2);
   float refvoltage =ADS.toVoltage(2);
  Serial.print(refvoltage);
  Serial.print( ": ");
   Serial.println(mesure_A2);;
 
  doc["refvoltage"]=refvoltage;
  serializeJson(doc,jsonstringP);
  mesh.sendBroadcast(jsonstringP);
  Serial.println(jsonstringP);

}


void receivedCallback( uint32_t from, String &msg ) {
 //activation  des relais
 JsonDocument doc;
 deserializeJson (doc,msg.c_str());
 /*
  for (int i=0; i<2;i++){
    (doc["relai"][i]=="ON")?etat[i]=1:etat[i]=0;
    digitalWrite(GPIOrelai[i],etat[i]);
  }

  */
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
 Serial.println("pont diviseur");
 Wire.begin();
 ADS.begin();
 ADS.setGain(1);

 //  digitalWrite(13,HIGH);
//digitalWrite(13,HIGH);
 mesh.setDebugMsgTypes( ERROR | STARTUP );  // set before init() so that you can see startup messages

 mesh.init( MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT , WIFI_AP_STA, 6);
//mesh.init( MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT );
 mesh.onReceive(&receivedCallback);
 mesh.onNewConnection(&newConnectionCallback);
 mesh.onChangedConnections(&changedConnectionCallback);
mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);
userScheduler.addTask(taskSendMessage);
taskSendMessage.enable();
 
/*
for(int i=0; i<2;i++){
  pinMode(GPIOrelai[i],OUTPUT);
  digitalWrite (GPIOrelai[i],etat[i]);
 }
 */

}


void loop() {
// ElegantOTA.loop();

userScheduler.execute();
mesh.update();

 
}