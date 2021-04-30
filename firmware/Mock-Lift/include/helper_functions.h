#define debug_print(x, level) ((level) <= debug_level ? Serial.print(x) : 0)
#define debug_println(x, level) ((level) <= debug_level ? Serial.println(x) : 0)

#define sgn(x) ((x) < 0 ? -1 : ((x) > 0 ? 1 : 0))

#define pull_low(pin) (digitalWrite(pin, LOW))
#define pull_high(pin) (digitalWrite(pin, HIGH))