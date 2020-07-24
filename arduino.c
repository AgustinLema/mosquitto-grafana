#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include "DHT.h"
#include <PubSubClient.h>

// Screen libraries
#include <Arduino.h>
#include <U8g2lib.h>
#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif
U8G2_SH1106_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);
//SDA goes to D2 and SCK GOES TO D1


// IR libraries
#include <i18n.h>
#include <IRac.h>
#include <IRrecv.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <IRtext.h>
#include <IRtimer.h>
#include <IRutils.h>
#include <ir_NEC.h>


// Uncomment one of the lines below for whatever DHT sensor type you're using!
//#define DHTTYPE DHT11   // DHT 11
//#define DHTTYPE DHT21   // DHT 21 (AM2301)
#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321

#define DHT_PIN D7 // DHT Sensor

IRsend irsend(D5);


/*Put your SSID & Password*/
const char* ssid = // Enter SSID here
const char* password = //Enter Password here
const char* mqtt_server = "192.168.0.228";
//const char* mqtt_server = "192.168.0.110";

const char* temperature_topic = "arduino/out/temperature";
const char* humidity_topic = "arduino/out/humidity";
const char* events_topic = "arduino/out/events";

const char* inboundTopic = "arduino/in/actions";

ESP8266WebServer server(80);
WiFiClient espClient;
PubSubClient mqttClient(espClient);

DHT dht(DHT_PIN, DHTTYPE); // Initialize DHT sensor.
        
float Temperature;
float Humidity;
int loopCount = 10000; // We use this to be able to have a loop every half second but check temp once every 30 seconds.
int MQTT_retries = 0;

// Variables for temp control
float lowerTempBound = 17;
float upperTempBound = 24;
bool powerStatus = false;


void setup() {
    Serial.begin(115200);
    delay(100);
    
    pinMode(DHT_PIN, INPUT);
    pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output

    irsend.begin();
    dht.begin();

    //Setup screen
    u8g2.begin();
    u8g2.setFont(u8g2_font_6x10_tr);
  
    showAvailableNetworks();
    wifiConnect();

    drawMessage("Setting up web server");
    setupWebserver();
    setupMQTT();
    drawMessage("Setup completed");
}


void loop() {
  server.handleClient(); // Check if there is any HTTP request
  if (!mqttClient.connected()) {
    drawMessage("Reconnecting to MQTT broker");
    mqttReconnect(); // Reconnect to subscribed topic in case of disconnection
  }
  mqttClient.loop(); // Check if there is any inbound message from mqtt topic

  if (loopCount >= 10) { // 10 loops, half second each = run post of temp every 5 seconds
      loopCount = 0;
      postDataToMQTT();
  }

  loopCount += 1;
  delay(500);
}


void wifiConnect() {
    drawMessage("Starting");
    IPAddress ip(192,168,0,22);     
    IPAddress gateway(192,168,0,1);   
    IPAddress subnet(255,255,255,0); 
  
    //connect to your local wi-fi network
    WiFi.mode(WIFI_STA);
    WiFi.config(ip, gateway, subnet);
    WiFi.begin(ssid, password);
  
    Serial.println("Connecting to ");
    Serial.println(ssid);
    drawMessage("Connecting to WiFi");
  
    //check wi-fi is connected to wi-fi network
    while (WiFi.status() != WL_CONNECTED) {
      delay(1000);
      Serial.print(".");
    }
    Serial.println("");
    Serial.println("WiFi connected..!");
    Serial.print("Got IP: ");  Serial.println(WiFi.localIP());

    drawMessage("Connected");
}

void showAvailableNetworks() {
    Serial.println("scan start");
  
    // WiFi.scanNetworks will return the number of networks found
    int n = WiFi.scanNetworks();
    Serial.println("scan done");
    if (n == 0)
      Serial.println("no networks found");
    else
    {
      Serial.print(n);
      Serial.println(" networks found");
      for (int i = 0; i < n; ++i)
      {
        // Print SSID and RSSI for each network found
        Serial.print(i + 1);
        Serial.print(": ");
        Serial.print(WiFi.SSID(i));
        Serial.print(" (");
        Serial.print(WiFi.RSSI(i));
        Serial.print(")");
        Serial.println((WiFi.encryptionType(i) == ENC_TYPE_NONE)?" ":"*");
        delay(10);
      }
    }
    Serial.println("");
}

void postDataToMQTT() {
  Temperature = dht.readTemperature(); // Gets the values of the temperature
  Humidity = dht.readHumidity(); // Gets the values of the humidity 
  Serial.println("Reading temp");
  Serial.print("Publish message: ");
  Serial.print(Temperature);
  Serial.print(" / ");
  Serial.println(Humidity);
  mqttClient.publish(temperature_topic, String(Temperature).c_str());
  mqttClient.publish(humidity_topic, String(Humidity).c_str());
  //mqttClient.publish(temperature_topic, String(20).c_str());
  redrawScreen();
  checkTempAndAct();
}

void checkTempAndAct() {
  if (Temperature < lowerTempBound && !powerStatus) {
    Serial.println("Temperature is too low, powering up");
    sendPowerIR();
    powerStatus = true;
  } else if (Temperature > upperTempBound && powerStatus) {
    Serial.println("Temperature is too high, powering off");
    sendPowerIR();
    powerStatus = false;
  }
}

void setupMQTT() {
    mqttClient.setServer(mqtt_server, 1883);
    mqttClient.setCallback(mqttCallback);
}



void setupWebserver() {
    server.on("/", webserverOnConnect);
    server.onNotFound(webserverOnNotFound);
  
    server.begin();
    Serial.println("HTTP server started");
}


void mqttCallback(char* topic, byte* payload, unsigned int length) {
    char message[length];
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] ");
    for (int i = 0; i < length; i++) {
      message[i] = (char)payload[i];
      //Serial.print((char)payload[i]);
    }
    //String recv_payload = String((char*) payload, length);
    Serial.println(message);

    float newLower = 17.0;
    float newUpper = 23.3;

    switch((char)payload[0]) {
      case '1':
          redrawScreen();
          digitalWrite(BUILTIN_LED, LOW);   // Turn the LED on (Note that LOW is the voltage level
                                            // but actually the LED is on; this is because
                                            // it is active low on the ESP-01)
          break;
      case '2':
          sendPowerIR();
          powerStatus = !powerStatus;
          break;
      case '3':
          drawMessage(message);
          break;
      case '4':
          sscanf(message, "%*s %f", &newLower);
          Serial.print("New lower set to: ");
          Serial.println(newLower);
          lowerTempBound = newLower;
          drawMessage("New lower temperature set");
          break;
      case '5':
          sscanf(message, "%*s %f", &newLower);
          Serial.print("New upper set to: ");
          Serial.println(newUpper);
          upperTempBound = newUpper;
          drawMessage("New upper temperature set");
          break;
      default:
          digitalWrite(BUILTIN_LED, HIGH);  // Turn the LED off by making the voltage HIGH
    }
}

void mqttReconnect() {
  // Loop until we're reconnected
  while (!mqttClient.connected() && MQTT_retries < 3) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (mqttClient.connect(clientId.c_str())) {
      Serial.println("connected");
      mqttClient.subscribe(inboundTopic);
    } else {
      MQTT_retries++;
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" try again in 5 seconds");
      // Wait 500 mseconds before retrying
      delay(500);
    }
  }
  MQTT_retries = 0;
}

void drawMessage(const char *message) {
    u8g2.clearDisplay();
    // picture loop
    u8g2.firstPage();  
    do {
        u8g2.setFont(u8g2_font_6x10_tr);
        u8g2.drawStr(2,15,message);  
    } while( u8g2.nextPage() );
}

void sendPowerIR() {
  irsend.sendNEC(0x2FDA05F, 32, 5); //32 bits, 5 repeats
  delay(300);
  Serial.println("Power signal sent");
  sendEventMessage("Power signal sent");
  drawMessage("Power IR signal sent");
}

void sendEventMessage(const char *message) {
    mqttClient.publish(events_topic, String(message).c_str());
}

void redrawScreen() {
    Temperature = dht.readTemperature(); // Gets the values of the temperature
    Humidity = dht.readHumidity(); // Gets the values of the humidity 
    u8g2.clearDisplay();
    // picture loop
    u8g2.firstPage();  
    do {
        u8g2.setFont(u8g2_font_6x10_tr);
        u8g2.drawStr(2,15,"Arduino sensors");  
        u8g2.drawFrame(0,0,u8g2.getDisplayWidth(),u8g2.getDisplayHeight() );
        u8g2.drawStr(2, 30, "Temperature:");
        u8g2.drawStr(2, 45, "Humidity:");
        u8g2.setFont(u8g2_font_t0_11b_tf);
        u8g2.drawStr(80, 30, String(Temperature).c_str());
        u8g2.drawStr(80, 45, String(Humidity).c_str());
    } while( u8g2.nextPage() );
}

void webserverOnConnect() {
    Temperature = dht.readTemperature(); // Gets the values of the temperature
    Humidity = dht.readHumidity(); // Gets the values of the humidity 
    Serial.print("Temperature:");
    Serial.println(Temperature);
    server.send(200, "text/html", SendHTML(Temperature,Humidity));
    redrawScreen();
}

void webserverOnNotFound(){
    server.send(404, "text/plain", "Not found");
}

String SendHTML(float Temperaturestat,float Humiditystat){
    String ptr = "<!DOCTYPE html> <html>\n";
    ptr +="<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n";
    ptr +="<title>Temperatura ambiente</title>\n";
    ptr +="<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}\n";
    ptr +="body{margin-top: 50px;} h1 {color: #444444;margin: 50px auto 30px;}\n";
    ptr +="p {font-size: 24px;color: #444444;margin-bottom: 10px;}\n";
    ptr +="</style>\n";
    ptr +="</head>\n";
    ptr +="<body>\n";
    ptr +="<div id=\"webpage\">\n";
    ptr +="<h1>ESP8266 NodeMCU Temperatura ambiente</h1>\n";
    
    ptr +="<p>Temperatura: ";
    ptr +=(int)Temperaturestat;
    ptr +=" ºC</p>";
    ptr +="<p>Humedad: ";
    ptr +=(int)Humiditystat;
    ptr +="%</p>";
    
    ptr +="</div>\n";
    ptr +="</body>\n";
    ptr +="</html>\n";
    return ptr;
}
