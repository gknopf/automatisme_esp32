#include <Arduino.h>
// programme bouilleur pt100 et lecture  de pression
// prise en compte des 2 relais 
//17 janvier 2026

#include <ADS1X15.h>

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "painlessMesh.h"
#include <ArduinoJson.h>
#include <OneWire.h>
#include <WiFi.h>
#include <WiFiClient.h>


String localip;
 ADS1115 ADS(0x48);

#define   MESH_PREFIX     "knobuntumesh"
#define   MESH_PASSWORD   "pechvogel"
#define   MESH_PORT       5555
#define RXD2 16
#define TXD2 17
#define TRANSFERT_BAUD 9600
#define SCREEN_WIDTH 128 //largeur oled
#define SCREEN_HEIGHT 64 //hauteur oled

Adafruit_SSD1306 oled(SCREEN_WIDTH, SCREEN_HEIGHT, & Wire, -1);


Scheduler userScheduler; // Controluer 
painlessMesh  mesh;

//initialisation materiel
float E = 5.0; //volts
int R1 = 138; // ohm
float Rpt100; // resistance sonde PT100
float Tpt100; // temperature sonde PT100
int etat_relais[2];
int broches_relais[2]={12,13};


HardwareSerial mySerial(2);



#define ONE_WIRE_BUS 4
OneWire oneWire(ONE_WIRE_BUS);


void sendMessage();  //prototype 

Task taskSendMessage( TASK_SECOND * 5 , TASK_FOREVER, &sendMessage ); //esecute la lecture des temperatures toutes les 5 secondes

void ecritureoled(String msg ){
  JsonDocument doc;
  float t0,t1,p0,p1;
  String relai;
  char txt[20];
  deserializeJson (doc,msg.c_str());

  oled.clearDisplay();
  oled.setTextSize(2);
  oled.setTextColor(WHITE);
  t0=doc["Tp100"][0];
  t1=doc["Tp100"][1];
  p0=doc["pression"][0];
  p1=doc["pression"][1];
  (etat_relais[0]==0)?relai="OFF":relai="ON";
  
  
 
 
  oled.setCursor(0,0);
  sprintf(txt,"eauC:%.1f\0",t0);
  oled.println(txt);
  oled.setCursor(0,19);
  sprintf(txt,"eauF:%.1f\0",t1);
  oled.println(txt);

  oled.setCursor(0,38);
  sprintf(txt,"P:%.1fhPa\0",p0);
  oled.println(txt);

  oled.setTextSize(1);
  oled.setCursor(4,56);
  sprintf(txt,"relai %s",relai);
  oled.println(txt);

  oled.display();
  delay(2000);
  
}



void sendMessage() {
  JsonDocument doc;
  doc["recepteur"] ="bouilleur";
  
  JsonArray PT100 = doc["Tp100"].to<JsonArray>();
  JsonArray PRESSION =doc["pression"].to<JsonArray>();
 // JsonArray relaiPT100 = doc["relai"].to<JsonArray>();
  String jsonstringP;
  int16_t mesure_0=ADS.readADC(0);
  // lecture des 2 valeurs de temprerature en a0 et a1
  for(int i=0;i<2; i++){ 
    int16_t mesure_Tp100=ADS.readADC(i);
    float tension=ADS.toVoltage(mesure_Tp100);
    Rpt100 = tension*R1/(E-tension);
    Tpt100 = (Rpt100-100)/0.385;
    PT100.add(Tpt100); 
    
  } 

 for(int i=2;i<4; i++){ 
    int16_t mesure_pression=ADS.readADC(i);
    float tension=ADS.toVoltage(mesure_pression);   
    // pression 10 bar filletage 1/4"
    //10bars-->5v ---- 1bar -->0.5v
    float pression_bar = tension/10;
    PRESSION.add(pression_bar); 
    
  } 


 
  serializeJson(doc,jsonstringP);
   mesh.sendBroadcast(jsonstringP);
  Serial.println(jsonstringP);
  ecritureoled(jsonstringP);
  delay(2000);
 
}




void receivedCallback( uint32_t from, String &msg ) {
 //activation  des relais
 JsonDocument doc;
 deserializeJson (doc,msg.c_str());
 
  if (!doc["ssr0"].isNull()){  // une cle ssr0 est trouvee alors les autres cles ssr  sont presentes 
    etat_relais[0]=int(doc["ssr5"]);
    etat_relais[1]=int(doc["ssr6"]);
    for (int i=0;i<2;i++){
      (etat_relais[i]==0) ? digitalWrite(broches_relais[i],LOW ): digitalWrite(broches_relais[i],HIGH);
    }
    String str="ssr1 etatrelais:1";
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
char txt[50];
float t0,t1;
 
  Wire.begin();
  ADS.begin();
  ADS.setGain(1);

 
  mesh.setDebugMsgTypes( ERROR | STARTUP );  // set before init() so that you can see startup messages

  mesh.init( MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT , WIFI_AP_STA, 6);
   mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);
  userScheduler.addTask(taskSendMessage);
  taskSendMessage.enable();


  
  //oled
   if (!oled.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("defaut oled SSD1306 OLED"));
    while (1);
  }
  oled.clearDisplay();

 
  delay(2000);
 
}


void loop() {

userScheduler.execute();
mesh.update();

 
}