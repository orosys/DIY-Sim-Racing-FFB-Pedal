#pragma once

#include "Arduino.h"
#include "Main.h"

static const int16_t JOYSTICK_MIN_VALUE = 0;
static const int16_t JOYSTICK_MAX_VALUE = 10000;
static const int16_t JOYSTICK_RANGE = JOYSTICK_MAX_VALUE - JOYSTICK_MIN_VALUE;
/*#ifdef CONFIG_IDF_TARGET_ESP32S2 ||ARDUINO_ESP32S3_DEV || CONFIG_IDF_TARGET_ESP32S3
  #define USB_JOYSTICK
#elif CONFIG_IDF_TARGET_ESP32
  #define BLUETOOTH_GAMEPAD
#endif
*/
#ifdef USB_JOYSTICK
  //#include <Joystick_ESP32S2.h>
  #include "Joystick_ESP32S2.h"
#endif



void SetupController();
bool IsControllerReady();

int32_t NormalizeControllerOutputValue(float value, float minVal, float maxVal, float maxGameOutput);
void SetControllerOutputValueBrake(int32_t value);
void SetControllerOutputValueAccelerator(int32_t value);
void SetControllerOutputValueThrottle(int32_t value);
void SetControllerOutputValueRudder(int32_t value);
void SetControllerOutputValueRudder_brake(int32_t value, int32_t value2);
void joystickSendState();
bool GetJoystickStatus();
void RestartJoystick();
