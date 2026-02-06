//esp32_uart _mesh

#include <Arduino.h>
#include <painlessMesh.h>
// #include <PubSubClient.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>

#define   MESH_PREFIX     "knobuntumesh"
#define   MESH_PASSWORD   "pechvogel"
#define   MESH_PORT       5555

//#define   STATION_SSID     "knobuntufree"
//#define   STATION_PASSWORD "Pech_Vogel_free123"
#define   STATION_SSID     "knobuntulink"
#define   STATION_PASSWORD "pechvogel"

#define RXD2 16
#define TXD2 17
#define TRANSFERT_BAUD 9600

// Prototypes
void receivedCallback( const uint32_t &from, const String &msg );


//IPAddress getlocalIP();

IPAddress myIP(0,0,0,0);
IPAddress mqttBroker(192, 168, 1, 140);
boolean ErreurBroker = true;
painlessMesh  mesh;
WiFiClient wifiClient;
//PubSubClient mqttClient(mqttBroker, 1883, mqttCallback, wifiClient);

HardwareSerial mySerial(2);




//const char* jsonstring =" ";




void setup() {
  Serial.begin(115200);

  mySerial.begin(TRANSFERT_BAUD , SERIAL_8N1, RXD2, TXD2);
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
  
}  

void loop() {
 

 
  String mystr;
  const char * jsonstring;

//diffusion des valeurs de reais vers les esp
//sens mesh vers programme bouilleur et relais
//format du message json {"ssr0":1 "ssr1":0 "ssr2": ..... "ssr5":0}
//ssr5 et 6 activent bouilleur
  if (mySerial.available()>0){
    mystr=mySerial.readStringUntil('\n');
    
    int position = mystr.indexOf("ssr");
    if(position>-1) {
     
      jsonstring=mystr.c_str();
      mesh.sendBroadcast(jsonstring);
      Serial.print ("mysr dans ssr decouvert : ");
      Serial.println(mystr);

    }else{
      int position = mystr.indexOf("coef");
      if(position>-1) {
     
        jsonstring=mystr.c_str();
        mesh.sendBroadcast(jsonstring);
        Serial.print ("renvoi des coef: ");
        Serial.println(mystr);

      }
    }

  }



  Serial.println (ESP.getFreeHeap());

  delay(1000);


  mesh.update();
 



}


//Reception depuis le MESH renvoi vers le MQTT
//renvoi via myserial vers esp32_uart_mqtt
//message de la forme  "esp32/jsonstring{"recepteur":"bouilleur", "PT100":[12.4,13.6,25.4]......}
void receivedCallback( const uint32_t &from, const String &msg ) {
  //Serial.printf("Mosquitto Received from %u msg=%s\n", from, msg.c_str());
  String topic = "esp32/jsonstring" ;
  mySerial.printf("esp32/jsonstring%s\n",msg.c_str());
  Serial.printf("Mosquitto Received from %u msg=%s\n", from, msg.c_str());


}


//IPAddress getlocalIP() {
//  return IPAddress(mesh.getStationIP());
//}