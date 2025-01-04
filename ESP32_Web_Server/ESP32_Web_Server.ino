#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include "routes.h"  // Include routes file
#include <Preferences.h>


// Global Variables
Preferences preferences;
AsyncWebServer server(80);
String currentSSID = "bardo";
String currentWiFiPassword = "12345679";
String currentAPSSID = "ESP32_001";
String currentAPPassword = "men0lel1";
String currentUsername = "admin";
String currentPassword = "password";

void setup() {
  Serial.begin(115200);

  // Load preferences
  preferences.begin("settings", true);
  currentSSID = preferences.getString("ssid", "bardo");
  currentWiFiPassword = preferences.getString("wifi_password", "12345679");
  currentAPSSID = preferences.getString("apssid", "ESP32_001");
  currentAPPassword = preferences.getString("ap_password", "men0lel1");
  currentUsername = preferences.getString("username", "admin");
  currentPassword = preferences.getString("password", "password");
  preferences.end();

  // Initialize hardware components
  pinMode(LED1_PIN, OUTPUT);
  pinMode(LED2_PIN, OUTPUT);
  digitalWrite(LED1_PIN, LOW);
  digitalWrite(LED2_PIN, LOW);
  dht.begin();

  WiFi.mode(WIFI_AP_STA);

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

  // Configure Access Point
  WiFi.softAP(currentAPSSID.c_str(), currentAPPassword.c_str());
  Serial.print("AP IP Address: ");
  Serial.println(WiFi.softAPIP());

  // Set up routes and start the server
  setupRoutes();
  server.begin();
  Serial.println("HTTP server started");

  // Log credentials
  Serial.print("Username: \n>");
  Serial.println(currentUsername);
  Serial.print("Password: \n>");
  Serial.println(currentPassword);
}

void loop() {
  cleanupExpiredSessions();
  delay(1000);
}
