#include <Arduino.h>

struct Beacon {
  String uuid;
  int rssi;
  int major;
  int minor;
  signed char power;
};
