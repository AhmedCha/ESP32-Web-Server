#ifndef DASHBOARD_H
#define DASHBOARD_H

#include <ESPAsyncWebServer.h>
#include <DHT.h>

// Pin definitions
#define LED1_PIN 26
#define LED2_PIN 27
#define DHT_PIN 4
#define DHT_TYPE DHT22

// State variables
bool led1State = false;
bool led2State = false;
extern AsyncWebServer server;

const char *ledOffSVG = "<svg class=\"svg-icon\" style=\"width: 50px; height: 50px; vertical-align: middle; fill: currentColor; overflow: hidden;\" viewBox=\"0 0 1024 1024\" version=\"1.1\" xmlns=\"http://www.w3.org/2000/svg\">"
                        "<path d=\"M512 256a170.666667 170.666667 0 0 0-170.666667 170.666667v256H256v85.333333h128v213.333333h85.333333v-213.333333h85.333334v213.333333h85.333333v-213.333333h128v-85.333333h-85.333333v-256a170.666667 170.666667 0 0 0-170.666667-170.666667z\" fill=\"\" />"
                        "</svg>";
const String ledOnSVG_Part1 = "<svg class=\"svg-icon\" style=\"width: 50px; height: 50px; vertical-align: middle; fill: currentColor; overflow: hidden;\" viewBox=\"0 0 1024 1024\" version=\"1.1\" xmlns=\"http://www.w3.org/2000/svg\">";
const String ledOnSVG_Part2 = "<path d=\"M469.333333 0v170.666667h85.333334V0h-85.333334m311.466667 97.706667l-130.56 128 59.733333 60.586666 130.56-128-59.733333-60.586666";
const String ledOnSVG_Part3 = "m-537.173333 0L183.04 158.293333l128 128 60.586667-60.586666-128-128M512 256a170.666667 170.666667 0 0 0-170.666667 170.666667v256H256v85.333333h128v213.333333h85.333333v-213.333333h85.333334v213.333333";
const String ledOnSVG_Part4 = "h85.333333v-213.333333h128v-85.333333h-85.333333v-256a170.666667 170.666667 0 0 0-170.666667-170.666667M85.333333 384v85.333333h170.666667V384H85.333333m682.666667 0v85.333333h170.666667V384h-170.666667z\" fill=\"\" />";
const String ledOnSVG_Part5 = "</svg>";
String ledOnSVG = ledOnSVG_Part1 + ledOnSVG_Part2 + ledOnSVG_Part3 + ledOnSVG_Part4 + ledOnSVG_Part5;

DHT dht(DHT_PIN, DHT_TYPE);


void toggleLED(int pin, bool &state) {
  state = !state;
  digitalWrite(pin, state ? HIGH : LOW);
}

void handleLED(AsyncWebServerRequest *request) {
  request->send_P(200, "text/html", INDEX_HTML);
}

// Helper function to escape double quotes inside SVG
String escapeSVG(String svg) {
  svg.replace("\"", "\\\"");  // Escape double quotes
  svg.replace("\n", "\\n");   // Escape new lines
  svg.replace("\r", "\\r");   // Escape carriage return
  return svg;
}

void handleLEDState(AsyncWebServerRequest *request) {
  String json = "{";
  json += "\"led1State\":";
  json += led1State ? "\"" + escapeSVG(ledOnSVG) + "\"" : "\"" + escapeSVG(String(ledOffSVG)) + "\"";
  json += ",";
  json += "\"led2State\":";
  json += led2State ? "\"" + escapeSVG(ledOnSVG) + "\"" : "\"" + escapeSVG(String(ledOffSVG)) + "\"";
  json += "}";

  request->send(200, "application/json", json);
}


// LED Toggle Handler
void handleToggleLED(AsyncWebServerRequest *request) {
  if (request->hasParam("led")) {
    String led = request->getParam("led")->value();
    if (led == "1") {
      toggleLED(LED1_PIN, led1State);
      request->send(200, "text/plain", led1State ? "true" : "false");
    } else if (led == "2") {
      toggleLED(LED2_PIN, led2State);
      request->send(200, "text/plain", led2State ? "true" : "false");
    }
  } else {
    request->send(400, "text/plain", "LED parameter missing");
  }
}

// Set LED Intensity Handler
void handleSetLEDIntensity(AsyncWebServerRequest *request) {
  if (!ensureLoggedIn(request)) return;

  if (request->hasParam("led") && request->hasParam("intensity")) {
    String led = request->getParam("led")->value();
    String intensity = request->getParam("intensity")->value();
    int intensityValue = intensity.toInt();

    if (intensityValue < 0 || intensityValue > 255) {
      request->send(400, "text/plain", "Invalid intensity value");
      return;
    }

    if (led == "1") {
      analogWrite(LED1_PIN, intensityValue);
    } else if (led == "2") {
      analogWrite(LED2_PIN, intensityValue);
    }
  }

  request->send(200, "text/plain", "LED intensity set");
}

// Sensor Data Route
void handleSensorData(AsyncWebServerRequest *request) {
  if (!ensureLoggedIn(request)) return;

  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();

  String temperatureStr = isnan(temperature) ? "Error" : "Temperature:" + String(temperature);
  String humidityStr = isnan(humidity) ? "Error" : "Humidity:" + String(humidity);

  String jsonResponse = "{\"temperature\": \"" + temperatureStr + " °C\", \"humidity\": \"" + humidityStr + " %\"}";

  request->send(200, "application/json", jsonResponse);
}

// Routes
void setupLEDRoutes() {
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!ensureLoggedInAndAuthorized(request, "")) return;
    handleLED(request);
  });

  server.on("/led-state", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!ensureLoggedInAndAuthorized(request, "")) return;
    handleLEDState(request);
  });

  server.on("/toggle", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!ensureLoggedInAndAuthorized(request, "")) return;
    handleToggleLED(request);
  });

  server.on("/set_led_intensity", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!ensureLoggedInAndAuthorized(request, "")) return;
    handleSetLEDIntensity(request);
  });
}

void setupSensorRoutes() {
  server.on("/sensor_data", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!ensureLoggedInAndAuthorized(request, "")) return;
    handleSensorData(request);
  });
}

#endif