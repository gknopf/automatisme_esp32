//esp32_uart _mesh

#include <Arduino.h>
#include <painlessMesh.h>
// #include <PubSubClient.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>

#define   MESH_PREFIX     "knobuntumesh"
#define   MESH_PASSWORD   "pechvogel"
#define   MESH_PORT       5555

#define   STATION_SSID     "knobuntufree"
#define   STATION_PASSWORD "Pech_Vogel_free123"
#define RXD2 16
#define TXD2 17
#define TRANFERT_BAUD 9600


// Prototypes
void receivedCallback( const uint32_t &from, const String &msg );
//void mqttCallback(char* topic, byte* payload, unsigned int length);

IPAddress getlocalIP();

IPAddress myIP(0,0,0,0);
IPAddress mqttBroker(192, 168, 1, 140);
boolean ErreurBroker = true;
painlessMesh  mesh;
WiFiClient wifiClient;
//PubSubClient mqttClient(mqttBroker, 1883, mqttCallback, wifiClient);

HardwareSerial mySerial(2);

const char* topicrelai[4]={"esp/relai0","esp/relai1","esp/relai2","esp/relai3"};

const char* jsonstring =" ";




void setup() {
  Serial.begin(115200);

  mySerial.begin(TRANFERT_BAUD, SERIAL_8N1, RXD2, TXD2);
  Serial.println("Serial 2 started at 9600 baud rate");


  mesh.setDebugMsgTypes( ERROR | STARTUP | CONNECTION );  // set before init() so that you can see startup messages

  // Channel set to 6. Make sure to use the same channel for your mesh and for you other
  // network (STATION_SSID)
  mesh.init( MESH_PREFIX, MESH_PASSWORD, MESH_PORT, WIFI_AP_STA, 6 );
  mesh.onReceive(&receivedCallback);

  mesh.stationManual(STATION_SSID, STATION_PASSWORD);
 

  // Bridge node, should (in most cases) be a root node. See [the wiki](https://gitlab.com/painlessMesh/painlessMesh/wikis/Possible-challenges-in-mesh-formation) for some background
  mesh.setRoot(true);
  // This node and all other nodes should ideally know the mesh contains a root, so call this on all nodes
  mesh.setContainsRoot(true);
  mesh.stop();
}  

void loop() {

while (mySerial.available() > 0){
    // get the byte data from the jsonstring
    char transfertData =mySerial.read();
    Serial.print(transfertData);
  }
  delay(1000);



  mesh.update();
 
/*
  mqttClient.loop();

  if (myIP != getlocalIP()) {               //pas d'adresse IP correcte
    myIP = getlocalIP();                    //mise a jour de l'IP locale
    if (!mqttClient.connected() ) {         //pas de connexion au Broker 
       if (mqttClient.connect("wifiClient")) {  //tentative de connexion 
          // si connexion établie
          //mqttClient.publish("esp8266/jsonstring","connection");      // envoi une information de connexion via TOPIC
          mqttClient.subscribe("esp8266/jsonstring"); 
          mqttClient.subscribe("esp/relai");  
          mqttClient.subscribe("esp/noeud");             // abonnement au TOPIC esp/relai0 a 3
          ErreurBroker= false;
          }
          else {
            ErreurBroker= true;
          }
      }
    }
*/
  //ajout d'une gestion de la deconnexion.

}


//Reception depuis le MESH renvoi vers le MQTT
void receivedCallback( const uint32_t &from, const String &msg ) {
  Serial.printf("Mosquitto Received from %u msg=%s\n", from, msg.c_str());
  String topic = "esp32/jsonstring" ;
  mySerial.printf("esp32/jsonstring%s\n",msg.c_str());
  Serial.printf("Mosquitto Received from %u msg=%s\n", from, msg.c_str());

 // mqttClient.publish(topic.c_str(), msg.c_str()); //renvoi jsonstring sur node-red

}


//************************************************************************************

//  Cette fonction va renvoyer sur le MESH les messages en provenance du BRIDGE

//************************************************************************************

/*
void mqttCallback(char* topic, uint8_t* payload, unsigned int length) {
 
  char* cleanPayload = (char*)malloc(length+1);
  payload[length] = '\0';
  memcpy(cleanPayload, payload, length+1);
  String msg = String(cleanPayload);
  free(cleanPayload);
  if (strcmp(topic,"esp/relai")==0){
     mesh.sendBroadcast(msg);
 
  }else{
    if (strcmp(topic,"esp/noeud")==0){
     String noeudsjson=mesh.subConnectionJson();
    
     
      auto nodes = mesh.getNodeList(true);
      String str;
      for (auto &&id : nodes){
        str += String(id) + String(" \r\n");
      }    
   
    mqttClient.publish("esp/noeud",noeudsjson.c_str(),sizeof(noeudsjson) );
    }
  }
   
}

*/

IPAddress getlocalIP() {
  return IPAddress(mesh.getStationIP());
}