#include <Arduino.h>
// programme relais


#include "painlessMesh.h"
#include <ArduinoJson.h>

#include <WiFi.h>
#include <WiFiClient.h>

//#include <ElegantOTA.h>

//const char* ssid = "knobuntufree";
//const char* password = "Pech_Vogel_free123";

String localip;


#define   MESH_PREFIX     "knobuntumesh"
#define   MESH_PASSWORD   "pechvogel"
#define   MESH_PORT       5555




Scheduler userScheduler; // Controluer 
painlessMesh  mesh;
int etat_relais[4];
int broches_relais[4]={19,18,17,16};


void receivedCallback( uint32_t from, String &msg ) {
  //activation  des relais

  JsonDocument doc;
  deserializeJson (doc,msg.c_str());
  if (!doc["ssr0"].isNull()){  // une cle ssr0 est trouvee alors les autres cles ssr  sont presentes 
    etat_relais[0]=int(doc["ssr0"]);
    etat_relais[1]=int(doc["ssr1"]);
    etat_relais[2]=int(doc["ssr2"]);
    etat_relais[3]=int(doc["ssr3"]);  
    
    
    for (int i=0;i<4;i++){
      (etat_relais[i]==0) ? digitalWrite(broches_relais[i],LOW ): digitalWrite(broches_relais[i],HIGH);
    }
    String str="ssr1 etatrelais:"+String(etat_relais[1]);
    mesh.sendBroadcast(str);
  }









  
  

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
 
 mesh.setDebugMsgTypes( ERROR | STARTUP );  // set before init() so that you can see startup messages

 mesh.init( MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT , WIFI_AP_STA, 6);
//mesh.init( MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT );
 mesh.onReceive(&receivedCallback);
 mesh.onNewConnection(&newConnectionCallback);
 mesh.onChangedConnections(&changedConnectionCallback);
mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);
for (int i=0;i<4;i++){
  pinMode(broches_relais[i],OUTPUT);
}


}


void loop() {

mesh.update();

 
}