#include <Arduino.h>
// programme shunt
//20260204

#include <ADS1X15.h>
#include <Wire.h>
//#include <Adafruit_GFX.h>
//#include <Adafruit_SSD1306.h>
#include "painlessMesh.h"
#include <ArduinoJson.h>
// #include <OneWire.h>
//#include <WiFi.h>
//#include <WiFiClient.h>

#include "esp_sleep.h"
#define uS_TO_S_FACTOR 1000000  // Conversion microsecondes → secondes




#define VERSION "1.0.2"
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
// #define ONE_WIRE_BUS 4
// OneWire oneWire(ONE_WIRE_BUS);
const int pinvolt_batterie =34;





// Structure pour sauvegarder les données en RTC memory
RTC_DATA_ATTR struct {
  float coef_tension_batterie;
  float coef_tension_AC;
  float coef_shunt;
  float coef_sct013;
  int transfert_coef;
} rtcData;


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
void enterDeepSleep(int seconds) {
  
  esp_sleep_enable_timer_wakeup(seconds * uS_TO_S_FACTOR);
  Serial.println("Entree en mode Deep Sleep pour " + String(seconds) + " secondes...");
  Serial.flush(); // Attendre la fin de l'envoi série
  esp_deep_sleep_start();
 
}

void sendMessage();  //prototype 

Task taskSendMessage( TASK_SECOND * 20 , TASK_FOREVER, &sendMessage ); //esecute la lecture des temperatures toutes les 5 secondes

/*
void recuperation_coef_ajustement(){;
  
  JsonDocument doc;
  String jsonstringP;
  //verifie si les coefficients d'ajustements sont définis
 
    doc["recepteur"] ="shunt";
    doc["coef"]="demandecoef";
    serializeJson(doc,jsonstringP);
    mesh.sendBroadcast(jsonstringP);
    Serial.println(jsonstringP);
    transfert_coef=1; // etat intermediaire en attente de reception

}
*/
float tension_batterie(){
//vs= ve(r4/(r4+v3)
//ve=vs(1+r3/r4)
//avec r4=100, r3 6.8k  vs voltage  A0

 int16_t mesure_A0=ADSgain4.readADC(0);
 float vs=ADSgain4.toVoltage(mesure_A0);
 coef_tension_batterie=3.12; float ve=vs*(1+6800.0/100.0)*coef_tension_batterie;
return ve;

}
float simple_Ubat()
{

 //4096 pour 3.3V
 float voltpin= analogRead(pinvolt_batterie);
 //ve=  (6800+330):330*volpin
 coef_tension_batterie=1.04;
 float tension = 21.6*voltpin*3.3/4096.0*coef_tension_batterie;
 return tension;

}


float shunt_batterie(){
//ddp (vsh) 75mv pour 100A
//vsh=2.5+ ddp(vsh)
//d'ou ibat=100*10³ /75*(vsh-2.5)
 int16_t mesure_diff23=ADSgain4.readADC_Differential_2_3();
 float vsh=ADSgain4.toVoltage(mesure_diff23);
 coef_shunt=1.0;  
 float ibat=100*1000.0/75.0*(vsh-2.5)*coef_shunt;
 return mesure_diff23;

}
float intensite_ac(){

//100a -->50mv :  -100a-->-50mv
//0a -->2.5v
//d'ou 1a-->50/100 mv+2.5v
//1mv -->100/50a
 int16_t mesure_A1=ADS.readADC(0);  // Lecture sur ADS (0x49)
 float vsh=ADS.toVoltage(mesure_A1);    // Conversion avec ADS
 float iac=(vsh-2.5)*100.0/50.0*coef_sct013;
 return iac;

}

void echantillonagezmpt(int16_t* zmpt_min, int16_t* zmpt_max){
  for (int i =0 ;i<2000; i++){
     int16_t mesure_A1=ADS.readADC(2);
     if (mesure_A1 < *zmpt_min){*zmpt_min= mesure_A1;}
     if (mesure_A1 > *zmpt_max){*zmpt_max= mesure_A1;}
    
  }
  
}


float zmpt101b_ac(){
  int16_t zmpt_min =32760; // valeur min et max de la sinusoide Volt AC
int16_t zmpt_max =0;

//voir courvbe zmpt101b
echantillonagezmpt(&zmpt_min,&zmpt_max);

 int16_t mesure_A1=ADS.readADC(2);
 int16_t moy_mesure=(zmpt_max-zmpt_min);
 float etalonnage=237.0/5750.0;
 
 coef_tension_AC=1;
 
Serial.print ("zmptmin : "); Serial.println (zmpt_min);
Serial.print ("zmpt_max : "); Serial.println (zmpt_max);
Serial.print ("moy : "); Serial.println (moy_mesure);

float vsh=moy_mesure*etalonnage;
Serial.print ("vsh "); Serial.println (vsh);
 return vsh;

}


  void sendMessage() {
  JsonDocument doc;
  doc["recepteur"] ="shunt";
  char txt[8];

  String jsonstringP;
 float shunt = shunt_batterie();
  float uac = zmpt101b_ac();
  float iac = intensite_ac();

  // Formatage avec 2 décimales
  // Mesures
  // float ubat = tension_batterie();
  float ubat =simple_Ubat();
  char buffer[10];
  dtostrf(ubat, 6, 2, buffer); doc["Ubat"] = buffer;
  // dtostrf(shunt, 6, 2, buffer); doc["shunt"] = buffer;
  dtostrf(uac, 6, 2, buffer); doc["Uac"] = buffer;
  dtostrf(iac, 6, 2, buffer); doc["Iac"] = buffer;
  doc["tcoef"] = transfert_coef;

  Serial.println("sendmessage"); 
 
  serializeJson(doc, jsonstringP);
  mesh.sendBroadcast(jsonstringP);
  Serial.println(jsonstringP);
 // ecritureoled(jsonstringP);
  delay(2000);

   // Sauvegarder les données avant le sleep
  rtcData.coef_tension_batterie = coef_tension_batterie;
  rtcData.coef_tension_AC = coef_tension_AC;
  rtcData.coef_shunt = coef_shunt;
  rtcData.coef_sct013 = coef_sct013;
  rtcData.transfert_coef = transfert_coef;

  // Entrer en Deep Sleep après l'envoi a voir plus tard
 // enterDeepSleep(30); // 30 secondes
 
}


void receivedCallback( uint32_t from, String &msg ) {

  JsonDocument doc;
  deserializeJson (doc,msg.c_str());
  //recuperation des varibles d'ajustement
  if (doc["recepteur"]=="shunt"){
    if (doc["coef"]=="envoicoef"){
      coef_shunt=doc["coef_shunt"];
      coef_tension_AC=doc["coef_Iac"];
      coef_tension_batterie=doc["coef_Ubat"];
      coef_sct013=doc["coef_Iac"];
      transfert_coef=2;
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
 coef_tension_batterie = rtcData.coef_tension_batterie;
 coef_tension_AC = rtcData.coef_tension_AC;
 coef_shunt = rtcData.coef_shunt; 
 coef_sct013 = rtcData.coef_sct013;
 transfert_coef = rtcData.transfert_coef;
 
 Wire.begin();
 // ADS.begin();
// ADSgain4.begin();
delay(100);
 //if (!ADSgain4.begin()) { Serial.println("ADS1115 (0x48) non trouvé !"); while (1); }
  if (!ADS.begin()) { Serial.println("ADS1115 (0x49) non trouvé !"); while (1); }
 //ADS.setGain(1);
// ADSgain4.begin();
 ADSgain4.setGain(4);


// Initialisation du mesh
mesh.setDebugMsgTypes( ERROR | STARTUP );  // set before init() so that you can see startup messages
mesh.init( MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT , WIFI_AP_STA, 6);
mesh.onReceive(&receivedCallback);
mesh.onNewConnection(&newConnectionCallback);
mesh.onChangedConnections(&changedConnectionCallback);
mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);
userScheduler.addTask(taskSendMessage);
taskSendMessage.enable();
Serial.println("Setup termine, envoi du premier message...");


delay(2000);



}

void loop() {

userScheduler.execute();
mesh.update();

 
}