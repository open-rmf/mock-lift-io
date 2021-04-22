#include <Arduino.h>

// Wifi and OTA
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>

// LCD driver
#include <U8g2lib.h>

AsyncWebServer server(80);
U8G2_ST7920_128X64_F_SW_SPI u8g2(U8G2_R0, /* clock=*/ 18, /* data=*/ 23, /* cs=*/ 2, /* reset=*/ 4);

// Screen handler
void init_screen(void) {
  u8g2.begin();
  u8g2.setFont(u8g_font_8x13);
  u8g2.setColorIndex(1);                   // Instructs the display to draw with a pixel on.

  // Populate basic text
  u8g2.drawStr(5, 5,   "FLOOR:");
  u8g2.drawStr(50, 5,  "DIRECTION:");
  u8g2.drawStr(5, 25,  "DOOR:");
  u8g2.drawStr(5, 45,  "STATE:");
}
 
void setup(void) {
  // Set serial baud to 115200bps
  Serial.begin(115200);

  // Get our mac id and use it to generate a nice SSID
  String ssid_prefix = "Mock-Lift-";
  String ssid_string = ssid_prefix + WiFi.macAddress();
  const char* ssid = ssid_string.c_str();
  const char* password = "open-rmf";

  // Start in AP mode
  Serial.print("Setting AP");
  WiFi.softAP(ssid, password);

  // Print our station IP to serial monitor (for reference)
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);

  // Default landing page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "Hi! I am Mock Lift controller.");
  });

  // Start ElegantOTA
  AsyncElegantOTA.begin(&server, "user", "password");    
  server.begin();
  Serial.println("HTTP server started");

  // Initialize screen
  init_screen();
}

enum Motion_Directions {
  STATIONARY = 0,
  UP = 1,
  DOWN = 2
};

enum Door_States {
  CLOSE = 0,
  OPENING = 1,
  OPEN = 2,
  CLOSING = 3
};

enum Service_States {
  NORMAL = 0,
  AGV = 1,
  FIRE = 2,
  OUT_OF_SERVICE = 3
};

const uint8_t PIN_FLOOR[] = {0, 1, 2, 3, 4, 5, 6, 7};
const uint8_t PIN_CALL_BUTTON[] = {0, 1, 2, 3, 4, 5, 6, 7};
const uint8_t PIN_DOOR_BUTTON[] = {0, 1};
const uint8_t PIN_DOOR_STATE[] = {0, 1, 2, 3};
const uint8_t PIN_MOTION_STATE[] = {0, 1};
const uint8_t PIN_SERVICE_STATE[] = {0, 1, 2, 3};
const uint8_t PIN_SWITCH_SERVICE[] = {0, 1, 2, 3};

struct LIFT_STATE {
  uint8_t floor; 
  uint8_t motion_direction;
  uint8_t door_state;
  uint8_t service_state;
  uint8_t call_button;
  uint8_t door_button;
  uint8_t service_switch;
  bool call_active;
};

struct LIFT_STATE lift_state;


// IO handler
void input_output_handler(void) {
  // Set output signal: Floor
  digitalWrite(PIN_FLOOR[lift_state.floor], HIGH);
  for(int8_t i = 0; i < sizeof(PIN_FLOOR) / sizeof(PIN_FLOOR[0]); i++ ) {
    if(i != lift_state.floor)
      digitalWrite(PIN_FLOOR[i], LOW);
  }

  // Set output signal: Motion direction
  digitalWrite(PIN_MOTION_STATE[lift_state.motion_direction], HIGH);
  for(int8_t i = 0; i < sizeof(PIN_MOTION_STATE) / sizeof(PIN_MOTION_STATE[0]); i++ ) {
    if(i != lift_state.motion_direction)
      digitalWrite(PIN_MOTION_STATE[i], LOW);
  }
  
  // Set output signal: Door state
  digitalWrite(PIN_DOOR_STATE[lift_state.door_state], HIGH);
  for(int8_t i = 0; i < sizeof(PIN_DOOR_STATE) / sizeof(PIN_DOOR_STATE[0]); i++ ) {
    if(i != lift_state.door_state)
      digitalWrite(PIN_DOOR_STATE[i], LOW);
  }

  // Set output signal: Service state
  digitalWrite(PIN_SERVICE_STATE[lift_state.service_state], HIGH);
  for(int8_t i = 0; i < sizeof(PIN_SERVICE_STATE) / sizeof(PIN_SERVICE_STATE[0]); i++ ) {
    if(i != lift_state.service_state)
      digitalWrite(PIN_SERVICE_STATE[i], LOW);
  }


  // Read input signal: Call button
  for(int8_t i = 0; i < sizeof(PIN_CALL_BUTTON) / sizeof(PIN_CALL_BUTTON[0]); i++ ) {
    if(digitalRead(PIN_CALL_BUTTON[i]) == HIGH) {
      lift_state.call_button = i;
      break;
    }
    else
      lift_state.call_button = 255;
  }

  // Read input signal: Door button
  for(int8_t i = 0; i < sizeof(PIN_DOOR_BUTTON) / sizeof(PIN_DOOR_BUTTON[0]); i++ ) {
    if(digitalRead(PIN_DOOR_BUTTON[i]) == HIGH) {
      lift_state.door_button = i;
      break;
    }
    else
      lift_state.door_button = 255;
  }

  // Read input signal: Service switch
  for(int8_t i = 0; i < sizeof(PIN_SWITCH_SERVICE) / sizeof(PIN_SWITCH_SERVICE[0]); i++ ) {
    if(digitalRead(PIN_SWITCH_SERVICE[i]) == HIGH) {
      lift_state.service_switch = i;
      break;
    }
    else
      lift_state.service_switch = 255;
  }
}

void update_screen(void) {

  String floor = String(lift_state.floor);

  String direction;
  switch(lift_state.motion_direction) {
    case UP:
      direction = "UP";
      break;
    case DOWN:
      direction = "DOWN";
      break;
    case STATIONARY:
      direction = "STATIONARY";
      break;
  }

  String door;
  switch(lift_state.door_state) {
    case CLOSE:
      door = "CLOSE";
      break;
    case OPENING:
      door = "OPENING";
      break;
    case OPEN:
      door = "OPEN";
      break;
    case CLOSING:
      door = "CLOSING";
      break;
  }

  String state;
  switch(lift_state.service_state) {
    case NORMAL:
      state = "NORMAL";
      break;
    case AGV:
      state = "AGV";
      break;
    case FIRE:
      state = "FIRE";
      break;
    case OUT_OF_SERVICE:
      state = "OUT_OF_SERVICE";
      break;
  }

  u8g2.drawStr(25, 5,   floor.c_str());
  u8g2.drawStr(100, 5,  direction.c_str());
  u8g2.drawStr(25, 25,  door.c_str());
  u8g2.drawStr(25, 45,  state.c_str());
}

// Normal motion model
void lift_normal_motion_simulator(uint8_t direction, uint8_t floor_dwell, uint8_t door_dwell) {

}

// Call motion model
void lift_call_motion_simulator(void) {

}

// Lift motion simulator task
void lift_controller(void) {
  uint8_t random_direction = random(0, 2);
  uint16_t random_floor_dwell = random(1000, 30000);
  uint16_t random_door_dwell = random(5000, 60000);

  if ((lift_state.call_button != 255) && (lift_state.door_button != 255) && (lift_state.service_switch != 255))
    lift_state.call_active = true;

  if(lift_state.call_active)
    lift_call_motion_simulator();
  else
    lift_normal_motion_simulator(random_direction, random_floor_dwell, random_door_dwell);
  
}

unsigned long controller_run_time = 0;
unsigned long controller_update_rate = 100;

void loop(void) {
  // Needed to restart ESP32 after update
  AsyncElegantOTA.loop();

  // Update lift controller, if it is time
  if (millis() >= (controller_run_time + controller_update_rate))  {
    controller_run_time = millis();
    lift_controller();
  }
  
  // Update display on screen
  update_screen();

  // Update IOs
  input_output_handler();
}