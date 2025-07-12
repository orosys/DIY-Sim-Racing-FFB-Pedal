#pragma once

#include "Arduino.h"
#include "Main.h"

/*#ifdef CONFIG_IDF_TARGET_ESP32S2 ||ARDUINO_ESP32S3_DEV || CONFIG_IDF_TARGET_ESP32S3
  #define USB_JOYSTICK
#elif CONFIG_IDF_TARGET_ESP32
  #define BLUETOOTH_GAMEPAD
#endif
*/
static const int16_t JOYSTICK_MIN_VALUE = 0;
static const int16_t JOYSTICK_MAX_VALUE = 10000;
static const int16_t JOYSTICK_RANGE = JOYSTICK_MAX_VALUE - JOYSTICK_MIN_VALUE;



void SetupController();
void SetupController_USB(uint8_t pedal_ID);
bool IsControllerReady();

void SetControllerOutputValue(int32_t value);
void SetControllerOutputValue_rudder(int32_t value, int32_t value2);
int32_t NormalizeControllerOutputValue(float value, float minVal, float maxVal, float maxGameOutput);
#ifdef USB_JOYSTICK
  void JoystickSendState();
  bool GetJoystickStatus();
  void RestartJoystick();
#endif
