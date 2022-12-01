#ifndef ENUMS_STRUCTS_H 
#define ENUMS_STRUCTS_H

enum Debug_Levels {
  NONE = 0,
  MINIMAL = 1,
  DEBUG = 2,
  VERBOSE = 3
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

enum Motion_Directions {
  DOWN = -1,
  STATIONARY = 0,
  UP = 1
};

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

#endif //ENUMS_STRUCTS_H