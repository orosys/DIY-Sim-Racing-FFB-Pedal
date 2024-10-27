#pragma once
//#include <ADS1256.h>


//#define PRINT_TASK_FREE_STACKSIZE_IN_WORDS

/********************************************************************/
/*                      Select the PCB      */
/********************************************************************/
#ifndef PCB_VERSION
  //#define PCB_VERSION 1 // V1 for regular ESP32
  //#define PCB_VERSION 2 // V1 for ESP32 S2 mini
  #define PCB_VERSION 3 // V3 for regular ESP32
  //#define PCB_VERSION 4 // speedcrafter PCB V1.3
  //#define PCB_VERSION 5 // speedcrafter PCB V1.4
  //#define PCB_VERSION 6 // V1 for ESP32 S3
#endif



/********************************************************************/
/*                      Other defines       */
/********************************************************************/

// target cycle time for pedal update task, to get constant cycle times, required for FIR filtering
#define DAP_MICROSECONDS_PER_SECOND 1000000

static const uint32_t STEPS_PER_MOTOR_REVOLUTION = 6400;

// 15kHz
//#define ADC_SAMPLE_RATE ADS1256_DRATE_15000SPS
//#define PUT_TARGET_CYCLE_TIME_IN_US DAP_MICROSECONDS_PER_SECOND / 15000

// 7.5kHz
//#define ADC_SAMPLE_RATE ADS1256_DRATE_7500SPS
//#define PUT_TARGET_CYCLE_TIME_IN_US DAP_MICROSECONDS_PER_SECOND / 7500

// 3.75kHz
//#define ADC_SAMPLE_RATE ADS1256_DRATE_3750SPS
//#define PUT_TARGET_CYCLE_TIME_IN_US DAP_MICROSECONDS_PER_SECOND / 3750

// 2.0kHz
// #define ADC_SAMPLE_RATE ADS1256_DRATE_2000SPS
// #define PUT_TARGET_CYCLE_TIME_IN_US DAP_MICROSECONDS_PER_SECOND / 2000

// 1.0kHz
#define ADC_SAMPLE_RATE ADS1256_DRATE_1000SPS
#define PUT_TARGET_CYCLE_TIME_IN_US DAP_MICROSECONDS_PER_SECOND / 1000



/********************************************************************/
/*                      Loadcell defines                            */
/********************************************************************/
#define LOADCELL_WEIGHT_RATING_KG 300.0
#define LOADCELL_EXCITATION_V 5.0
#define LOADCELL_SENSITIVITY_MV_V 2.0


/********************************************************************/
/*                      Motor defines                            */
/********************************************************************/
//#define MOTOR_INVERT_MOTOR_DIR false




/********************************************************************/
/*                      PIN defines                                 */
/********************************************************************/
// initial version of dev PCB for regular ESP32
#if PCB_VERSION == 1
  // ADC defines
  #define PIN_DRDY 17// 17 --> DRDY
  #define PIN_RST  16 // X --> X
  #define PIN_SCK 18 // 18 -->SCLK
  #define PIN_MISO 19 // 19 --> DOUT
  #define PIN_MOSI 23 // 23 --> DIN
  #define PIN_CS 5 // 5 --> CS

  // stepper pins
  #define dirPinStepper    0
  #define stepPinStepper   4

  // endstop pins
  #define minPin 34
  #define maxPin 35

  // level shifter not present on this PCB design
  #define SENSORLESS_HOMING false

  #define BLUETOOTH_GAMEPAD
  //#define USB_JOYSTICK
  #define SERIAL_COOMUNICATION_TASK_DELAY_IN_MS 1
#endif


// initial version of dev PCB for ESP32 S2 mini
#if PCB_VERSION == 2
  // ADC defines
  #define PIN_DRDY 37// 37 --> DRDY
  #define PIN_RST  16 // X --> X
  #define PIN_SCK 18 // 18 -->SCLK
  #define PIN_MISO 35 // 35 --> DOUT
  #define PIN_MOSI 33 // 33 --> DIN
  #define PIN_CS 39 // 39 --> CS

  // stepper pins
  #define dirPinStepper    8
  #define stepPinStepper   9

  // endstop pins
  #define minPin 11
  #define maxPin 10

  // level shifter not present on this PCB design
  #define SENSORLESS_HOMING false

  //#define BLUETOOTH_GAMEPAD
  #define USB_JOYSTICK
  #define SERIAL_COOMUNICATION_TASK_DELAY_IN_MS 1
#endif



// V3 version of dev PCB for regular ESP32
#if PCB_VERSION == 3
  // ADC defines
  #define PIN_DRDY 19// 19 --> DRDY
  #define PIN_RST  15 // X --> X
  #define PIN_SCK 16 // 16 -->SCLK
  #define PIN_MISO 18 // 18 --> DOUT
  #define PIN_MOSI 17 // 17 --> DIN
  #define PIN_CS 21 // 21 --> CS

  // stepper pins
  #define dirPinStepper    22
  #define stepPinStepper   23
  //analog output pin
  #define D_O 25 
  //I2Cpins
  #define I2C_SDA 32
  #define I2C_SCL 33
  
  // endstop pins
  #define minPin 12
  #define maxPin 13

  // level shifter is present on this PCB design
  #define SENSORLESS_HOMING true
  #define ISV57_TXPIN 27 //17
  #define ISV57_RXPIN 26 // 16
  //pedal assignment
  #define PEDAL_ASSIGNMENT
  #define CFG1 15
  #define CFG2 12
  #define Using_analog_output
  //#define Using_I2C_Sync
  #define ESPNOW_Enable
  #define ESPNow_ESP32
  #define I2C_slave_address 0x15
  #define BLUETOOTH_GAMEPAD
  //#define USB_JOYSTICK
  #define SERIAL_COOMUNICATION_TASK_DELAY_IN_MS 1
  #define ESPNow_Pairing_function
  //#define Pairing_GPIO 13
  //#define ESPNow_debug_rudder
#endif



// speedcrafter PCB V1.3
#if PCB_VERSION == 4
  // ADC defines
  #define PIN_DRDY 27// 19 --> DRDY
  #define PIN_RST  5 // X --> X
  #define PIN_SCK 14 // 16 -->SCLK
  #define PIN_MISO 12 // 18 --> DOUT
  #define PIN_MOSI 13 // 17 --> DIN
  #define PIN_CS 15 // 21 --> CS

  // stepper pins
  #define dirPinStepper    32
  #define stepPinStepper   33
  
  // endstop pins
  #define minPin 35
  #define maxPin 34

  // level shifter is present on this PCB design
  #define SENSORLESS_HOMING true
  #define ISV57_TXPIN 27 //17
  #define ISV57_RXPIN 26 // 16

  #define BLUETOOTH_GAMEPAD
  //#define USB_JOYSTICK

  #define SERIAL_COOMUNICATION_TASK_DELAY_IN_MS 3
  //#define Pairing_GPIO 0
  #define ESPNow_Pairing_function
  #define ESPNow_ESP32
#endif




// Speedcrafters PCB V1.4
#if PCB_VERSION == 5
  // ADC defines
  #define PIN_DRDY 19// 19 --> DRDY
  #define PIN_RST  34 // X --> X
  #define PIN_SCK 15 // 16 -->SCLK
  #define PIN_MISO 18 // 18 --> DOUT
  #define PIN_MOSI 13 // 17 --> DIN
  #define PIN_CS 21 // 21 --> CS

  // stepper pins
  #define dirPinStepper    22
  #define stepPinStepper   23
  
  // endstop pins
  #define minPin 35
  #define maxPin 34

  // level shifter is present on this PCB design
  #define SENSORLESS_HOMING true
  #define ISV57_TXPIN 26 //17
  #define ISV57_RXPIN 27 // 16
  #define ESPNOW_Enable
  #define BLUETOOTH_GAMEPAD
  //#define USB_JOYSTICK
  #define SERIAL_COOMUNICATION_TASK_DELAY_IN_MS 3
  #define Pairing_GPIO 0
  #define ESPNow_Pairing_function
  #define ESPNow_ESP32
#endif




// V3 version of dev PCB for ESP32 S3
// flash instructions, see https://hutscape.com/tutorials/hello-arduino-esp32s3
// 1. ESP32S3 Dev Module
// 2. USB CDC On Boot Enabled
#if PCB_VERSION == 6
  // ADC defines
  #define PIN_DRDY 15//19// 19 --> DRDY
  #define PIN_RST  6 // X --> X
  #define PIN_SCK 16//16 // 16 -->SCLK
  #define PIN_MISO 18 // 18 --> DOUT
  #define PIN_MOSI 17 // 17 --> DIN
  #define PIN_CS 7//21 // 21 --> CS

  // stepper pins
  #define dirPinStepper    37//22
  #define stepPinStepper   38//23

  //analog output pin
  //#define D_O 25   
  //MCP4725 SDA SCL
  #define MCP_SDA 48
  #define MCP_SCL 47
  
  // endstop pins
  #define minPin 12
  #define maxPin 13

  // level shifter is present on this PCB design
  #define SENSORLESS_HOMING true
  #define ISV57_TXPIN 10//27 //17
  #define ISV57_RXPIN 9//26 // 16

  #define Using_analog_output_ESP32_S3

  //#define BLUETOOTH_GAMEPAD
  #define USB_JOYSTICK
  #define ESPNOW_Enable
  #define ESPNow_S3
  #define SERIAL_COOMUNICATION_TASK_DELAY_IN_MS 5
  #define ESPNow_Pairing_function
  #define Pairing_GPIO 0
#endif

// For Gilphilbert PCBA design
// flash instructions, see https://hutscape.com/tutorials/hello-arduino-esp32s3
// 1. USB CDC On Boot Enabled
#if PCB_VERSION == 7
  // ADC defines
  #define PIN_DRDY 15//19// 19 --> DRDY
  #define PIN_RST  6 // X --> X
  #define PIN_SCK 16//16 // 16 -->SCLK
  #define PIN_MISO 18 // 18 --> DOUT
  #define PIN_MOSI 17 // 17 --> DIN
  #define PIN_CS 7//21 // 21 --> CS

  // stepper pins
  #define dirPinStepper    37//22
  #define stepPinStepper   38//23

  //analog output pin
  //#define D_O 25   
  //MCP4725 SDA SCL
  #define MCP_SDA 5
  #define MCP_SCL 4
  
  // endstop pins
  #define minPin 12
  #define maxPin 13
  // Pedal assignment pin
  #define PEDAL_ASSIGNMENT
  #define CFG1 1
  #define CFG2 2

  #define EMERGENCY_BUTTON
  #define ShutdownPin 6
  // level shifter is present on this PCB design
  #define SENSORLESS_HOMING true
  #define ISV57_TXPIN 10//27 //17
  #define ISV57_RXPIN 9//26 // 16

  //#define Using_analog_output_ESP32_S3
  #define ESPNOW_Enable
  #define ESPNow_S3
  //#define BLUETOOTH_GAMEPAD
  #define USB_JOYSTICK

  #define SERIAL_COOMUNICATION_TASK_DELAY_IN_MS 5
  #define ESPNow_Pairing_function
  #define Hardware_Pairing_button
  #define Pairing_GPIO 0
  //#define ESPNow_debug_rudder
  #define USING_LED
  #define LED_GPIO 12
#endif

#if PCB_VERSION == 8
  // ADC defines
  #define PIN_DRDY 15//19// 19 --> DRDY
  #define PIN_RST  6 // X --> X
  #define PIN_SCK 16//16 // 16 -->SCLK
  #define PIN_MISO 18 // 18 --> DOUT
  #define PIN_MOSI 17 // 17 --> DIN
  #define PIN_CS 7//21 // 21 --> CS

  // stepper pins
  #define dirPinStepper    37//22
  #define stepPinStepper   38//23

  //analog output pin
  //#define D_O 25   
  //MCP4725 SDA SCL
  #define MCP_SDA 5
  #define MCP_SCL 4
  
  // endstop pins
  #define minPin 12
  #define maxPin 13
  // Pedal assignment pin
  #define PEDAL_ASSIGNMENT
  #define CFG1 1
  #define CFG2 2

  #define EMERGENCY_BUTTON
  #define ShutdownPin 6
  // level shifter is present on this PCB design
  #define SENSORLESS_HOMING true
  #define ISV57_TXPIN 10//27 //17
  #define ISV57_RXPIN 9//26 // 16

  //#define Using_analog_output_ESP32_S3
  #define ESPNOW_Enable
  #define ESPNow_S3
  //#define BLUETOOTH_GAMEPAD
  #define USB_JOYSTICK

  #define SERIAL_COOMUNICATION_TASK_DELAY_IN_MS 5
  #define ESPNow_Pairing_function
  #define Hardware_Pairing_button
  #define Pairing_GPIO 33
  //#define ESPNow_debug_rudder
  #define CONTROLLER_SPECIFIC_VIDPID
  #define USING_LED
  #define LED_GPIO 12
#endif
