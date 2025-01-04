#ifndef HTML_PAGES_H
#define HTML_PAGES_H

////////////////////////////////// INDEX PAGE / DASHBOARD //////////////////////////////////
const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">

<head>
  <title>ESP32 Dashboard</title>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body {
      font-family: Arial, sans-serif;
      background: rgb(51, 51, 51);
      padding: 0;
      margin: 0;
      text-align: center;
    }

    .nav {
      background: #ff8801;
      color: white;
      width: 90%;
      max-width: 720px;
      padding: 10px 20px;
      margin: 0px auto;
      display: flex;
      justify-content: space-between;
      align-items: center;
      box-shadow: 0 2px 4px rgba(0, 0, 0, 0.2);
      font-size: 1.2em;
      font-weight: bold;
    }

    .nav a {
      color: white;
      text-decoration: none;
      margin: 0 10px;
    }

    .nav a:hover {
      text-decoration: underline;
    }

    h1 {
      color: white;
    }

    .container {
      display: flex;
      flex-wrap: wrap;
      justify-content: center;
      gap: 20px;
    }

    .card {
      background: white;
      padding: 20px;
      border-radius: 8px;
      box-shadow: 0 4px 8px rgba(0, 0, 0, 0.2);
      text-align: center;
      width: 220px;
    }

    .btn {
      background-color: #ff8801;
      color: white;
      padding: 10px;
      border: none;
      border-radius: 5px;
      cursor: pointer;
      font-size: 1em;
    }

    .btn:hover {
      background-color: #ffa43d;
    }

    .sld {
      appearance: none;
      width: 100%;
      height: 8px;
      background: #ff8801;
      border-radius: 4px;
    }

    header {
      display: flex;
      justify-content: space-around;
      margin-bottom: 20px;
    }
  </style>

</head>

<body>
  <div class="nav">
    <span>ESP32 Dashboard</span>
    <div>
      <a href="/">Dashboard</a>
      <a href="/settings">Settings</a>
      <a href="/logout">Logout</a>
    </div>
  </div>
  <h1>Controle de LEDs</h1>
  <div class="container">
    <div class="card">
      <p>LED 1</p>
      <input class="sld" type="range" id="led1-slider" min="0" max="255" value="0"
        oninput="updateLEDIntensity(1, this.value)">
      <p>Intensity: <span id="led1-intensity">0</span></p>
    </div>
    <div class="card">
      <p>LED 2</p>
      <button class="btn" onclick="toggleLED(2)">Toggle</button>
    </div>
  </div>
  <hr style="margin-top: 30px;">
  <h1>Capteurs</h1>
  <div class="container">
    <div class="card">
      <h2>DHT22</h2>
      <p id="temperature">Temperature:</p>
      <p id="humidity">Humidity:</p>
    </div>
  </div>
  <script>
    function toggleLED(led) {
      fetch(`/toggle_led?led=${led}`);
    }

    function updateSensorData() {
      fetch("/sensor_data")
        .then(response => response.json())
        .then(data => {
          document.getElementById("temperature").innerText = data.temperature;
          document.getElementById("humidity").innerText = data.humidity;
        });
    }
    setInterval(updateSensorData, 1000);  // Update every 1 seconds

    function updateLEDIntensity(led, intensity) {
      fetch(`/set_led_intensity?led=${led}&intensity=${intensity}`)
        .then(response => response.text())
        .then(data => {
          document.getElementById(`led${led}-intensity`).innerText = intensity;
        });
    }
  </script>
</body>

</html>
)rawliteral";


////////////////////////////////// LOGIN PAGE //////////////////////////////////
const char LOGIN_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>

<head>
  <title>ESP32 Login</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body {
      font-family: Arial, sans-serif;
      background: rgb(51, 51, 51);
      display: flex;
      justify-content: center;
      align-items: center;
      height: 100vh;
      margin: 0;
    }

    .container {
      background: white;
      padding: 20px;
      border-radius: 8px;
      box-shadow: 0 4px 8px rgba(0, 0, 0, 0.2);
      text-align: center;
      width: 300px;
    }

    h1 {
      color: #ff8800;
    }

    input {
      width: 80%;
      padding: 10px;
      margin: 10px 0;
      border: 0px;
      border-bottom: 1px solid;
      font-size: 1em;
    }

    input:focus {
      outline: none;
    }

    input:focus::placeholder {
      color: transparent;
    }

    button {
      background-color: #ff8801;
      color: white;
      padding: 10px;
      width: 100%;
      border: none;
      border-radius: 5px;
      cursor: pointer;
      font-size: 1em;
    }

    button:hover {
      background-color: #ffa43d;
    }

    .message {
      color: red;
      font-size: 0.9em;
    }
  </style>
</head>

<body>
  <div class="container">
    <h1>ESP32 Login</h1>
    <form method="POST" action="/login">
      <input type="text" name="username" placeholder="Username" required>
      <input type="password" name="password" placeholder="Password" required>
      <button type="submit">Login</button>
    </form>
    <p class="message">%MESSAGE%</p>
  </div>
</body>

</html>
)rawliteral";


////////////////////////////////// SETTINGS PAGE //////////////////////////////////
const char SETTINGS_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">

<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>ESP32 Settings</title>
  <style>
    body {
      font-family: Arial, sans-serif;
      background: rgb(51, 51, 51);
      padding: 0;
      margin: 0;
      display: flex;
      flex-direction: column;
      align-items: center;
      height: 100vh;
    }

    .nav {
      background: #ff8801;
      color: white;
      width: 90%;
      max-width: 720px;
      padding: 10px 20px;
      margin: 0;
      display: flex;
      justify-content: space-between;
      align-items: center;
      box-shadow: 0 2px 4px rgba(0, 0, 0, 0.2);
      font-size: 1.2em;
      font-weight: bold;
    }

    .nav a {
      color: white;
      text-decoration: none;
      margin: 0 10px;
    }

    .nav a:hover {
      text-decoration: underline;
    }

    .container {
      background: white;
      margin-top: 20px;
      padding: 20px;
      border-radius: 10px;
      max-width: 400px;
      width: 100%;
      box-shadow: 0 4px 8px rgba(0, 0, 0, 0.2);
      text-align: center;
    }

    h1 {
      color: #ff8801;
      text-align: center;
    }

    label {
      display: block;
      margin-bottom: 5px;
      font-weight: bold;
    }

    input {
      width: 80%;
      margin-bottom: 15px;
      padding: 10px;
      border: 0px;
      border-bottom: 1px solid;
      font-size: 1em;
    }

    input:focus {
      outline: none;
    }

    input:focus::placeholder {
      color: transparent;
    }

    button {
      background: #ff8801;
      color: white;
      padding: 10px;
      border: none;
      border-radius: 5px;
      cursor: pointer;
      width: 100%;
      font-size: 1em;
    }

    button:hover {
      background-color: #ffa43d;
    }
  </style>
</head>

<body>
  <div class="nav">
    <span>ESP32 Settings</span>
    <div>
      <a href="/">Dashboard</a>
      <a href="/settings">Settings</a>
      <a href="/logout">Logout</a>
    </div>
  </div>
  <div class="container">
    <h1>Change Settings</h1>
    <form action="/update_settings" method="POST">
      <label for="username">Username:</label>
      <input type="text" id="username" name="username" placeholder="Username">

      <label for="password">Password:</label>
      <input type="password" id="password" name="password" placeholder="Password">

      <label for="ssid">WiFi SSID:</label>
      <input type="text" id="ssid" name="ssid" placeholder="SSID">

      <label for="wifi_password">WiFi Password:</label>
      <input type="password" id="wifi_password" name="wifi_password" placeholder="Password">

      <button type="submit">Update</button>
    </form>
  </div>
</body>

</html>
)rawliteral";

#endif  // HTML_PAGES_H