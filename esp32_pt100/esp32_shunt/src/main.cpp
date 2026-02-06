#include <Arduino.h>
// programme shunt
//20260204

#include <ADS1X15.h>
#include <Wire.h>
//#include <Adafruit_GFX.h>
//#include <Adafruit_SSD1306.h>
#include "painlessMesh.h"
#include <ArduinoJson.h>
#include <OneWire.h>
//#include <WiFi.h>
//#include <WiFiClient.h>


#define SCREEN_WIDTH 128 //largeur oled
#define SCREEN_HEIGHT 64 //hauteur oled
#define   MESH_PREFIX     "knobuntumesh"
#define   MESH_PASSWORD   "pechvogel"


#define   MESH_PORT       5555
#define TRANSFERT_BAUD 9600


//Adafruit_SSD1306 oled(SCREEN_WIDTH, SCREEN_HEIGHT, & Wire, -1);
String localip;
ADS1115 ADS(0x49);
ADS1115 ADSgain4(0x48);

#define ONE_WIRE_BUS 4
OneWire oneWire(ONE_WIRE_BUS);







//coefficients d'ajustement definis dans node-red  par defaut =1
float coef_tension_batterie = 1;
float coef_tension_AC=1 ;
float coef_shunt =1;
float coef_sct013 =1;
int transfert_coef=0;

Scheduler userScheduler; // Controluer 
painlessMesh  mesh;
/*
void ecritureoled(String msg ){
  JsonDocument doc;
  float ubat,shunt,voltAC;
  String relai;
  char txt[20];
  deserializeJson (doc,msg.c_str());

  oled.clearDisplay();
  oled.setTextSize(2);
  oled.setTextColor(WHITE);
  ubat=doc["tension_batterie"][0];
  shunt=doc["intensite_batterie"][1];
  voltAC=doc["tension_AC"][0];

  oled.setCursor(0,0);
  sprintf(txt,"ubat:%.1f\0",ubat);
  oled.println(txt);
  oled.setCursor(0,19);
  sprintf(txt,"shunt:%.1f\0",shunt);
  oled.println(txt);

  oled.setCursor(0,38);
  sprintf(txt,"P:%.1fhPa\0",voltAC);
  oled.println(txt);

  oled.setTextSize(1);
  oled.setCursor(4,56);
  sprintf(txt,"coef_shunt %s",coef_shunt);
  oled.println(txt);

  oled.display();
  delay(2000);
  
}


*/


void sendMessage();  //prototype 

Task taskSendMessage( TASK_SECOND * 5 , TASK_FOREVER, &sendMessage ); //esecute la lecture des temperatures toutes les 5 secondes


void recuperation_coef_ajustement(){;
  
  JsonDocument doc;
  String jsonstringP;
  //verifie si les coefficients d'ajustements sont définis
  if(transfert_coef==0){  // seulement dans le cas ou le transfert n'a pas encore ete demande
    doc["recepteur"] ="shunt";
    doc["coef"]="demandecoef";
    serializeJson(doc,jsonstringP);
    mesh.sendBroadcast(jsonstringP);
    Serial.println(jsonstringP);
    transfert_coef=1; // etat intermediaire en attente de reception

  }
}

float tension_batterie(){
//vs= ve(r4/(r4+v3)
//ve=vs(1+r3/r4)
//avec r4=100, r3 6.8k  vs voltage  A0

 int16_t mesure_A0=ADSgain4.readADC(0);
 float vs=ADSgain4.toVoltage(0);
 float ve=vs*(1+6800/100)*coef_tension_batterie;
return ve;
 

}
 
float shunt_batterie(){
//ddp (vsh) 75mv pour 100A
//vsh=2.5+ ddp(vsh)
//d'ou ibat=100*10³ /75*(vsh-2.5)
 int16_t mesure_A1=ADSgain4.readADC(3);
 float vsh=ADSgain4.toVoltage(3);
 float ibat=100*1000/75*(vsh-2.5)*coef_shunt;
 return ibat;

}
float intensite_ac(){
//100a -->50mv :  -100a-->-50mv
//0a -->2.5v
//d'ou 1a-->50/100 mv+2.5v
//1mv -->100/50a
 int16_t mesure_A1=ADS.readADC(0);
 float vsh=ADSgain4.toVoltage(0);
 float iac=(vsh-2.5)*100/50*coef_sct013;
 return iac;

}
float zmpt101b_ac(){
//voir courvbe zmpt101b

 int16_t mesure_A1=ADS.readADC(2);
 float vsh=ADSgain4.toVoltage(2)*coef_tension_AC;
 
 return vsh;

}


void sendMessage() {
  JsonDocument doc;
  doc["recepteur"] ="shunt";
  char txt[8];

  String jsonstringP;
  Serial.println("sendmessage"); 
  sprintf(txt,"%1f",tension_batterie());
  doc["tension_batterie"] = txt;
  
  sprintf(txt,"%1f",shunt_batterie());
  doc["intensite_batterie"]=txt;
  sprintf(txt,"%1f",zmpt101b_ac());
  doc["tension_AC"]=txt;
  sprintf(txt,"%1f",intensite_ac());
  doc["intensite_AC"]=txt;
  sprintf(txt,"%1f",tension_batterie());
  doc["transfert_coef"]=transfert_coef;
 
  serializeJson(doc,jsonstringP);
  mesh.sendBroadcast(jsonstringP);
  Serial.println(jsonstringP);
 // ecritureoled(jsonstringP);
  delay(2000);
 
}


void receivedCallback( uint32_t from, String &msg ) {

  JsonDocument doc;
  deserializeJson (doc,msg.c_str());
  //recuperation des varibles d'ajustement
  if (doc["recepteur"]=="shunt"){
    if (doc["coef"]=="envoicoef"){
      coef_shunt=doc["coef_shunt"];
      coef_tension_AC=doc["coef_tension_AC"];
      coef_tension_batterie=doc["coef_tension_batterie"];
      coef_sct013=doc["coefsct13"];
      transfert_coef=2; //reception des coef
    }
 }

}

void newConnectionCallback(uint32_t nodeId) {
    Serial.printf("--> startHere: New Connection, nodeId = %u\n", nodeId);
}

void changedConnectionCallback() {
  Serial.printf("Changed connections\n");
}

void nodeTimeAdjustedCallback(int32_t offset) {
    Serial.printf("Adjusted time %u. Offset = %d\n", mesh.getNodeTime(),offset);
}


void setup() {

 Serial.begin(115200);
 
 Wire.begin();
 ADS.begin();
 ADS.setGain(1);
 ADSgain4.begin();
 ADSgain4.setGain(4);
 


mesh.setDebugMsgTypes( ERROR | STARTUP );  // set before init() so that you can see startup messages
mesh.init( MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT , WIFI_AP_STA, 6);
mesh.onReceive(&receivedCallback);
mesh.onNewConnection(&newConnectionCallback);
mesh.onChangedConnections(&changedConnectionCallback);
mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);
userScheduler.addTask(taskSendMessage);
taskSendMessage.enable();

recuperation_coef_ajustement();
delay(2000);



}

void loop() {

userScheduler.execute();
mesh.update();

 
}