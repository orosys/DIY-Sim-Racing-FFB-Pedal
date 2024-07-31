#pragma once

#include "Arduino.h"
#include "Main.h"

/*#ifdef CONFIG_IDF_TARGET_ESP32S2 ||ARDUINO_ESP32S3_DEV || CONFIG_IDF_TARGET_ESP32S3
  #define USB_JOYSTICK
#elif CONFIG_IDF_TARGET_ESP32
  #define BLUETOOTH_GAMEPAD
#endif
*/




void SetupController();
bool IsControllerReady();

int32_t NormalizeControllerOutputValue(float value, float minVal, float maxVal, float maxGameOutput);
void SetControllerOutputValueBrake(int32_t value);
void SetControllerOutputValueAccelerator(int32_t value);
void SetControllerOutputValueThrottle(int32_t value);
void joystickSendState();