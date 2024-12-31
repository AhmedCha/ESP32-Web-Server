#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include "routes.h"  // Include routes file
#include <Preferences.h>

// Global Variables
Preferences preferences;
AsyncWebServer server(80);
String currentSSID = "bardo";
String currentWiFiPassword = "12345679";
String currentUsername = "admin";
String currentPassword = "password";
bool isLoggedIn = false;

void setup() {
  Serial.begin(115200);

  // Load preferences
  preferences.begin("settings", true);
  currentSSID = preferences.getString("ssid", "bardo");
  currentWiFiPassword = preferences.getString("wifi_password", "12345679");
  currentUsername = preferences.getString("username", "admin");
  currentPassword = preferences.getString("password", "password");
  preferences.end();

  // Initialize hardware components
  pinMode(LED1_PIN, OUTPUT);
  pinMode(LED2_PIN, OUTPUT);
  digitalWrite(LED1_PIN, LOW);
  digitalWrite(LED2_PIN, LOW);
  dht.begin();

  // Connect to Wi-Fi
  WiFi.begin(currentSSID.c_str(), currentWiFiPassword.c_str());
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < WIFI_ATTEMPTS) {
    delay(WIFI_DELAY_MS);
    Serial.print(".");
    attempts++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConnected to WiFi");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nFailed to connect to WiFi");
  }

  // Set up routes and start the server
  setupRoutes();
  server.begin();
  Serial.println("HTTP server started");

  // Log credentials
  Serial.println("Username: \n>\"", currentUsername, "\"");
  Serial.println("Password: \n>\"", currentPassword, "\"");
}

void loop() {
  // Async server handles requests; nothing is needed here
}
