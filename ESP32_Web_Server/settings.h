#ifndef SETTINGS_H
#define SETTINGS_H

#include <ESPAsyncWebServer.h>
#include <Preferences.h>
#include <WiFi.h>
#include <ctime>

extern String currentSSID;
extern String currentWiFiPassword;
extern String currentAPSSID;
extern String currentAPPassword;
extern String wifiOptionsHTML;
extern String currentUsername;
extern String currentPassword;
extern AsyncWebServer server;
extern Preferences preferences;

#define WIFI_ATTEMPTS 10
#define WIFI_DELAY_MS 1000

// Scan available wifi SSID
void scanForWiFiNetworks() {
  String options = "";
  int n = WiFi.scanNetworks();  // Perform Wi-Fi scan
  if (n == 0) {
    options = "<option value='Null'>No Networks Found</option>";
  } else {
    for (int i = 0; i < n; i++) {
      options += "<option value='" + WiFi.SSID(i) + "'>";
      options += WiFi.SSID(i) + " (" + String(WiFi.RSSI(i)) + "dB)";
      options += "</option>";
    }
  }
  wifiOptionsHTML = options;
  WiFi.scanDelete();
}

// Settings Page Handler
void handleSettings(AsyncWebServerRequest *request) {
  if (!ensureLoggedIn(request)) return;

  String html = FPSTR(SETTINGS_HTML);
  html.replace("%WIFI_OPTIONS%", wifiOptionsHTML);
  html.replace("CURRENT_SSID", currentSSID);
  html.replace("CURRENT_USERNAME", currentUsername);
  request->send(200, "text/html", html);
}

// Update Settings Handler
void handleUpdateSettings(AsyncWebServerRequest *request) {
  bool isUpdated = false;
  IPAddress clientIP = request->client()->remoteIP();

  if (!ensureLoggedIn(request)) return;
  if (userRoles[clientIP] != "admin") return;

  if (request->method() == HTTP_POST) {
    preferences.begin("settings", false);

    // Wifi SSID and password
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

    // AP SSID and password
    if (request->hasParam("apssid", true) && request->hasParam("ap_password", true)) {
      String newAPSSID = request->getParam("apssid", true)->value();
      String newAPPassword = request->getParam("ap_password", true)->value();

      if (newAPSSID.length() > 0 && newAPPassword.length() >= 8) {  // Password length should be at least 8 characters
        preferences.putString("apssid", newAPSSID);
        preferences.putString("ap_password", newAPPassword);
        isUpdated = true;

        Serial.println("Updating Access Point settings...");

        WiFi.softAPdisconnect(true);  // Disconnect any existing AP
        bool apStarted = WiFi.softAP(newAPSSID.c_str(), newAPPassword.c_str());

        if (apStarted) {
          Serial.print("New AP SSID: ");
          Serial.println(newAPSSID);
          Serial.print("AP Password: ");
          Serial.println(newAPPassword);
        } else {
          Serial.println("Failed to start Access Point.");
        }
      } else {
        Serial.println("Invalid AP SSID or Password.");
      }
    }

    // Username and password
    if (request->hasParam("username", true)) {
      String newUsername = request->getParam("username", true)->value();
      if (newUsername.length() != 0 && newUsername != currentUsername) {
        currentUsername = newUsername;
        preferences.putString("username", currentUsername);
        isUpdated = true;
        handleLogout(request);
        Serial.println("Username updated, user logged out.");
      }
    }

    if (request->hasParam("password", true)) {
      String newPassword = request->getParam("password", true)->value();
      if (newPassword.length() != 0 && newPassword != currentPassword) {
        currentPassword = newPassword;
        preferences.putString("password", currentPassword);
        isUpdated = true;
        handleLogout(request);
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

void setupSettingsRoutes() {
  server.on("/settings", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!ensureLoggedInAndAuthorized(request, "admin")) return;
    handleSettings(request);  // Call the actual settings handler
  });

  server.on("/update_settings", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (!ensureLoggedInAndAuthorized(request, "admin")) return;
    handleUpdateSettings(request);  // Handle settings update
  });
}

#endif