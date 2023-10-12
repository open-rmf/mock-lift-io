/*
  Copyright 2021 Open Source Robotics Corporation
  akash @ openrobotics . org

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

#include <includes.h>

// Screen handlers
U8G2_ST7920_128X64_F_SW_SPI u8g2(U8G2_R0, PIN_LCD[0], PIN_LCD[1], PIN_LCD[2], PIN_LCD[3]);

void display_prepare(void) {
  u8g2.clearBuffer();
  u8g2.setFontRefHeightExtendedText();
  u8g2.setDrawColor(1);
  u8g2.setFontPosTop();
  u8g2.setFontDirection(0);
}

void init_screen(void) {
  u8g2.begin();

  u8g2.setFont(font_type); 
  display_prepare();
  
  // Populate basic text
  u8g2.drawStr(0, 10, "            OSRC");
  u8g2.drawStr(0, 30, "      Lift simulator");
  if(SIMULATED_MOTION)
    u8g2.drawStr(0, 40, "Human riding this lift!");
  else
    u8g2.drawStr(0, 40, "A very obidient lift :)");
  u8g2.sendBuffer();
  delay(2000);
}

const uint8_t MIN_FLOOR = 0;
const uint8_t MAX_FLOOR = (sizeof(PIN_CALL_BUTTON)/sizeof(PIN_CALL_BUTTON[0])) - 1; // see how many call buttons we have, those are the number of floors for us

struct LIFT_STATE lift_state;

void init_lift_states(void) {
  lift_state.floor = random(MIN_FLOOR, MAX_FLOOR);
  lift_state.motion_direction = random(DOWN, UP);
  lift_state.door_state = CLOSE;
  lift_state.service_state = NORMAL;
  lift_state.call_active = false;
}

// generates a random motion direction
int8_t random_motion_direction(int8_t min, int8_t max) {
  int8_t direction;
  long rand = random(0,15000);

  if(rand <= 5000)
    direction=  -1;
  else if(rand >= 10000)
    direction= 1;
  else
    direction= 0;
  
  if((max == 0) && (direction > 0))
    direction = 0;

  if((min == 0) && (direction < 0))
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
  
  // set output pins, and tentatively initialize to low
  for(int8_t i = 0; i < sizeof(PIN_FLOOR) / sizeof(PIN_FLOOR[0]); i++ ) {
    pinMode(PIN_FLOOR[i], OUTPUT);
    digitalWrite(PIN_FLOOR[i], LOW);
  }
  for(int8_t i = 0; i < sizeof(PIN_DOOR_STATE) / sizeof(PIN_DOOR_STATE[0]); i++ ) {
    pinMode(PIN_DOOR_STATE[i], OUTPUT);
    digitalWrite(PIN_DOOR_STATE[i], LOW);
  }
  for(int8_t i = 0; i < sizeof(PIN_MOTION_STATE) / sizeof(PIN_MOTION_STATE[0]); i++ ) {
    pinMode(PIN_MOTION_STATE[i], OUTPUT);
    digitalWrite(PIN_MOTION_STATE[i], LOW);
  }
  for(int8_t i = 0; i < sizeof(PIN_SERVICE_STATE) / sizeof(PIN_SERVICE_STATE[0]); i++ ) {
    pinMode(PIN_SERVICE_STATE[i], OUTPUT);
    digitalWrite(PIN_SERVICE_STATE[i], LOW);
  }
}

AsyncWebServer server(WEBSERVER_PORT);

void setup(void) {
  
  Serial.begin(SERIAL_BAUD);

  // pull inputs, outputs low
  input_output_init();

  // Get our mac id and use it to generate a nice SSID
  String ssid_string = SSID_PREFIX + WiFi.macAddress();
  const char* ssid = ssid_string.c_str();
  
  // Start in AP mode
  debug_println("Setting AP", MINIMAL);
  WiFi.softAP(ssid, PSK);
  debug_print("OTA AP SSID: ", MINIMAL);
  debug_println(ssid, MINIMAL);
  debug_print("PSK: ", DEBUG);
  debug_println(PSK, DEBUG);

  // Print our station IP to serial monitor (for reference)
  IPAddress IP = WiFi.softAPIP();
  debug_println("OTA server IP address: ", MINIMAL);
  debug_println(IP, MINIMAL);

  // Default landing page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "Hi! I am OSRC Lift Simulator.");
  });

  // Start ElegantOTA
   AsyncElegantOTA.begin(&server, OTA_USERNAME, OTA_PASSWORD); 
  server.begin();
  debug_println("HTTP server started", MINIMAL);
  debug_print("Login credentials: ", DEBUG);
  debug_print(OTA_USERNAME, DEBUG);
  debug_print(" , ", DEBUG);
  debug_println(OTA_USERNAME, DEBUG);

  // Initialize screen
  init_screen();

  // Initialize lift at a random state
  init_lift_states();
}

// IO handler
void input_output_handler(void) {
  // Set output signal: Floor
  if ((OUTPUT_SIGNALS_IN_AGV_MODE_ONLY == true) && (lift_state.service_state == AGV))
    digitalWrite(PIN_FLOOR[lift_state.floor], HIGH);

  for(int8_t i = 0; i < sizeof(PIN_FLOOR) / sizeof(PIN_FLOOR[0]); i++ ) {
    if(i != lift_state.floor)
      digitalWrite(PIN_FLOOR[i], LOW);
  }

  /*
  for(int8_t i = 0; i < sizeof(PIN_FLOOR) / sizeof(PIN_FLOOR[0]); i++ ) {
    // if lift is not in AGV mode, dont output floor info to signal output
    if (OUTPUT_SIGNALS_IN_AGV_MODE_ONLY == true) {
      if (lift_state.service_state == AGV) {
        if(i != lift_state.floor)
          digitalWrite(PIN_FLOOR[i], LOW);
      }
      else
        digitalWrite(PIN_FLOOR[i], LOW);
    }
    else {
      if(i != lift_state.floor)
        digitalWrite(PIN_FLOOR[i], LOW);
    }
  }
  */

  // Set output signal: Motion direction
  if(lift_state.motion_direction == UP) {
    digitalWrite(PIN_MOTION_STATE[0], HIGH);
    digitalWrite(PIN_MOTION_STATE[1], LOW);
  }
  if(lift_state.motion_direction == STATIONARY) {
    digitalWrite(PIN_MOTION_STATE[0], LOW); // [AV 20210622] Changed from HIGH
    digitalWrite(PIN_MOTION_STATE[1], LOW); // [AV 20210622] Changed from HIGH
  }
  if(lift_state.motion_direction == DOWN) {
    digitalWrite(PIN_MOTION_STATE[0], LOW);
    digitalWrite(PIN_MOTION_STATE[1], HIGH);
  }

  // Set output signal: Door state
  if(lift_state.door_state == CLOSE) {
    digitalWrite(PIN_DOOR_STATE[0], HIGH); // [AV 20210622] Changed from LOW
    digitalWrite(PIN_DOOR_STATE[1], LOW); 
  }
  if(lift_state.door_state == OPENING) {
    digitalWrite(PIN_DOOR_STATE[0], LOW); 
    digitalWrite(PIN_DOOR_STATE[1], LOW); 
  }
  if(lift_state.door_state == OPEN) {
    digitalWrite(PIN_DOOR_STATE[0], LOW);  // [AV 20210622] Changed from HIGH
    digitalWrite(PIN_DOOR_STATE[1], HIGH);  
  }
  if(lift_state.door_state == CLOSING) {
    digitalWrite(PIN_DOOR_STATE[0], LOW); 
    digitalWrite(PIN_DOOR_STATE[1], LOW);  // [AV 20210622] Changed from HIGH
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
unsigned long door_open_timestamp = 0;
unsigned long door_state_change_timestamp = 0;
uint8_t _call_button;

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
// door_dwell specifies the time the door stays open by default once arrived at floor.
// door_state_change_time specifies the time it needs for the door to change states (i.e. move from closed -> opening -> open and the other way)
void lift_call_motion_simulator(uint16_t door_dwell, uint16_t door_state_change_time) {

  // if we have arrived on our intended floor, Door is Open, set call to inactive
  if(lift_state.floor == _call_button) {
    // Set our motion direction as stationary
    lift_state.motion_direction = STATIONARY;

    // Door is not open, start opening the door
    if(lift_state.door_state < OPEN)
      if(millis() >= (door_state_change_timestamp + door_dwell)) {
        door_state_change_timestamp = millis();
        lift_state.door_state++;
      }
         
    // Door is Open && Door open button is not held, set call to inactive
    else if((lift_state.door_state >= OPEN) && (lift_state.door_button != 0))
      lift_state.call_active = false;    
  }

  // if we are not on the intended floor
  else{
    // if call is for an opposite direction of motion, reject call
    if((( _call_button - (lift_state.floor + lift_state.motion_direction)) > (_call_button - lift_state.floor)) && (SIMULATED_MOTION)){
        lift_state.call_active = false;
      }
    // accept call if it is in our direction
    else {
      // if doors are Close, move lift 
      if(lift_state.door_state == CLOSE) {
        lift_state.motion_direction = sgn(_call_button - lift_state.floor);
        lift_state.floor = lift_state.floor + lift_state.motion_direction;
      }
      //otherwise start closing them
      else {
        // if doors are open dwell for a random time
        if(lift_state.door_state < OPEN) {
          door_open_timestamp = millis();
          lift_state.door_state++;
        }
        else if(lift_state.door_state == OPEN) {
          if(millis() >= (door_open_timestamp + door_dwell))
            lift_state.door_state++;
        }
        else
          lift_state.door_state = CLOSE;
      }
    }
  }
}

// Lift motion simulator task
void update_lift_controller(void) {
  // if we have a new incoming call request, take it if the door open button is not held
  if ((lift_state.call_button != 255) && (lift_state.door_button != 0)){
    lift_state.call_active = true;
    _call_button = lift_state.call_button;
  }

  // if we are in fire or out of service, cancel calls and put lift in the right service state
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

  // if call is complete, clear the car call 
  if(lift_state.call_active == false)
    _call_button = 255;

  // randomize next motion update time and run appropriate motion simulator if it is time
  if (millis() >= (motion_update_time + random(3000, 8000)))  {
    motion_update_time = millis();

    // randomize door dwell time while running motion simulation
    if(lift_state.call_active)
      lift_call_motion_simulator(random(3000, 10000), DOOR_STATE_CHANGE_TIME);
    else if(SIMULATED_MOTION)
      lift_normal_motion_simulator(random(3000, 10000));
    else
      lift_state.motion_direction = STATIONARY;
  }
}

unsigned long screen_update_timestamp=0;

void loop(void) {
  // Update IOs
  input_output_handler();

  // Update lift controller
  update_lift_controller();
  
  // Update display on screen, if it is time
  if((screen_update_timestamp + 1000) <= millis()) {
    screen_update_timestamp = millis();
    update_screen();
  }

  // kill and restart ESP32 if OTA update receives new firmware
  AsyncElegantOTA.loop();
}