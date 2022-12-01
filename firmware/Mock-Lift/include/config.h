#ifndef CONFIG_H 
#define CONFIG_H 

// debug info print level
#define DEBUG_LEVEL DEBUG

// LCD
#define font_height 6
#define font_type u8g2_font_5x7_tf
// font reference: https://github.com/olikraus/u8g2/wiki/fntlistall

// Set serial baud to 115200bps
#define SERIAL_BAUD 115200

// OTA AP setup
#define SSID_PREFIX "Mock-Lift-"
#define PSK "open-rmf"

// OTA webserver port
#define WEBSERVER_PORT 80

// OTA credentials
#define OTA_USERNAME "user"
#define OTA_PASSWORD "password"

// Simulated lift motion by "humans"
#define SIMULATED_MOTION true

#endif