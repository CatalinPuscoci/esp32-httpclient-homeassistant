#include <WiFi.h>
#include <WireGuard-ESP32.h>
#include <WiFiUdp.h>
#include <Arduino.h>
#include <ArduinoJson.h>

// WireGuard configuration --- UPDATE this configuration from JSON
char private_key[] ="";  // [Interface] PrivateKey
IPAddress local_ip(192,168,69,13);            // [Interface] Address
char public_key[] = "";     // [Peer] PublicKey
char endpoint_address[] = "";    // [Peer] Endpoint
int endpoint_port = 42069;              // [Peer] Endpoint

static constexpr const uint32_t UPDATE_INTERVAL_MS = 2000;
IPAddress target(192,168,69,16); 
static WireGuard wg;


/*
 Basic ESP8266 MQTT example
 This sketch demonstrates the capabilities of the pubsub library in combination
 with the ESP8266 board/library.
 It connects to an MQTT server then:
  - publishes "hello world" to the topic "outTopic" every two seconds
  - subscribes to the topic "inTopic", printing out any messages
    it receives. NB - it assumes the received payloads are strings not binary
  - If the first character of the topic "inTopic" is an 1, switch ON the ESP Led,
    else switch it off
 It will reconnect to the server if the connection is lost using a blocking
 reconnect function. See the 'mqtt_reconnect_nonblocking' example for how to
 achieve the same result without blocking the main loop.
 To install the ESP8266 board, (using Arduino 1.6.4+):
  - Add the following 3rd party board manager under "File -> Preferences -> Additional Boards Manager URLs":
       http://arduino.esp8266.com/stable/package_esp8266com_index.json
  - Open the "Tools -> Board -> Board Manager" and click install for the ESP8266"
  - Select your ESP8266 in "Tools -> Board"
*/

#include <WiFi.h>
#include <PubSubClient.h>
#include <HTTPClient.h>
// Update these with values suitable for your network.

const char* ssid = "";
const char* password = "";
String remote_server = "192.168.69.16";
const char* tokencatutu = "Bearer ";
const char* token = "Bearer ";
WiFiClient espClient;
WiFiServer server(80);
HTTPClient httpClient;
//PubSubClient client(espClient);
// Current time
unsigned long currentTime = millis();
// Previous time
unsigned long previousTime = 0; 
// Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 2000;
// Variable to store the HTTP request
String header;
// Auxiliar variables to store the current output state
String lampState = "off";

unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE	(50)
char msg[MSG_BUFFER_SIZE];
int value = 0;

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  server.begin();
}


void setup() {
  pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  Serial.begin(115200);
  setup_wifi();
  Serial.println("Connected. Initializing WireGuard...");
     wg.begin(
         local_ip,
         private_key,
         endpoint_address,
         public_key,
         endpoint_port);
         delay(1000);
  //  get lamp state
      String getPath = "http://" + remote_server +":8123" + "/api/states/switch.iancu";
      httpClient.begin(getPath.c_str());
      httpClient.addHeader("Content-Type", "application/json");
      httpClient.addHeader("Authorization", token);
      int httpResponseCode = httpClient.GET();
      if (httpResponseCode>0) {
        Serial.print("HTTP Response code: ");
        Serial.println(httpResponseCode);
        String payload = httpClient.getString();
        DynamicJsonDocument doc(1024);
        deserializeJson(doc,payload);
        JsonObject obj = doc.as<JsonObject>();
        String state = obj["state"];

        Serial.println(payload);
        Serial.print("state is ");
        Serial.println( state.c_str());
        lampState = state;
      }
      else {
        Serial.print("Error code: ");
        Serial.println(httpResponseCode);
      }
      // Free resources
      httpClient.end();

}

void loop() {

  

  WiFiClient wifi_client = server.available();   // Listen for incoming clients

  if (wifi_client) {                             // If a new client connects,
    currentTime = millis();
    previousTime = currentTime;
    Serial.println("New Client.");          // print a message out in the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (wifi_client.connected() && currentTime - previousTime <= timeoutTime) {  // loop while the client's connected
      currentTime = millis();
   
      if (wifi_client.available()) {             // if there's bytes to read from the client,
        char c = wifi_client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        header += c;
        if (c == '\n') {                    // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            wifi_client.println("HTTP/1.1 200 OK");
            wifi_client.println("Content-type:text/html");
            wifi_client.println("Connection: close");
            wifi_client.println();
            
            // turns the GPIOs on and off
            if (header.indexOf("GET /on") >= 0) {
              //turn lamp on
                String postPath = "http://" + remote_server +":8123" + "/api/services/switch/turn_on";
              httpClient.begin(postPath.c_str());
              httpClient.addHeader("Content-Type", "application/json");
              httpClient.addHeader("Authorization", token);
              DynamicJsonDocument doc(1024);
              doc["entity_id"] = "switch.iancu";
              String payload;
              serializeJson(doc,payload);
              int httpResponseCode = httpClient.POST(payload);
              if (httpResponseCode>0) {
                Serial.print("HTTP Response code: ");
                Serial.println(httpResponseCode);
                String payload = httpClient.getString();
                
                deserializeJson(doc,payload);
                JsonObject obj = doc.as<JsonObject>();
                String state = obj["state"] == NULL?"off":"on";
                Serial.println(payload);
                Serial.print("state is now ");
                Serial.println( state.c_str());
                lampState = state;
      }
      else {
        Serial.print("Error code: ");
        Serial.println(httpResponseCode);
      }
      // Free resources
      httpClient.end();
            } else if (header.indexOf("GET /off") >= 0) {
                  //turn lamp on
                String postPath = "http://" + remote_server +":8123" + "/api/services/switch/turn_off";
              httpClient.begin(postPath.c_str());
              httpClient.addHeader("Content-Type", "application/json");
              httpClient.addHeader("Authorization", token);
              DynamicJsonDocument doc(1024);
              doc["entity_id"] = "switch.iancu";
              String payload;
              serializeJson(doc,payload);
              int httpResponseCode = httpClient.POST(payload);
              if (httpResponseCode>0) {
                Serial.print("HTTP Response code: ");
                Serial.println(httpResponseCode);
                String payload = httpClient.getString();
                
                deserializeJson(doc,payload);
                JsonObject obj = doc.as<JsonObject>();
                String state = obj["state"] == NULL?"on":"off";
                Serial.println(payload);
                Serial.print("state is now ");
                Serial.println(state.c_str());
                lampState = state;
      }
      else {
        Serial.print("Error code: ");
        Serial.println(httpResponseCode);
      }
      // Free resources
      httpClient.end();
            }
            //  get lamp state
      String getPath = "http://" + remote_server +":8123" + "/api/states/switch.iancu";
      httpClient.begin(getPath.c_str());
      httpClient.addHeader("Content-Type", "application/json");
      httpClient.addHeader("Authorization", token);
      int httpResponseCode = httpClient.GET();
      if (httpResponseCode>0) {
        Serial.print("HTTP Response code: ");
        Serial.println(httpResponseCode);
        String payload = httpClient.getString();
        DynamicJsonDocument doc(1024);
        deserializeJson(doc,payload);
        JsonObject obj = doc.as<JsonObject>();
        String state = obj["state"];

        Serial.println(payload);
        Serial.print("state is ");
        Serial.println( state.c_str());
        lampState = state;
      }
      else {
        Serial.print("Error code: ");
        Serial.println(httpResponseCode);
      }
      //  get temperature state
      float temp;
      getPath = "http://" + remote_server +":8123" + "/api/states/sensor.pvvx_temperature";
      httpClient.begin(getPath.c_str());
      httpClient.addHeader("Content-Type", "application/json");
      httpClient.addHeader("Authorization", token);
      httpResponseCode = httpClient.GET();
      if (httpResponseCode>0) {
        Serial.print("HTTP Response code: ");
        Serial.println(httpResponseCode);
        String payload = httpClient.getString();
        DynamicJsonDocument doc(1024);
        deserializeJson(doc,payload);
        JsonObject obj = doc.as<JsonObject>();
        float state = obj["state"];
        temp = state;
        Serial.println(payload);
        Serial.print("state is ");
        Serial.println(state);
      }
      else {
        Serial.print("Error code: ");
        Serial.println(httpResponseCode);
      }
      //  get temperature state
      float hum;
      getPath = "http://" + remote_server +":8123" + "/api/states/sensor.pvvx_humidity";
      httpClient.begin(getPath.c_str());
      httpClient.addHeader("Content-Type", "application/json");
      httpClient.addHeader("Authorization", token);
      httpResponseCode = httpClient.GET();
      if (httpResponseCode>0) {
        Serial.print("HTTP Response code: ");
        Serial.println(httpResponseCode);
        String payload = httpClient.getString();
        DynamicJsonDocument doc(1024);
        deserializeJson(doc,payload);
        JsonObject obj = doc.as<JsonObject>();
        float state = obj["state"];
        hum = state;
        Serial.println(payload);
        Serial.print("state is ");
        Serial.println(state);
      }
      else {
        Serial.print("Error code: ");
        Serial.println(httpResponseCode);
      }
      
      // Free resources
      httpClient.end();
            // Display the HTML web page
            wifi_client.println("<!DOCTYPE html><html>");
            wifi_client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            wifi_client.println("<link rel=\"icon\" href=\"data:,\">");
            // CSS to style the on/off buttons 
            // Feel free to change the background-color and font-size attributes to fit your preferences
            wifi_client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
            wifi_client.println(".button { background-color: #4CAF50; border: none; color: white; padding: 16px 40px;");
            wifi_client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
            wifi_client.println(".button2 {background-color: #555555;}</style></head>");
            
            // Web Page Heading
            wifi_client.println("<body><h1>ESP32 Web Server</h1>");
            
            // Display current state, and ON/OFF buttons for GPIO 26  
            wifi_client.println("<p>Lamp - State " + lampState + "</p>");

            // Display the buttons
            wifi_client.println("<p><a href=\"/on\"><button class=\"button\">ON</button></a></p>");
            wifi_client.println("<p><a href=\"/off\"><button class=\"button button2\">OFF</button></a></p>");
            wifi_client.println("<p>Temperature: " + String(temp) + "C"+"</p>");
            wifi_client.println("<p>Humidity: " + String(hum) + "%</p>");
            // The HTTP response ends with another blank line
            wifi_client.println();
            // Break out of the while loop
            break;
          } else { // if you got a newline, then clear currentLine
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }
    // Clear the header variable
    header = "";
    // Close the connection
    wifi_client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }
}