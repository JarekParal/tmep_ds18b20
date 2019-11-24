/*
  Simple sketch for sending data to the TMEP.cz
  by mikrom (http://www.mikrom.cz)
  http://wiki.tmep.cz/doku.php?id=zarizeni:esp8266
 
  If you send only temperature url will be: http://ahoj.tmep.cz/?mojemereni=25.6 (domain is "ahoj" and guid is "mojemereni"
  If you send also humidity url will be: http://ahoj.tmep.cz/?mojemereni=25.6&humV=60
  But for humidity You will need DHT11 or DHT22 sensor and code will need some modifications, see the other sketch.
 
  Blinking with ESP internal LED according status:
  LED constantly blinking = connecting to WiFi
  LED on for 5 seconds = boot, setup, WiFi connect OK
  LED blink 2x per second = temp error (wrong pin, bad sensor, read -127.00°C)
  LED blink 3x per second = connect to host failed
  LED blink 4x per second = client connection timeout
  LED blink 1x per [sleepInMinutes] = send data ok
*/

#include <WiFi.h>
#include <OneWire.h>           // OneWire communication library for DS18B20
#include <DallasTemperature.h> // DS18B20 library
 
// Define settings
// const char ssid[]     = "---ssid---"; // WiFi SSID
// const char pass[]     = "---password---"; // WiFi password
// const char domain[]   = "---domain---";  // domain.tmep.cz
// const char guid[]     = "---guid---"; // mojemereni
#include "credential.h"

const byte sleepInMinutes = 1; // How often send data to the server. In minutes
const byte oneWireBusPin = 13; // Pin where is DS18B20 connected
 
// Create Temperature object "sensors"
OneWire oneWire(oneWireBusPin);         // Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
DallasTemperature sensors(&oneWire); // Pass our oneWire reference to Dallas Temperature.
 
const int LED_PIN = 12;
const int PIR_SENSOR_PIN = 34;
const int PIR_INDICATION_GND_PIN = 25;
const int PIR_INDICATION_VCC_PIN = 26;

void setup() {
  pinMode(LED_PIN, OUTPUT); // GPIO2, LED on ESP8266
  pinMode(PIR_SENSOR_PIN, INPUT);

  pinMode(PIR_INDICATION_GND_PIN, OUTPUT);
  pinMode(PIR_INDICATION_VCC_PIN, OUTPUT);
  digitalWrite(PIR_INDICATION_GND_PIN, LOW);

  // Start serial
  Serial.begin(115200);
  delay(10);
  Serial.println();
 
  // Connect to the WiFi
  Serial.print(F("Connecting to ")); Serial.println(ssid);
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(LED_PIN, LOW); delay(100); digitalWrite(LED_PIN, HIGH); // Blinking with LED during connecting to WiFi
    delay(500);
    //digitalWrite(2, HIGH); // Blinking with LED during connecting to WiFi
    Serial.print(F("."));
  }
  Serial.println();
  Serial.println(F("WiFi connected"));
  Serial.print(F("IP address: ")); Serial.println(WiFi.localIP());
  Serial.println();
 
  sensors.begin(); // Initialize the DallasTemperature DS18B20 class (not strictly necessary with the client class, but good practice).
 
  // LED on for 5 seconds after setup is done
  digitalWrite(LED_PIN, LOW);
  delay(5000);
  digitalWrite(LED_PIN, HIGH);
}
 
void loop() {
  sensors.requestTemperatures();        // Send the command to get temperatures. request to all devices on the bus
  float t = sensors.getTempCByIndex(0); // Read temperature in "t" variable
  if (t == -127.00) {                   // If you have connected it wrong, Dallas read this temperature! :)
    Serial.println(F("Temp error!"));
    // Blink 2 time when temp error
    digitalWrite(LED_PIN, LOW); delay(100); digitalWrite(LED_PIN, HIGH); delay(100); digitalWrite(LED_PIN, LOW); delay(100); digitalWrite(LED_PIN, HIGH);
    delay(1000);
    return;
  }
 
  // Connect to the HOST and send data via GET method
  WiFiClient client; // Use WiFiClient class to create TCP connections
 
  char host[50];            // Joining two chars is little bit difficult. Make new array, 50 bytes long
  strcpy(host, domain);     // Copy /domain/ in to the /host/
  strcat(host, ".tmep.cz"); // Add ".tmep.cz" at the end of the /host/. /host/ is now "/domain/.tmep.cz"
 
  Serial.print(F("Connecting to ")); Serial.println(host);
  if (!client.connect(host, 80)) {
    // If you didn't get a connection to the server
    Serial.println(F("Connection failed"));
    // Blink 3 times when host connection error
    digitalWrite(LED_PIN, LOW); delay(100); digitalWrite(LED_PIN, HIGH); delay(100); digitalWrite(LED_PIN, LOW); delay(100); digitalWrite(LED_PIN, HIGH); delay(100); digitalWrite(LED_PIN, LOW); delay(100); digitalWrite(LED_PIN, HIGH);
    delay(1000);
    return;
  }
  Serial.println(F("Client connected"));
 
  bool pirSensorDetect = digitalRead(PIR_SENSOR_PIN);
  int pirSensorValue = pirSensorDetect ? 80 : 20;
  digitalWrite(PIR_INDICATION_GND_PIN, pirSensorDetect);

  // Make an url. We need: /?guid=t
  String url = "/?";
         url += guid;
         url += "=";
         url += t;
         url += "&humV=";
         url += pirSensorValue;
  Serial.print(F("Requesting URL: ")); Serial.println(url);
 
  // Make a HTTP GETrequest.
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" + 
               "Connection: close\r\n\r\n");
 
  // Blik 1 time when send OK
  digitalWrite(LED_PIN, LOW); delay(100); digitalWrite(LED_PIN, HIGH);
 
  // Workaroud for timeout
  unsigned long timeout = millis();
  while (client.available() == 0) {
    if (millis() - timeout > 5000) {
      Serial.println(F(">>> Client Timeout !"));
      client.stop();
      // Blink 4 times when client timeout
      digitalWrite(LED_PIN, LOW); delay(100); digitalWrite(LED_PIN, HIGH); delay(100); digitalWrite(LED_PIN, LOW); delay(100); digitalWrite(LED_PIN, HIGH); digitalWrite(LED_PIN, LOW); delay(100); digitalWrite(LED_PIN, HIGH); delay(100); digitalWrite(LED_PIN, LOW); delay(100); digitalWrite(LED_PIN, HIGH);
      delay(1000);
      return;
    }
  }
 
  Serial.println();
 
  // Wait for another round
  delay(sleepInMinutes*60000);
}