#include <Arduino.h>

#define sgn(x) ((x) < 0 ? -1 : ((x) > 0 ? 1 : 0))

enum Debug_Levels {
  NONE = 0,
  MINIMAL = 1,
  DEBUG = 2,
  VERBOSE = 3
};

#define debug_level DEBUG
#define debug_print(x, level) ((level) <= debug_level ? Serial.print(x) : 0)
#define debug_println(x, level) ((level) <= debug_level ? Serial.println(x) : 0)
//#define debug_println(x, level) (Serial.println(x))

// Wifi and OTA
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>

// LCD driver
#include <U8g2lib.h>

AsyncWebServer server(80);
U8G2_ST7920_128X64_F_SW_SPI u8g2(U8G2_R0, /* clock=*/ 18, /* data=*/ 23, /* cs=*/ 2, /* reset=*/ 4);

uint8_t font_height;
// Screen handler
void display_prepare(void) {
  u8g2.clearBuffer();
  u8g2.setFontRefHeightExtendedText();
  u8g2.setDrawColor(1);
  u8g2.setFontPosTop();
  u8g2.setFontDirection(0);
}

void init_screen(void) {
  u8g2.begin();

  u8g2.setFont(u8g2_font_5x7_tf);
  font_height = 6;  
  display_prepare();
  
  // Populate basic text
  u8g2.drawStr(10, 10, "OSRC");
  u8g2.drawStr(10, 20, "Lift simulator");
  u8g2.sendBuffer();
  delay(1000);
}
// dont use {11, 10, 9}
//{13}, {12, 14, 27, 26, 25, 33, 32, 35, 34, 39, 36}


//{6, 7, 8, 15, 2, 0, 4, 16, 17, 5, 18, 19}, {21, 3, 1, 22, 23}
// dont use {6, 7, 8, 2, 4, 5, 18, 19}, {3, 1, 23}
//==> {15, 0, 16, 17}, {21, 22}

/*
const uint8_t PIN_FLOOR[] = {11, 10, 9};
const uint8_t PIN_DOOR_STATE[] = {13, 12}; 
const uint8_t PIN_MOTION_STATE[] = {14, 27};
const uint8_t PIN_SERVICE_STATE[] = {26, 25}; //,33, 32, 35, 34, 39

const uint8_t PIN_CALL_BUTTON[] = {15, 0, 16};
const uint8_t PIN_DOOR_BUTTON[] = {17};

const uint8_t PIN_SWITCH_SERVICE[] = {21, 22}; //fire, out of service 
*/

const uint8_t PIN_FLOOR[] = {12, 14, 27};
const uint8_t PIN_DOOR_STATE[] = {26, 25}; 
const uint8_t PIN_MOTION_STATE[] = {33, 32};
const uint8_t PIN_SERVICE_STATE[] = {35, 34}; //39

const uint8_t PIN_CALL_BUTTON[] = {15, 0, 16}; 
const uint8_t PIN_DOOR_BUTTON[] = {36};
const uint8_t PIN_SWITCH_SERVICE[] = {13, 17}; //fire, out of service //11,10,9




struct LIFT_STATE {
  uint8_t floor; 
  int8_t motion_direction;
  uint8_t door_state;
  uint8_t service_state;
  uint8_t call_button;
  uint8_t door_button;
  uint8_t service_switch;
  bool call_active;
};

enum Motion_Directions {
  DOWN = -1,
  STATIONARY = 0,
  UP = 1
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

const uint8_t MIN_FLOOR = 0;
const uint8_t MAX_FLOOR = 2;

struct LIFT_STATE lift_state;

void init_lift_states(void) {
  lift_state.floor = random(MIN_FLOOR, MAX_FLOOR);
  lift_state.motion_direction = random(DOWN, UP);
  lift_state.door_state = CLOSE;
  lift_state.service_state = NORMAL;
  lift_state.call_active = false;
}

int8_t random_motion_direction(int8_t min, int8_t max) {
  int8_t direction;
  long rand = random(0,15000);

  if(rand <= 5000)
    direction=  -1;
  else if(rand >= 10000)
    direction= 1;
  else
    direction= 0;
  
  if(max == 0)
    if(direction > 0)
      direction = 0;
  if(min == 0)
    if(direction < 0)
      direction = 0;
  
  return direction;
}

void input_output_init(void) {
  // pull inputs low
  for(int8_t i = 0; i < sizeof(PIN_CALL_BUTTON) / sizeof(PIN_CALL_BUTTON[0]); i++ ) {
    pinMode(PIN_CALL_BUTTON[i], INPUT);
    digitalWrite(PIN_CALL_BUTTON[i], LOW);
  }
      
  for(int8_t i = 0; i < sizeof(PIN_DOOR_BUTTON) / sizeof(PIN_DOOR_BUTTON[0]); i++ ) {
    pinMode(PIN_DOOR_BUTTON[i], INPUT);
    digitalWrite(PIN_DOOR_BUTTON[i], LOW);
  }
  for(int8_t i = 0; i < sizeof(PIN_SWITCH_SERVICE) / sizeof(PIN_SWITCH_SERVICE[0]); i++ ) {
    pinMode(PIN_SWITCH_SERVICE[i], INPUT);
    digitalWrite(PIN_SWITCH_SERVICE[i], LOW);
  }
    
}
 
void setup(void) {
  // Set serial baud to 115200bps
  Serial.begin(115200);

  // pull inputs low
  input_output_init();

  // Get our mac id and use it to generate a nice SSID
  String ssid_prefix = "Mock-Lift-";
  String ssid_string = ssid_prefix + WiFi.macAddress();
  const char* ssid = ssid_string.c_str();
  const char* password = "open-rmf";

  // Start in AP mode
  debug_println("Setting AP", MINIMAL);
  WiFi.softAP(ssid, password);
  debug_print("OTA SSID: ", MINIMAL);
  debug_println(ssid, MINIMAL);
  debug_print("PSK: ", DEBUG);
  debug_println(password, DEBUG);

  // Print our station IP to serial monitor (for reference)
  IPAddress IP = WiFi.softAPIP();
  debug_println("OTA server IP address: ", MINIMAL);
  debug_println(IP, MINIMAL);

  // Default landing page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "Hi! I am Mock Lift controller.");
  });

  // Start ElegantOTA
  const char* ota_username = "user";
  const char* ota_password = "password";

  AsyncElegantOTA.begin(&server, ota_username, ota_password);    
  server.begin();
  debug_println("HTTP server started", MINIMAL);
  debug_print("Login credentials: ", DEBUG);
  debug_print(ota_username, DEBUG);
  debug_print(" , ", DEBUG);
  debug_println(ota_username, DEBUG);

  // Initialize screen
  init_screen();

  // Initialize lift at a random state
  init_lift_states();
}

// IO handler
void input_output_handler(void) {
  // Set output signal: Floor
  digitalWrite(PIN_FLOOR[lift_state.floor], HIGH);
  for(int8_t i = 0; i < sizeof(PIN_FLOOR) / sizeof(PIN_FLOOR[0]); i++ ) {
    if(i != lift_state.floor)
      digitalWrite(PIN_FLOOR[i], LOW);
  }

  // Set output signal: Motion direction
  if(lift_state.motion_direction == UP) {
    digitalWrite(PIN_MOTION_STATE[0], HIGH);
    digitalWrite(PIN_MOTION_STATE[1], LOW);
  }
  if(lift_state.motion_direction == STATIONARY) {
    digitalWrite(PIN_MOTION_STATE[0], HIGH);
    digitalWrite(PIN_MOTION_STATE[1], HIGH);
  }
  if(lift_state.motion_direction == DOWN) {
    digitalWrite(PIN_MOTION_STATE[0], LOW);
    digitalWrite(PIN_MOTION_STATE[1], HIGH);
  }

  // Set output signal: Door state
  if(lift_state.door_state == CLOSE) {
    digitalWrite(PIN_DOOR_STATE[0], LOW);
    digitalWrite(PIN_DOOR_STATE[1], LOW);
  }
  if(lift_state.door_state == OPENING) {
    digitalWrite(PIN_DOOR_STATE[0], HIGH);
    digitalWrite(PIN_DOOR_STATE[1], LOW);
  }
  if(lift_state.door_state == OPEN) {
    digitalWrite(PIN_DOOR_STATE[0], HIGH);
    digitalWrite(PIN_DOOR_STATE[1], HIGH);
  }
  if(lift_state.door_state == CLOSING) {
    digitalWrite(PIN_DOOR_STATE[0], LOW);
    digitalWrite(PIN_DOOR_STATE[1], HIGH);
  }

  // Set output signal: Service state
  if(lift_state.service_state == NORMAL) {
    digitalWrite(PIN_SERVICE_STATE[0], LOW);
    digitalWrite(PIN_SERVICE_STATE[1], LOW);
  }
  if(lift_state.service_state == AGV) {
    digitalWrite(PIN_SERVICE_STATE[0], LOW);
    digitalWrite(PIN_SERVICE_STATE[1], HIGH);
  }
  if(lift_state.service_state == FIRE) {
    digitalWrite(PIN_SERVICE_STATE[0], HIGH);
    digitalWrite(PIN_SERVICE_STATE[1], HIGH);
  }
  if(lift_state.service_state == OUT_OF_SERVICE) {
    digitalWrite(PIN_SERVICE_STATE[0], HIGH);
    digitalWrite(PIN_SERVICE_STATE[1], LOW);
  }

  // Read input signal: Call button
  for(int8_t i = 0; i < sizeof(PIN_CALL_BUTTON) / sizeof(PIN_CALL_BUTTON[0]); i++ ) {
    if(digitalRead(PIN_CALL_BUTTON[i]) == HIGH) {
      lift_state.call_button = i;
      debug_print("PIN_CALL_BUTTON[", DEBUG); debug_print(i, DEBUG); debug_println("]", DEBUG);
      break;
    }
    else
      lift_state.call_button = 255;
  }

  // Read input signal: Door button
  for(int8_t i = 0; i < sizeof(PIN_DOOR_BUTTON) / sizeof(PIN_DOOR_BUTTON[0]); i++ ) {
    if(digitalRead(PIN_DOOR_BUTTON[i]) == HIGH) {
      lift_state.door_button = i;
      debug_print("PIN_DOOR_BUTTON[", DEBUG); debug_print(i, DEBUG); debug_println("]", DEBUG);
      break;
    }
    else
      lift_state.door_button = 255;
  }

  // Read input signal: Service switch
  for(int8_t i = 0; i < sizeof(PIN_SWITCH_SERVICE) / sizeof(PIN_SWITCH_SERVICE[0]); i++ ) {
    if(digitalRead(PIN_SWITCH_SERVICE[i]) == HIGH) {
      lift_state.service_switch = i+2;
      debug_print("PIN_SWITCH_SERVICE[", DEBUG); debug_print(i, DEBUG); debug_println("]", DEBUG);
      break;
    }
    else
      lift_state.service_switch = 255;
  }
}

void update_screen(void) {

  String floor = String(lift_state.floor);

  String direction;
  if (lift_state.motion_direction == UP)
    direction = "UP";
  if (lift_state.motion_direction == DOWN)
    direction = "DOWN";
  if (lift_state.motion_direction == STATIONARY)
    direction = "STATIONARY";

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

  String call_active;
  if(lift_state.call_active)
    call_active = "TRUE";
  else
    call_active = "FALSE";

  display_prepare();
  uint8_t x=0,y=0;
  uint8_t y_spacing=5;
  uint8_t x_spacing=55;

  y+= 0;
  u8g2.drawStr(x, y, "Floor:");
  u8g2.drawStr(x+x_spacing, y, floor.c_str());

  y+= font_height + y_spacing;
  u8g2.drawStr(x, y, "Direction:");
  u8g2.drawStr(x+x_spacing, y, direction.c_str());

  y+= font_height + y_spacing;
  u8g2.drawStr(x, y, "Door:");
  u8g2.drawStr(x+x_spacing, y, door.c_str());

  y+= font_height + y_spacing;
  u8g2.drawStr(x, y, "State:");
  u8g2.drawStr(x+x_spacing, y, state.c_str());  

  u8g2.sendBuffer();

  debug_print("FLOOR,DIRECTION,DOOR,STATE,CALL: ", DEBUG);
  debug_print(floor, DEBUG);
  debug_print(",", DEBUG);
  debug_print(direction, DEBUG);
  debug_print(",", DEBUG);
  debug_print(door, DEBUG);
  debug_print(",", DEBUG);
  debug_println(state, DEBUG);
}

unsigned long motion_update_time = 0;
unsigned long motion_update_rate;
unsigned long door_open_timestamp;
uint8_t _call_button;
uint8_t _open_button;

// Normal motion model
void lift_normal_motion_simulator(uint16_t door_dwell) {
    
  // If lift was set to be in motion in previous cycle
  if(lift_state.motion_direction != STATIONARY) {
    // Move lift in the direction of the motion, while making sure we dont shoot beyond MIN or MAX floors
    lift_state.floor = lift_state.floor + lift_state.motion_direction;

    // Set random direction for next motion
    if (lift_state.floor >= MAX_FLOOR) {
      lift_state.floor = MAX_FLOOR;
      lift_state.motion_direction = random_motion_direction(DOWN, STATIONARY);
    }

    else if(lift_state.floor <= MIN_FLOOR) {
      lift_state.floor = MIN_FLOOR;
      lift_state.motion_direction = random_motion_direction(STATIONARY, UP);
    }

    else {
        if(lift_state.motion_direction > STATIONARY)
          lift_state.motion_direction = random_motion_direction(STATIONARY, UP);

        else if(lift_state.motion_direction < STATIONARY)
          lift_state.motion_direction = random_motion_direction(DOWN, STATIONARY);
    }
  }

  // If lift was set to be in stationary in previous cycle
  else {
    if(lift_state.door_state < OPEN) {
      lift_state.door_state++;

      // record the time when we set the door to open
      door_open_timestamp = millis();
    }
    else if(lift_state.door_state == OPEN) {
      // leave door open for the dwell time assigned to us
      if ((millis() - door_open_timestamp) >= door_dwell)
        lift_state.door_state++;
    }
    else {
      lift_state.door_state = CLOSE;

      // Set random direction for next motion
      if (lift_state.floor >= MAX_FLOOR) {
        lift_state.floor = MAX_FLOOR;
        lift_state.motion_direction = random_motion_direction(DOWN, STATIONARY);
      }

      else if(lift_state.floor <= MIN_FLOOR) {
        lift_state.floor = MIN_FLOOR;
        lift_state.motion_direction = random_motion_direction(STATIONARY, UP);
      }

      else {
        lift_state.motion_direction = random_motion_direction(DOWN, UP);
      }
    }
  }
    
}

// Call motion model
void lift_call_motion_simulator(void) {

  // if we have arrived on our intended floor, Door is Open, set call to inactive
  if((lift_state.floor == _call_button) && (lift_state.door_state >= OPEN)){
    lift_state.call_active = false;
  }

  // else make the lift move
  else{
    // if call is for an opposite direction of motion, reject call
    if(( _call_button - (lift_state.floor + lift_state.motion_direction)) > (_call_button - lift_state.floor)) {
        lift_state.call_active = false;
        _call_button = 255;
      }
    else {
        // if we are at the floor we intend to be, start opening door
        if(lift_state.floor == _call_button) {
          if(lift_state.door_state <= OPENING) {
           lift_state.door_state++;
           door_open_timestamp = millis();
          }

          // If door is OPEN && dwell time is more than random && OPEN button is not pressed
          if((lift_state.door_state == OPEN) && ((millis() - door_open_timestamp) > random(5000, 15000)) && (_open_button != 0))
            lift_state.door_state++;
        }
        // set motion direction and move lift if needed
        lift_state.motion_direction = sgn(_call_button - lift_state.floor);
        lift_state.floor = lift_state.floor + lift_state.motion_direction;
    }
  }
}

// Lift motion simulator task
void update_lift_controller(void) {
  uint16_t random_door_dwell = random(3000, 10000);

  if (lift_state.call_button != 255) {
    lift_state.call_active = true;
    _call_button = lift_state.call_button;
  }
  if (lift_state.door_button != 255) {
    _open_button = lift_state.door_button;
  }

  if (lift_state.service_switch != 255) {
    lift_state.service_state = lift_state.service_switch;
    lift_state.call_active = false;
  }
  else
  {
    if(lift_state.call_active)
      lift_state.service_state = AGV;
    else
      lift_state.service_state = NORMAL;
  }  

  // randomize next motion update time
  motion_update_rate = random(2000, 5000);

  if (millis() >= (motion_update_time + motion_update_rate))  {
    motion_update_time = millis();

    if(lift_state.call_active)
      lift_call_motion_simulator();
    else
      lift_normal_motion_simulator(random_door_dwell);
  }
}

unsigned long screen_update_timestamp=0;

void loop(void) {
  // Update IOs
  input_output_handler();

  // Update lift controller
  update_lift_controller();
  
  // Update display on screen
  if((screen_update_timestamp + 1000) <= millis()) {
    screen_update_timestamp = millis();
    update_screen();
  }

  // Loop call needed to restart ESP32 after update
  AsyncElegantOTA.loop();
}