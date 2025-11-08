

// programme esp32_uart_mqtt

#include <Arduino.h>

#include <WiFi.h>
#include <PubSubClient.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

#define RXD2 16
#define TXD2 17
#define TRANSFERT_BAUD 9600
HardwareSerial mySerial(2);


//#include <Wire.h>

// Replace the next variables with your SSID/Password combination
const char* ssid ;// = "knobuntulink";
const char* password ;// = "pechvogel";

// Add your MQTT Broker address, example:
const char* mqtt_server;// = "192.168.1.140";
const char* unique_identifier; // = "knobuntumesh";

 WiFiClient espClient;
 PubSubClient client(espClient);
 
// essai d'ajout


long lastMsg = 0;
int value = 0;


// LED Pin
const int ledPin = 4;
const int buttonPin = 14;

// When you connect to WIFI, only 36 39 34 35 32 33 pins can be used for analog reading.
// Define constants

void setup_wifi();
void callback(char* topic, byte* message, unsigned int length);



void setup() {
  Serial.begin(115200);
  Serial.print("coucou");
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
    
  mySerial.begin(TRANSFERT_BAUD,SERIAL_8N1,RXD2,TXD2);

  // default settings
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  pinMode(buttonPin, INPUT);
  pinMode(ledPin, OUTPUT);
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

void callback(char* topic, uint8_t* payload, unsigned int length) {
  char* cleanPayload = (char*)malloc(length+1);
  payload[length] = '\0';
  memcpy(cleanPayload, payload, length+1);
  String msg = String(cleanPayload);
  free(cleanPayload);
  if (strcmp(topic,"esp32/relais")==0){
    mySerial.println (msg);
     
  }else{
    if (strcmp(topic,"esp32/noeud")==0){
      mySerial.println (msg);
    
    }
  }
 }

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(unique_identifier)) {
      Serial.println("cconnecte au reseau knobuntulink");
      // Subscribe
      client.subscribe("esp32/jsonstring");
      client.subscribe ("esp32/relais");
      client.publish("esp32","connecte au reseau knobuntutplink sur raspberry 192.168..140");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}



void loop() {

  String mystr;
  const char * jsonstring;


  if (!client.connected()) {
    reconnect();
  }
 
  client.loop();
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
 
      

