#ifndef ROUTES_H
#define ROUTES_H

#include <ESPAsyncWebServer.h>
#include <Preferences.h>
#include "auth.h"
#include "dashboard.h"
#include "settings.h"

// External variables
extern Preferences preferences;
extern AsyncWebServer server;


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