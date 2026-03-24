#ifndef AUTH_H
#define AUTH_H

#include <ESPAsyncWebServer.h>
#include <IPAddress.h>
#include <ctime>
#include <map>
#include <Preferences.h>
#include "html_pages.h"

std::map<IPAddress, String> userRoles;
std::map<IPAddress, bool> loggedInUsers;
std::map<IPAddress, time_t> loginTimestamps;

extern String currentUsername;
extern String currentPassword;
extern AsyncWebServer server;


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


#endif