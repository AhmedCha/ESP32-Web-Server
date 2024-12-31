#ifndef ROUTES_H
#define ROUTES_H

#include <ESPAsyncWebServer.h>
#include "html_pages.h"
#include <DHT.h>
#include <Preferences.h>

// External variables
extern Preferences preferences;
extern AsyncWebServer server;
extern String currentSSID;
extern String currentWiFiPassword;
extern String currentUsername;
extern String currentPassword;
extern bool isLoggedIn;

// Pin definitions
#define LED1_PIN 26
#define LED2_PIN 27
#define DHT_PIN 4
#define DHT_TYPE DHT22
#define WIFI_ATTEMPTS 10
#define WIFI_DELAY_MS 1000

// State variables
bool led1State = false;
bool led2State = false;

// DHT sensor
DHT dht(DHT_PIN, DHT_TYPE);

// Helper Functions
bool ensureLoggedIn(AsyncWebServerRequest *request) {
  if (!isLoggedIn) {
    request->redirect("/login");
    return false;
  }
  return true;
}

void toggleLED(int pin, bool &state) {
  state = !state;
  digitalWrite(pin, state ? HIGH : LOW);
}

// Route Handlers
// Dashboard Page Handler
void handleLED(AsyncWebServerRequest *request) {
  if (!ensureLoggedIn(request)) return;

  String html = FPSTR(INDEX_HTML);
  html.replace("LED1_STATE", led1State ? "ON" : "OFF");
  html.replace("LED2_STATE", led2State ? "ON" : "OFF");

  request->send(200, "text/html", html);
}

// LED Toggle Handler
void handleToggleLED(AsyncWebServerRequest *request) {
  if (!ensureLoggedIn(request)) return;

  if (request->hasParam("led")) {
    String led = request->getParam("led")->value();
    if (led == "1") {
      toggleLED(LED1_PIN, led1State);
    } else if (led == "2") {
      toggleLED(LED2_PIN, led2State);
    }
    request->redirect("/");
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

// Settings Page Handler
void handleSettings(AsyncWebServerRequest *request) {
  if (!ensureLoggedIn(request)) return;

  String html = FPSTR(SETTINGS_HTML);
  html.replace("CURRENT_SSID", currentSSID);
  html.replace("CURRENT_USERNAME", currentUsername);
  request->send(200, "text/html", html);
}

// Update Settings Handler
void handleUpdateSettings(AsyncWebServerRequest *request) {
  bool isUpdated = false;

  if (!ensureLoggedIn(request)) return;

  if (request->method() == HTTP_POST) {
    preferences.begin("settings", false);

    if (request->hasParam("ssid", true) && request->hasParam("wifi_password", true)) {
      currentSSID = request->getParam("ssid", true)->value();
      currentWiFiPassword = request->getParam("wifi_password", true)->value();

      if (currentSSID.length() != 0 && currentWiFiPassword.length() != 0) {
        Serial.print("Attempting to connect to SSID: ");
        Serial.println(currentSSID);

        WiFi.begin(currentSSID.c_str(), currentWiFiPassword.c_str());

        int attempts = 0;
        while (WiFi.status() != WL_CONNECTED && attempts < WIFI_ATTEMPTS) {
          delay(WIFI_DELAY_MS);
          Serial.print(".");
          attempts++;
        }

        if (WiFi.status() == WL_CONNECTED) {
          preferences.putString("ssid", currentSSID);
          preferences.putString("wifi_password", currentWiFiPassword);
          isUpdated = true;
          Serial.println("Connected to WiFi");
          Serial.print("IP address: ");
          Serial.println(WiFi.localIP());
        } else {
          Serial.println("Failed to connect to Wi-Fi.");
        }
      }
    }

    if (request->hasParam("username", true)) {
      String newUsername = request->getParam("username", true)->value();
      if (newUsername.length() != 0 && newUsername != currentUsername) {
        currentUsername = newUsername;
        preferences.putString("username", currentUsername);
        isUpdated = true;
        isLoggedIn = false;  // Force logout
        Serial.println("Username updated, user logged out.");
      }
    }

    if (request->hasParam("password", true)) {
      String newPassword = request->getParam("password", true)->value();
      if (newPassword.length() != 0 && newPassword != currentPassword) {
        currentPassword = newPassword;
        preferences.putString("password", currentPassword);
        isUpdated = true;
        isLoggedIn = false;  // Force logout
        Serial.println("Password updated, user logged out.");
      }
    }


    preferences.end();

    if (isUpdated) {
      request->send(200, "text/html", "<script>alert('Settings Updated Successfully!'); window.location.href='/';</script>");
    } else {
      request->send(200, "text/html", "<script>alert('No Changes Made or Connection Failed!'); window.location.href='/settings';</script>");
    }

  } else {
    request->send(405, "text/plain", "Method Not Allowed");
  }
}

// Login Page Handler
void sendLoginHtml(AsyncWebServerRequest *request, const char *message = nullptr) {
  String html = FPSTR(LOGIN_HTML);
  html.replace("%MESSAGE%", message ? message : "");
  request->send(200, "text/html", html);
}

// Login Handler
void handleLogin(AsyncWebServerRequest *request) {
  if (request->method() == HTTP_POST) {
    String username = request->getParam("username", true)->value();
    String password = request->getParam("password", true)->value();

    if (username == currentUsername && password == currentPassword) {
      isLoggedIn = true;
      request->redirect("/");
      return;
    }

    sendLoginHtml(request, "Invalid credentials. Please try again.");
  } else {
    sendLoginHtml(request);
  }
}

// Logout Handler
void handleLogout(AsyncWebServerRequest *request) {
  isLoggedIn = false;
  request->redirect("/login");
}

// Route Setup
void setupAuthRoutes() {
  server.on("/login", HTTP_GET, [](AsyncWebServerRequest *request) {
    sendLoginHtml(request);
  });
  server.on("/login", HTTP_POST, [](AsyncWebServerRequest *request) {
    handleLogin(request);
  });
  server.on("/logout", HTTP_GET, handleLogout);
}

void setupLEDRoutes() {
  server.on("/", HTTP_GET, handleLED);
  server.on("/toggle_led", HTTP_GET, handleToggleLED);
  server.on("/set_led_intensity", HTTP_GET, handleSetLEDIntensity);
}

void setupSensorRoutes() {
  server.on("/sensor_data", HTTP_GET, handleSensorData);
}

void setupSettingsRoutes() {
  server.on("/settings", HTTP_GET, handleSettings);
  server.on("/update_settings", HTTP_POST, handleUpdateSettings);
}

// Main Setup Function for All Routes
void setupRoutes() {
  setupAuthRoutes();
  setupLEDRoutes();
  setupSensorRoutes();
  setupSettingsRoutes();
}

// Initialization Function
void initializeSystem() {
  // Initialize LED pins
  pinMode(LED1_PIN, OUTPUT);
  pinMode(LED2_PIN, OUTPUT);

  // Initialize DHT sensor
  dht.begin();

  // Load stored preferences
  preferences.begin("settings", true);
  currentSSID = preferences.getString("ssid", "");
  currentWiFiPassword = preferences.getString("wifi_password", "");
  currentUsername = preferences.getString("username", "admin");
  currentPassword = preferences.getString("password", "admin");
  preferences.end();

  // Connect to Wi-Fi if credentials are available
  if (!currentSSID.isEmpty() && !currentWiFiPassword.isEmpty()) {
    Serial.print("Connecting to Wi-Fi: ");
    Serial.println(currentSSID);
    WiFi.begin(currentSSID.c_str(), currentWiFiPassword.c_str());

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < WIFI_ATTEMPTS) {
      delay(WIFI_DELAY_MS);
      Serial.print(".");
      attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\nWi-Fi connected");
      Serial.print("IP Address: ");
      Serial.println(WiFi.localIP());
    } else {
      Serial.println("\nFailed to connect to Wi-Fi");
    }
  } else {
    Serial.println("No Wi-Fi credentials stored.");
  }

  // Start the server
  setupRoutes();
  server.begin();
  Serial.println("Server started");
}

#endif  // ROUTES_H
