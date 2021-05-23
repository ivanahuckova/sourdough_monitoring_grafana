#include <Arduino.h>
#include <Prometheus.h>
#include <bearssl_x509.h>
#include <DHT.h>
#include <HCSR04.h>

#include "certificates.h"
#include "config.h"

// DHT Sensor
DHT dht(DHTPIN, DHTTYPE);

// Ultrasonic Sensor
UltraSonicDistanceSensor distanceSensor(ULTRASONIC_PIN_TRIG, ULTRASONIC_PIN_ECHO);  

// Prometheus client
Prometheus client;
// Create a write request for 4 series
WriteRequest req(4, 1024);

// Create a labelset arrays for the 2 labels that is going to be used for all series
LabelSet label_set[] = {{ "monitoring_type", "sourdough" }, { "board_type", "esp32-devkit1" }};

// Define a TimeSeries which can hold up to 5 samples, has a name of `temperature/humidity/...` and uses the above labels of which there are 2
TimeSeries ts1(5, "temperature_celsius", label_set, 2);
TimeSeries ts2(5, "humidity_percent", label_set, 2);
TimeSeries ts3(5, "heat_index_celsius", label_set, 2);
TimeSeries ts4(5, "height_centimeter", label_set, 2);

int loopCounter = 0;

// Function to set up the connection to the WiFi
void setupClient() {
    Serial.println("Setting up client...");

    // Configure the client
    client.setUrl(GC_URL);
    client.setPath(GC_PATH);
    client.setPort(GC_PORT);
    client.setUser(GC_USER);
    client.setPass(GC_PASS);
    client.setUseTls(true);
    client.setCerts(TAs, TAs_NUM);
    client.setWifiSsid(WIFI_SSID);
    client.setWifiPass(WIFI_PASSWORD);
    client.setDebug(Serial);  // Remove this line to disable debug logging of the client.
    if (!client.begin()){
        Serial.println(client.errmsg);
    }

    // Add our TimeSeries to the WriteRequest
    req.addTimeSeries(ts1);
    req.addTimeSeries(ts2);
    req.addTimeSeries(ts3);
    req.addTimeSeries(ts4);
    req.setDebug(Serial);  // Remove this line to disable debug logging of the write request serialization and compression.
}

// Get height of starter
float getHeight() {
  float height_from_bottom = 16.50;
  float height = height_from_bottom - distanceSensor.measureDistanceCm();
  if (height > 0) {
    return height;
  };
  return 0;
}


// ========== MAIN FUNCTIONS: SETUP & LOOP ========== 
// SETUP: Function called at boot to initialize the system
void setup() {
  // Start the serial output at 115,200 baud
  Serial.begin(115200);

  // Set up client
  setupClient();

  // Start the DHT sensor
  dht.begin();
}


// LOOP: Function called in a loop to read from sensors and send them do databases
void loop() {
  int64_t time;
  time = client.getTimeMillis();

  // Read temperature, humidity and distance
  float hum = dht.readHumidity();
  float cels = dht.readTemperature();
  float height = getHeight();

  // Check if any reads failed and exit early (to try again).
  if (isnan(hum) || isnan(cels) || isnan(height) ) {
    Serial.println(F("Failed to read from sensor!"));
    return;
  }

  // Compute heat index in Celsius (isFahreheit = false)
  int hic = dht.computeHeatIndex(cels, hum, false);

  if (loopCounter >= 5) {
    //Send
    loopCounter = 0;
    if (!client.send(req)) {
      Serial.println(client.errmsg);
    }
    // Reset batches after a succesful send.
    ts1.resetSamples();
    ts2.resetSamples();
    ts3.resetSamples();
    ts4.resetSamples();
  }
  else {
    if (!ts1.addSample(time, cels)) {
      Serial.println(ts1.errmsg);
    }
    if (!ts2.addSample(time, hum)) {
      Serial.println(ts2.errmsg);
    }
    if (!ts3.addSample(time, hic)) {
      Serial.println(ts3.errmsg);
    }
    if (!ts4.addSample(time, height)) {
      Serial.println(ts2.errmsg);
    }
    loopCounter++;
  }

  // wait INTERVAL seconds and then do it again
  delay(INTERVAL * 1000);
}














