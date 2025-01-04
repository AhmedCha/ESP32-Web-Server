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
String userRole = "";

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

  // Get the role of the current user
  String userRole = userRoles[clientIP];

  // Restrict access to settings for non-admin users
  if (userRole != "admin" && request->url().startsWith("/settings")) {
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
  html.replace("CURRENT_SSID", currentSSID);
  html.replace("CURRENT_USERNAME", currentUsername);
  request->send(200, "text/html", html);
}

// Update Settings Handler
void handleUpdateSettings(AsyncWebServerRequest *request) {
  bool isUpdated = false;

  if (!ensureLoggedIn(request)) return;

  if (WiFi.getMode() != WIFI_AP) {
    request->send(403, "text/plain", "Settings updates are not allowed in STA mode.");
    return;
  }

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
  currentAPSSID = preferences.getString("apssid", "ESP32_001");
  currentAPPassword = preferences.getString("ap_password", "men0lel1");
  currentUsername = preferences.getString("username", "admin");
  currentPassword = preferences.getString("password", "admin");
  preferences.end();

  // Configure Wi-Fi in Station + AP mode
  WiFi.mode(WIFI_AP_STA);

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

  // Start Access Point Mode
  WiFi.softAP(currentAPSSID.c_str(), currentAPPassword.c_str());  // Configure SSID and password
  Serial.print("AP IP Address: ");
  Serial.println(WiFi.softAPIP());

  // Start the server
  setupRoutes();
  server.begin();
  Serial.println("Server started");
}

#endif  // ROUTES_H
