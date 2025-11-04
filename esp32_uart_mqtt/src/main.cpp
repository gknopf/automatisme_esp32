#include <Arduino.h>

#include <WiFi.h>
#include <PubSubClient.h>

#define RXD2 16
#define TXD2 17
#define TRANSFERT_BAUD 9600



//#include <Wire.h>

// Replace the next variables with your SSID/Password combination
const char* ssid = "knobuntufree";
const char* password = "Pech_Vogel_free123";

// Add your MQTT Broker address, example:
const char* mqtt_server = "192.168.1.140";
const char* unique_identifier = "knobuntumesh";

WiFiClient espClient;
PubSubClient client(espClient);
HardwareSer.ial mySerial(2);
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

void callback(char* topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;

  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();

  // If a message is received on the topic "SF/LED", you check if the message is either "on" or "off".
  // Changes the output state according to the message
  if (String(topic) == "SF/LED") {
    Serial.print("Changing state to ");
    if (messageTemp == "on") {
      Serial.println("on");
      digitalWrite(ledPin, HIGH);
    } else if (messageTemp == "off") {
      Serial.println("off");
      digitalWrite(ledPin, LOW);
    }
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(unique_identifier)) {
      Serial.println("connected");
      // Subscribe
      client.subscribe("esp32/jsonstring");
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
 
      

