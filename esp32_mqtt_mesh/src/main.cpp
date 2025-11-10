

// programme esp32_uart_mqtt

#include <Arduino.h>

#include <WiFi.h>
#include <painlessMesh.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#include <LittleFS.h>
#include <ArduinoJson.h>




#define   MESH_PREFIX     "knobuntumesh"
#define   MESH_PASSWORD   "pechvogel"
#define   MESH_PORT       5555

// Prototypes
void receivedCallback( const uint32_t &from, const String &msg );
void mqttCallback(char* topic, byte* payload, unsigned int length);
void setup_wifi();
//void callback(char* topic, byte* message, unsigned int length);
//
// Replace the next variables with your SSID/Password combination
const char* ssid ;// = "knobuntulink";
const char* password ;// = "pechvogel";

// Add your MQTT Broker address, example:
const char* mqtt_server;// = "192.168.1.140";
const char* unique_identifier; // = "knobuntumesh";

painlessMesh  mesh;
WiFiClient wifiClient;
PubSubClient mqttClient(mqtt_server, 1883, mqttCallback, wifiClient);



const char* topicrelai[4]={"ssr0","ssr1","ssr2","ssr3"};

const char* jsonstring =" ";





void setup() {
  Serial.begin(115200);
 
  //lecture du fichier secret
  if (!LittleFS.begin(true)){
    Serial.print( "erreur de lecture littlefs");
    return;

  }
  
  File file =LittleFS.open("/secret.txt","r");
  if(!file){
    Serial.println("Failed to open file for reading");
    return;
  }
  String str;
  JsonDocument doc;
  while (file.available()){
   char r=file.read();
   str +=r;
  
  }
  deserializeJson(doc,str);
  ssid=doc["ssid"];
  password=doc["password"];
  mqtt_server=doc["mqtt_server"];
  unique_identifier=doc["unique_identifier"];
  
  Serial.println(ssid);
  Serial.println(password);
  Serial.println(mqtt_server);
  Serial.println(unique_identifier);
    

  // default settings
  setup_wifi();
  mqttClient.setServer(mqtt_server, 1883);
  mqttClient.setCallback(mqttCallback);


  //initialisation mesh
  mesh.setDebugMsgTypes( ERROR | STARTUP | CONNECTION );  // set before init() so that you can see startup messages

  mesh.init( MESH_PREFIX, MESH_PASSWORD, MESH_PORT, WIFI_AP_STA, 6 );
  mesh.onReceive(&receivedCallback);
  mesh.stationManual(ssid, password);
  mesh.setRoot(true);
  mesh.setContainsRoot(true);  

 
}

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  
}

//Reception depuis le MESH renvoi vers le MQTT
void receivedCallback( const uint32_t &from, const String &msg ) {
  Serial.printf("Mosquitto Received from %u msg=%s\n", from, msg.c_str());
  String topic = "esp32/jsonstring" ;
  mqttClient.publish(topic.c_str(), msg.c_str()); //renvoi jsonstring sur node-red

}

void mqttcallback(char* topic, uint8_t* payload, unsigned int length) {
  char* cleanPayload = (char*)malloc(length+1);
  payload[length] = '\0';
  memcpy(cleanPayload, payload, length+1);
  String msg = String(cleanPayload);
  free(cleanPayload);
  if (strcmp(topic,"esp32/relais")==0){
    mesh.sendBroadcast(msg);
     
  }else{
    if (strcmp(topic,"esp32/noeud")==0){
      String noeudsjson=mesh.subConnectionJson();
       auto nodes = mesh.getNodeList(true);
      String str;
      for (auto &&id : nodes){
        str += String(id) + String(" \r\n");
      }    
      mqttClient.publish("esp32/noeud",noeudsjson.c_str(),sizeof(noeudsjson) );
    } 
  }
 }

void reconnect() {
  // Loop until we're reconnected
  while (!mqttClient.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (mqttClient.connect(unique_identifier)) {
      Serial.println("cconnecte au reseau knobuntulink");
      // Subscribe
      mqttClient.subscribe("esp32/jsonstring");
      mqttClient.subscribe ("esp32/relais");
      mqttClient.publish("esp32","connecte au reseau knobuntutplink sur raspberry 192.168..140");
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}



void loop() {

  String mystr;
  const char * jsonstring;


  if (!mqttClient.connected()) {
    reconnect();
  }
 
  mqttClient.loop();
  if (mySerial.available()>0){
    mystr=mySerial.readStringUntil('\n');
    
    int position = mystr.indexOf("Sonnenkraft");
    if(position>-1) {
      String  substr=mystr.substring(16);
      jsonstring=substr.c_str();
      client.publish("esp32/sonnenkraft", jsonstring);
    } else{
      int position = mystr.indexOf("bouilleur");
      if(position>-1) {
        String  substr=mystr.substring(16);
        jsonstring=substr.c_str();
        client.publish("esp32/bouilleur", jsonstring);
      }else{
        int position = mystr.indexOf("bassin");
        if(position>-1) {
          String  substr=mystr.substring(16);
          jsonstring=substr.c_str();
          client.publish("esp32/bassin", jsonstring);     
        } 
      }
    }    
  }

  
}
 
      

