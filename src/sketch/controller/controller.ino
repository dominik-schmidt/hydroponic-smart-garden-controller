#include <ESP8266WiFi.h>
#include <time.h>

// Pin output values
#define ON 1
#define OFF 0

// Time library settings
#define MY_NTP_SERVER "at.pool.ntp.org"           
#define MY_TZ "CET-1CEST,M3.5.0/02,M10.5.0/03"

// Wifi settings
const char* ssid     = "";
const char* password = "";

// Pin setup
// D1
const int PIN_LED_RED = 5;
// D2
const int PIN_LED_BLUE = 4;
// D3
const int PIN_PUMP = 0;

// Determines light colors as well as on and off times (if mode is auto)
const int SCENE_NORMAL = 0;
const int SCENE_ENJOY = 1;
const int SCENE_GROW = 2;

// Determines light behavior
const int MODE_TIMER = 0;
const int MODE_ON = 1;
const int MODE_OFF = 2;

// Current scene and mode states
int scene = SCENE_NORMAL;
int mode = MODE_OFF;

// Current lights and pump states
int stateLights = OFF;
int statePump = OFF;

WiFiServer server(80);

time_t now;
time_t lastUpdate = 0;
tm tm;

void setup() {
  Serial.begin(115200);

  pinMode(PIN_LED_RED, OUTPUT);
  pinMode(PIN_LED_BLUE, OUTPUT);
  pinMode(PIN_PUMP, OUTPUT);

  digitalWrite(PIN_LED_RED, OFF);
  digitalWrite(PIN_LED_BLUE, OFF);
  digitalWrite(PIN_PUMP, OFF);

  configTime(MY_TZ, MY_NTP_SERVER);

  connectWifi();
}

void loop() {
  // Update states every 5 seconds
  time(&now);
  if (now - lastUpdate >= 5) {
    lastUpdate = now;
    updateStatus();
  }
 
  WiFiClient client = server.available();

  delay(1);  
  
  if (!client) {
    return;
  }
  log("Waiting for new client");

  while(!client.available()) {
    delay(1);
  }

  String request = client.readStringUntil('\r');
  client.flush();

  // Control logic
  if (request.indexOf("?scene=normal") != -1) {
    setScene(SCENE_NORMAL);
  } else if (request.indexOf("?scene=enjoy") != -1) {
    setScene(SCENE_ENJOY);
  } else if (request.indexOf("?scene=grow") != -1) {
    setScene(SCENE_GROW);
  } else if (request.indexOf("?mode=timer") != -1) {
    mode = MODE_TIMER;
  } else if (request.indexOf("?mode=on") != -1) {
    mode = MODE_ON;
  } else if (request.indexOf("?mode=off") != -1) {
    mode = MODE_OFF;
  }

  updateStatus();

  client.println("HTTP/1.1 200 OK"); //
  client.println("Content-Type: text/html");
  client.println("");

  htmlDoc(&client);
  
  delay(1);

  log("Client disonnected");
}

void connectWifi() {
  Serial.print("Connecting to WiFi");
  WiFi.persistent(false);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  server.begin();
}

/*
 * Outputs time stamp (without new line).
 */
void printTime() {
  time(&now);
  localtime_r(&now, &tm);

  Serial.print(tm.tm_year + 1900);
  Serial.print("-");
  Serial.print(tm.tm_mon + 1);
  Serial.print("-");
  Serial.print(tm.tm_mday);
  Serial.print(" ");
  Serial.print(tm.tm_hour);
  Serial.print(":");
  Serial.print(tm.tm_min);
  Serial.print(":");
  Serial.print(tm.tm_sec);
  Serial.print("\t");
}

/*
 * Logs message with current time.
 */
void log(char msg[]) {
  printTime();
  Serial.println(msg);
}

/*
 * Updates pump and light states based on current time
 * as well as scene and timer settings.
 */
void updateStatus() {
  time(&now);
  localtime_r(&now, &tm);

  int min = tm.tm_min;
  int hour = tm.tm_hour;

  updatePump(min);
  updateLights(scene, mode, hour);

  log("Status updated.");
}

/*
 * Turns on or off pump based on current time.
 */
void updatePump(int min) {
  if (min > 29) {
    turnOnPump();
  } else {
    turnOffPump();
  }
}

/*
 * Turns on or off lights based on current time 
 * as well as scene and timer settings.
 */
void updateLights(int scene, int timer, int hour) {
  switch (timer) {
    case MODE_ON:
      return turnOnLights(scene);
    case MODE_OFF:
      return turnOffLights(); 
    case MODE_TIMER:
      int startHour = 0;
      int stopHour = 23;
    
      switch (scene) {
        case SCENE_NORMAL:
          startHour = 7;
          stopHour = 21;
          break;
        case SCENE_ENJOY:
          startHour = 7;
          stopHour = 19;
          break;
        case SCENE_GROW:
          startHour = 6;
          stopHour = 22;
          break;
      }

      if (hour >= startHour && hour <= stopHour) {
        return turnOnLights(scene);
      } else {
        return turnOffLights();
      }
  }
}

/* 
 * Sets light scene. Briefly turns off lights to ensure
 * new scen is immediately applied.
 */
void setScene(int _scene) {
  scene = _scene;
  log("Set scene.");
}

/* 
 * Sets light mode.
 */
void setMode(int _mode) {
  mode = _mode;
  log("Set mode.");
}

/*
 * Turns on lights using provided scene settings.
 */
void turnOnLights(int scene) {
  switch (scene) {
    case SCENE_NORMAL:
      digitalWrite(PIN_LED_RED, ON);
      digitalWrite(PIN_LED_BLUE, ON);
      break;
    case SCENE_ENJOY:
      analogWrite(PIN_LED_RED, 128);
      digitalWrite(PIN_LED_BLUE, OFF);
      break;
    case SCENE_GROW:
      digitalWrite(PIN_LED_RED, ON);
      digitalWrite(PIN_LED_BLUE, OFF);
      break;
    default:
      digitalWrite(PIN_LED_RED, ON);
      digitalWrite(PIN_LED_BLUE, ON);
  }

  stateLights = ON;
}

/*
 * Turns off lights.
 */
void turnOffLights() {
  digitalWrite(PIN_LED_RED, OFF);
  digitalWrite(PIN_LED_BLUE, OFF);

  stateLights = OFF;
}

/*
 * Turns on pump.
 */
void turnOnPump() {
  digitalWrite(PIN_PUMP, ON);

  statePump = ON;
}


/*
 * Turns off pump.
 */
void turnOffPump() {
  digitalWrite(PIN_PUMP, OFF);

  statePump = OFF;
}

/*
 *  Outputs html button with correct state.
 */
void htmlButton(WiFiClient* client, char href[], char label[]) {
  bool isActive = false;

  if (href == "?scene=normal" && scene == SCENE_NORMAL) {
    isActive = true;
  } else if (href == "?scene=enjoy" && scene == SCENE_ENJOY) {
    isActive = true;
  } else if (href == "?scene=grow" && scene == SCENE_GROW) {
    isActive = true;
  } else if (href == "?mode=timer" && mode == MODE_TIMER) {
    isActive = true;
  } else if (href == "?mode=on" && mode == MODE_ON) {
    isActive = true;
  } else if (href == "?mode=off" && mode == MODE_OFF) {
    isActive = true;
  } else {
    isActive = false;
  }
  
  client->print("<a class='btn' href='");
  client->print(href);
  client->print("'");

  if (isActive) {
    client->print(" data-is-active");
  }

  client->print(">");
  client->print(label);
  client->println("</a>");
}

/*
 * Outputs html light element (lamp shade).
 */
void htmlLight(WiFiClient* client) {
  if (stateLights == ON) {
    client->println("<div id='light' data-is-active></div>");
  } else {
    client->println("<div id='light'></div>");
  }
}

/*
 * Outputs html pump element.
 */
void htmlPump(WiFiClient* client) {
  if (statePump == ON) {
    client->println("<div id='container' data-is-active>");
  } else {
    client->println("<div id='container'>");
  }
}

/*
 * Outputs html doc.
 */
 // TODO: Copy from output/html-doc.c
