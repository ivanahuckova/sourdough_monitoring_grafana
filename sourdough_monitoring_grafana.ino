#include <Arduino.h>
#include <WiFi.h>
#include <NTPClient.h>

#include <DHT.h>
#include <HCSR04.h>

#include "config.h"

// NTP Client
WiFiUDP ntpUDP;
NTPClient ntpClient(ntpUDP);

// DHT Sensor
DHT dht(DHTPIN, DHTTYPE);

// Ultrasonic Sensor
UltraSonicDistanceSensor distanceSensor(ULTRASONIC_PIN_TRIG, ULTRASONIC_PIN_ECHO);  

// Function to set up the connection to the WiFi
void setupWiFi() {
  Serial.print("Connecting to '");
  Serial.print(WIFI_SSID);
  Serial.print("' ...");

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}


// ========== MAIN FUNCTIONS: SETUP & LOOP ========== 
// SETUP: Function called at boot to initialize the system
void setup() {
  // Start the serial output at 115,200 baud
  Serial.begin(115200);

  // Connect to WiFi
  setupWiFi();

  // Initialize a NTPClient to get time
  ntpClient.begin();

  // Start the DHT sensor
  dht.begin();
}


// LOOP: Function called in a loop to read from sensors and send them do databases
void loop() {
  // Reconnect to WiFi if required
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.disconnect();
    yield();
    setupWiFi();
  }

  // Update time via NTP if required
  while(!ntpClient.update()) {
    yield();
    ntpClient.forceUpdate();
  }

  // Get current timestamp
  unsigned long ts = ntpClient.getEpochTime();

  // Read temperature, humidity and distance
  float hum = dht.readHumidity();
  float cels = dht.readTemperature();
  double dist = distanceSensor.measureDistanceCm();

  // Check if any reads failed and exit early (to try again).
  if (isnan(hum) || isnan(cels) || isnan(dist) ) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }

  // Compute heat index in Celsius (isFahreheit = false)
  float hic = dht.computeHeatIndex(cels, hum, false);

  Serial.println(ts);
  Serial.println(cels);
  Serial.println(hum);
  Serial.println(hic);
  Serial.println(dist);
  Serial.println("==============");

  // wait INTERVAL seeconds and then do it again
  delay(INTERVAL * 1000);
}

