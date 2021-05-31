#include <Arduino.h>
#include <ArduinoBearSSL.h>
#include <PromLokiTransport.h>
#include <Prometheus.h>
#include <DHT.h>
#include <HCSR04.h>

#include "certificates.h"
#include "config.h"

// DHT Sensor
DHT dht(DHTPIN, DHTTYPE);

// Ultrasonic Sensor
UltraSonicDistanceSensor distanceSensor(ULTRASONIC_PIN_TRIG, ULTRASONIC_PIN_ECHO);  

// Prometheus client and transport
PromLokiTransport transport;
PromClient client(transport);

// Create a write request for 4 series
WriteRequest req(4, 2048);

// Define a TimeSeries which can hold up to 5 samples
TimeSeries ts1(5, "temperature_celsius", "monitoring_type=\"sourdough\",board_type=\"esp32_devkit1\",sourdough_type=\"rye\"");
TimeSeries ts2(5, "humidity_percent", "monitoring_type=\"sourdough\",board_type=\"esp32_devkit1\",sourdough_type=\"rye\"");
TimeSeries ts3(5, "heat_index_celsius", "monitoring_type=\"sourdough\",board_type=\"esp32_devkit1\",sourdough_type=\"rye\"");
TimeSeries ts4(5, "height_centimeter", "monitoring_type=\"sourdough\",board_type=\"esp32_devkit1\",sourdough_type=\"rye\"");

int loopCounter = 0;

// Function to set up Prometheus client
void setupClient() {
  Serial.println("Setting up client...");
  
  // Configure and start the transport layer
  transport.setUseTls(true);
  transport.setCerts(TAs, TAs_NUM);
  transport.setWifiSsid(WIFI_SSID);
  transport.setWifiPass(WIFI_PASSWORD);
  transport.setDebug(Serial);  // Remove this line to disable debug logging of the client.
  if (!transport.begin()) {
      Serial.println(transport.errmsg);
      while (true) {};
  }

  // Configure the client
  client.setUrl(GC_PROM_URL);
  client.setPath(GC_PROM_PATH);
  client.setPort(GC_PORT);
  client.setUser(GC_PROM_USER);
  client.setPass(GC_PROM_PASS);
  client.setDebug(Serial);  // Remove this line to disable debug logging of the client.
  if (!client.begin()) {
      Serial.println(client.errmsg);
      while (true) {};
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
  Serial.begin(115200);
  setupClient();
  dht.begin();
}

// LOOP: Function called in a loop to read from sensors and send them do databases
void loop() {
  int64_t time;
  time = transport.getTimeMillis();

  float hum = dht.readHumidity();
  float cels = dht.readTemperature();
  float height = getHeight();

  // Check if any reads failed and exit early (to try again).
  if (isnan(hum) || isnan(cels) || isnan(height) ) {
    Serial.println(F("Failed to read from sensor!"));
    return;
  }

  int hic = dht.computeHeatIndex(cels, hum, false);

  if (loopCounter >= 5) {
    //Send
    loopCounter = 0;
    PromClient::SendResult res = client.send(req);
    if (!res == PromClient::SendResult::SUCCESS) {
      Serial.println(client.errmsg);
    }
    // Reset batches after a succesful send.
    ts1.resetSamples();
    ts2.resetSamples();
    ts3.resetSamples();
    ts4.resetSamples();
  } else {
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
      Serial.println(ts4.errmsg);
    }
    loopCounter++;
  }

  // wait INTERVAL seconds and then do it again
  delay(INTERVAL * 1000);
}

