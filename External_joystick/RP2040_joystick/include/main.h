#pragma once
#include<Arduino.h>
#include <stdint.h>
#define DAP_PAYLOAD_TYPE_JOYSTICKUART 215
#define DAP_JOY_VERSION 0x01
#define DAP_JOY_KEY 0x97
#define JOYSTICK_VALUE_MAX  10000
#define JOYSTICK_RANGE_LOCAL  1024
//const uint8_t DAP_PAYLOAD_TYPE_JOYSTICKUART=215;
//const uint16_t JOYSTICK_VALUE_MAX=10000;

struct __attribute__((packed)) payloadjoystick
{
  uint8_t payloadtype;
  uint8_t key;
  uint8_t DAP_JOY_Version;
  int16_t controllerValue_i32[3];
  int8_t pedal_status;
  uint8_t pedalAvailability[3];
  uint8_t JoystickAction;
};
struct __attribute__((packed)) JoystickPayloadFooter
{
  // To check if structure is valid
  uint16_t checkSum;
};
struct __attribute__((packed)) DAP_JoystickUART_State
{
  payloadjoystick _payloadjoystick;
  JoystickPayloadFooter _payloadfooter;
};

enum Pedal_status
{
  Pedal_status_Pedal,
  Pedal_status_Rudder,
  Pedal_status_RudderBrake
};

enum JoystickAction
{
  NONE,
  JOYSTICKACTION_DEBUG_MODE,
  JOYSTICKACTION_RESET_INTO_BOOTLOADER,
  JOYSTICK_RESTART
};
