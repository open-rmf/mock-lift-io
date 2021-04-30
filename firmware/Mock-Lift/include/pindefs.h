/*
Pin map
{13}, {12, 14, 27, 26, 25, 33, 32}, Inputs only {35, 34, 39, 36}
{2, 0, 4, 16, 17} , {21, 3, 1, 22}

//2, 0 are unused
*/

const uint8_t PIN_FLOOR[] = {4, 16, 17, 21, 3};
const uint8_t PIN_DOOR_STATE[] = {1, 22}; 
const uint8_t PIN_MOTION_STATE[] = {13, 12};
const uint8_t PIN_SERVICE_STATE[] = {14, 27};

const uint8_t PIN_CALL_BUTTON[] = {26, 25, 33, 32, 35};
const uint8_t PIN_DOOR_BUTTON[] = {34};

const uint8_t PIN_SWITCH_SERVICE[] = {39, 36}; //fire, out of service

// LCD pins
const uint8_t PIN_LCD[] = {18, 23, 5, 15}; //clk, data, cs, rst