#include <Arduino.h>
// fusion de relais et pr100


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
#define RXD2 16
#define TXD2 17
#define TRANSFERT_BAUD 9600




Scheduler userScheduler; // Controluer 
painlessMesh  mesh;

//initialisation materiel
float E = 5.0; //volts
int R1 = 138; // ohm
float Rpt100; // resistance sonde PT100
float Tpt100; // temperature sonde PT100
//relais
int etat_relais[2]; //0 ou 1
int broches_relais[2]={12,13}; //acctionneur des relais


//HardwareSerial mySerial(2);



#define ONE_WIRE_BUS 4
OneWire oneWire(ONE_WIRE_BUS);




void receivedCallback( uint32_t from, String &msg ) {
  //activation  des relais

  JsonDocument doc;
  deserializeJson (doc,msg.c_str());
  if (!doc["ssr0"].isNull()){  // une cle ssr0 est trouvee alors les autres cles ssr  sont presentes 
    etat_relais[0]=int(doc["ssr0"]);
    etat_relais[1]=int(doc["ssr1"]);
    for (int i=0;i<2;i++){
      (etat_relais[i]==0) ? digitalWrite(broches_relais[i],LOW ): digitalWrite(broches_relais[i],HIGH);
    }
    String str="ssr1 etatrelais:"+String(etat_relais[1]);
    mesh.sendBroadcast(str);
  }

}





void sendMessage();  //prototype 

Task taskSendMessage( TASK_SECOND * 5 , TASK_FOREVER, &sendMessage ); //esecute la lecture des temperatures toutes les 5 secondes





void sendMessage() {
  JsonDocument doc;
  doc["recepteur"] ="bouilleur";
  
  JsonArray PT100 = doc["Tp100"].to<JsonArray>();
  JsonArray Pression = doc["pression"].to<JsonArray>();

 // JsonArray relaiPT100 = doc["relai"].to<JsonArray>();
  String jsonstringP;
  int16_t mesure_0=ADS.readADC(0);
  for(int i=0;i<2; i++){ 
    int16_t mesure_Tp100=ADS.readADC(i);
    float tension=ADS.toVoltage(mesure_Tp100);
    Rpt100 = tension*R1/(E-tension);
    Tpt100 = (Rpt100-100)/0.385;
    PT100.add(Tpt100); 
    
  } 
  //lecture pression  nouveau a completer
  for(int i=2;i<2; i++){ 
    int16_t mesure_Pression=ADS.readADC(i);
    Pression.add(mesure_Pression); 
  } 
 
  // recopie des valeurs des 2 relais dans jsonstring
  doc["ssr0"]=etat_relais[0];
  doc["ssr1"]=etat_relais[1];

  serializeJson(doc,jsonstringP);
   mesh.sendBroadcast(jsonstringP);
  Serial.println(jsonstringP);

    
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
 // mySerial.begin(TRANSFERT_BAUD,SERIAL_8N1,RXD2,TXD2);
 // initialisation de etatrelais à0
 for (int i=0;i<2;i++){
   etat_relais[i]=0;
 }
 //pinMode(13,OUTPUT);
 
// Serial.println("pont diviseur");
 Wire.begin();
 ADS.begin();
ADS.setGain(1);

 //  digitalWrite(13,HIGH);
//digitalWrite(13,HIGH);
 mesh.setDebugMsgTypes( ERROR | STARTUP );  // set before init() so that you can see startup messages

 mesh.init( MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT , WIFI_AP_STA, 6);

 mesh.onReceive(&receivedCallback);
 mesh.onNewConnection(&newConnectionCallback);
 mesh.onChangedConnections(&changedConnectionCallback);
mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);
userScheduler.addTask(taskSendMessage);
taskSendMessage.enable();

for(int i=0; i<2;i++){
   pinMode(broches_relais[i],OUTPUT);
 
 }


}


void loop() {
// ElegantOTA.loop();

userScheduler.execute();
mesh.update();

 
}