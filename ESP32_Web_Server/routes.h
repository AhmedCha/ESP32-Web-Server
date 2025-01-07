#ifndef ROUTES_H
#define ROUTES_H

#include <ESPAsyncWebServer.h>
#include "html_pages.h"
#include <DHT.h>
#include <Preferences.h>
#include <map>
#include <IPAddress.h>
#include <ctime>

std::map<IPAddress, String> userRoles;
std::map<IPAddress, bool> loggedInUsers;
std::map<IPAddress, time_t> loginTimestamps;

// External variables
extern Preferences preferences;
extern AsyncWebServer server;
extern String currentSSID;
extern String currentWiFiPassword;
extern String currentAPSSID;
extern String currentAPPassword;
extern String currentUsername;
extern String currentPassword;
extern String wifiOptionsHTML;

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

// Helper Function: Check if the client is logged in
bool isSessionValid(IPAddress clientIP) {
  if (loginTimestamps.find(clientIP) != loginTimestamps.end()) {
    time_t currentTime = time(nullptr);
    time_t loginTime = loginTimestamps[clientIP];

    if (difftime(currentTime, loginTime) > 300) {  // Session expired
      loginTimestamps.erase(clientIP);
      userRoles.erase(clientIP);
      return false;
    }
    return true;
  }
  return false;  // No session found
}

bool ensureLoggedIn(AsyncWebServerRequest *request) {
  // Get the client's IP address
  IPAddress clientIP = request->client()->remoteIP();

  if (!loggedInUsers[clientIP]) {
    request->redirect("/login");
    return false;
  }

  // Validate the session
  if (!isSessionValid(clientIP)) {
    loggedInUsers[clientIP] = false;
    request->redirect("/login");
    return false;
  }

  // Restrict access to settings for non-admin users
  if (userRoles[clientIP] != "admin" && request->url().startsWith("/settings")) {
    request->redirect("/");  // Redirect non-admin users to the dashboard
    Serial.println("Redirected, not logged in!!");
    return false;
  }

  return true;
}

void toggleLED(int pin, bool &state) {
  state = !state;
  digitalWrite(pin, state ? HIGH : LOW);
}

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

// Route Handlers
// Dashboard Page Handler
void handleLED(AsyncWebServerRequest *request) {
  if (!ensureLoggedIn(request)) return;

  String html = FPSTR(INDEX_HTML);
  html.replace("LED1_STATE", led1State ? "ON" : "OFF");
  html.replace("LED2_STATE", led2State ? "ON" : "OFF");

  request->send(200, "text/html", html);
}

// Login Page Handler
void sendLoginHtml(AsyncWebServerRequest *request, const char *message = nullptr) {
  String html = FPSTR(LOGIN_HTML);
  html.replace("%MESSAGE%", message ? message : "");
  request->send(200, "text/html", html);
}

// Login Handler
void handleLogin(AsyncWebServerRequest *request) {
  IPAddress clientIP = request->client()->remoteIP();
  if (isSessionValid(clientIP)) {
    request->redirect(userRoles[clientIP] == "admin" ? "/settings" : "/");
    return;
  }

  if (request->method() == HTTP_POST) {
    String username = request->getParam("username", true)->value();
    String password = request->getParam("password", true)->value();

    if (username == currentUsername && password == currentPassword) {
      loggedInUsers[clientIP] = true;

      // Get the client's IP address
      IPAddress clientIP = request->client()->remoteIP();

      // Store the login timestamp
      loginTimestamps[clientIP] = time(nullptr);

      // Determine role based on client IP
      String role;
      if (clientIP[0] == 192 && clientIP[1] == 168 && clientIP[2] == 4) {
        role = "admin";  // AP user
        request->redirect("/settings");
      } else {
        role = "viewer";  // STA user
        request->redirect("/");
      }

      // Store the user's role in the map
      userRoles[clientIP] = role;

      return;
    }

    sendLoginHtml(request, "Invalid credentials. Please try again.");
  } else {
    sendLoginHtml(request);
  }
}


// Logout Handler
void handleLogout(AsyncWebServerRequest *request) {
  IPAddress clientIP = request->client()->remoteIP();

  // Remove the user's login state and role
  loggedInUsers.erase(clientIP);
  userRoles.erase(clientIP);

  request->redirect("/login");
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


void cleanupExpiredSessions() {
  time_t currentTime = time(nullptr);
  for (auto it = loginTimestamps.begin(); it != loginTimestamps.end();) {
    if (difftime(currentTime, it->second) > 300) {
      userRoles.erase(it->first);
      it = loginTimestamps.erase(it);
    } else {
      ++it;
    }
  }
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

bool ensureLoggedInAndAuthorized(AsyncWebServerRequest *request, String requiredRole) {
  IPAddress clientIP = request->client()->remoteIP();
  if (!isSessionValid(clientIP)) {
    request->redirect("/login");
    return false;
  }

  if (userRoles[clientIP] != requiredRole && !requiredRole.isEmpty()) {
    request->redirect("/");
    return false;
  }

  return true;
}


void setupLEDRoutes() {
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!ensureLoggedInAndAuthorized(request, "")) return;
    handleLED(request);
  });

  server.on("/toggle_led", HTTP_GET, [](AsyncWebServerRequest *request) {
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


// Main Setup Function for All Routes
void setupRoutes() {
  // Authentication Routes
  setupAuthRoutes();
  // LED and Dashboard Routes
  setupLEDRoutes();
  // Settings Routes (with access control)
  setupSettingsRoutes();
  // Sensor Data Routes
  setupSensorRoutes();
}

#endif  // ROUTES_H
