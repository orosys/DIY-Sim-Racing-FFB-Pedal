#include <Joystick.h>
#include "main.h"
#include "pico/bootrom.h"
#include "checksumcalculator.h"
#include "uart.h"
#include "hardware/watchdog.h"
#define DEBUGOUT_INTERVAL 100

DAP_JoystickUART_State dap_joystickUART_st_local;
unsigned long runtime_last=0;
unsigned long debug_message_last=0;
void restart_UART();
void printOutRaw();

void setup()
{
  delay(10000);
  Serial.begin(921600);
  Serial2.begin(baud);
  Joystick.begin();
  Joystick.useManualSend(true);
  Joystick.X(0);
  Joystick.Y(0);
  Joystick.Z(0);
  Joystick.sliderLeft(0);
  Joystick.sliderRight(0);
  Joystick.Zrotate(0);
  Serial.println("starting GPIO initializing");
  pinMode(handshakeGPIO,OUTPUT);
  delay(10);
  pinMode(LED_PIN, OUTPUT);
  delay(10);
  digitalWrite(handshakeGPIO, HIGH);
  delay(100);
  digitalWrite(LED_PIN, HIGH);
  Serial.println("GPIO initialized");
  //DEBUG_OUTPUT_b = true;
}

unsigned long temp=0;
unsigned long UART_last=0;
int uart_index=0;
uint8_t uart_buffer[sizeof(DAP_JoystickUART_State)];
uint8_t uart_buffer_raw[sizeof(DAP_JoystickUART_State)];

void loop() 
{
  while (Serial2.available())
  {
    uint8_t incoming = Serial2.read();

    
    if (uart_index == 0 && incoming != DAP_PAYLOAD_TYPE_JOYSTICKUART)
      continue;
    if (uart_index == 1 && incoming != DAP_JOY_KEY)
    {
      uart_index = 0;
      continue;
    }
    uart_buffer[uart_index++] = incoming;

    
    if (uart_index >= sizeof(DAP_JoystickUART_State))
    {
      memcpy(&dap_joystickUART_st_local, uart_buffer, sizeof(DAP_JoystickUART_State));
      memcpy(uart_buffer_raw, uart_buffer, sizeof(DAP_JoystickUART_State));
      uart_index = 0; 


      if (dap_joystickUART_st_local._payloadjoystick.payloadtype == DAP_PAYLOAD_TYPE_JOYSTICKUART)
      {
        if (dap_joystickUART_st_local._payloadjoystick.DAP_JOY_Version== DAP_JOY_VERSION)
        {
          uint16_t crc = checksumCalculator((uint8_t *)&(dap_joystickUART_st_local._payloadjoystick), sizeof(dap_joystickUART_st_local._payloadjoystick));

          if (crc == dap_joystickUART_st_local._payloadfooter.checkSum)
          {
            // check action
            if (dap_joystickUART_st_local._payloadjoystick.JoystickAction == JOYSTICKACTION_DEBUG_MODE)
            {
              if (DEBUG_OUTPUT_b)
              {
                DEBUG_OUTPUT_b = false;
              }
              else
              {
                DEBUG_OUTPUT_b = true;
              }
            }

            if (dap_joystickUART_st_local._payloadjoystick.JoystickAction == JOYSTICKACTION_RESET_INTO_BOOTLOADER)
            {
              RESET_BOOTLOADER_b = true;
            }
            int16_t controller_reading[3] = {0, 0, 0};
            for (int i = 0; i < 3; i++)
            {
              controller_reading[i] = constrain(map(dap_joystickUART_st_local._payloadjoystick.controllerValue_i32[i], 0, JOYSTICK_VALUE_MAX, 0, 1023), 0, 1023);
            }

            if (dap_joystickUART_st_local._payloadjoystick.pedal_status == Pedal_status_Pedal)
            {
              Joystick.Zrotate(controller_reading[0]);     // clutch
              Joystick.sliderLeft(controller_reading[1]);  // brake
              Joystick.sliderRight(controller_reading[2]); // throttle
            }
            else if (dap_joystickUART_st_local._payloadjoystick.pedal_status == Pedal_status_Rudder)
            {
              Joystick.Zrotate(0);     // clutch
              Joystick.sliderLeft(0);  // brake
              Joystick.sliderRight(0); // throttle
              // 3% deadzone
              if (controller_reading[2] < ((int16_t)(0.47 * JOYSTICK_RANGE_LOCAL)) || controller_reading[2] > ((int16_t)(0.53 * JOYSTICK_RANGE_LOCAL)))
              {
                Joystick.Z(JOYSTICK_RANGE_LOCAL - controller_reading[2]);
              }
              else
              {
                Joystick.Z(0.5 * JOYSTICK_RANGE_LOCAL);
              }
            }
            else if (dap_joystickUART_st_local._payloadjoystick.pedal_status == Pedal_status_RudderBrake)
            {
              Joystick.Zrotate(0);     // clutch
              Joystick.sliderLeft(0);  // brake
              Joystick.sliderRight(0); // throttle
              Joystick.Z((int16_t)(0.5 * JOYSTICK_RANGE_LOCAL));// keeo rudder in center
              // int16_t filter_brake=0;
              // int16_t filter_throttle=0;
              if (dap_joystickUART_st_local._payloadjoystick.pedalAvailability[0] == 1)
              {
                Joystick.sliderLeft(controller_reading[0]);
                Joystick.sliderRight(controller_reading[2]);
              }
              else
              {
                Joystick.sliderLeft(controller_reading[1]);
                Joystick.sliderRight(controller_reading[2]);
              }
            }

            Joystick.send_now();
            UART_last = millis();
          }
          else
          {
            Serial.print(millis());
            Serial.println(":CRC Error");
            Serial.print("Raw: ");
            for (int i = 0; i < sizeof(DAP_JoystickUART_State); i++)
            {
              Serial.print(uart_buffer_raw[i], HEX);
              Serial.print(" ");
            }
            Serial.println();
            uart_index = 0; // clear buffer, and bytes alignment
          }
        }
        else
        {
          Serial.print(millis());
          Serial.print(": Verison Error, expected version: ");
          Serial.print(DAP_JOY_VERSION);
          Serial.print("get verison: ");
          Serial.println(dap_joystickUART_st_local._payloadjoystick.DAP_JOY_Version);
          printOutRaw();
          uart_index = 0; // clear buffer, and bytes alignment
        }
            }
      else
      {
        Serial.print(millis());
        Serial.print(": Payload Type Error, Get:");
        Serial.print(dap_joystickUART_st_local._payloadjoystick.payloadtype);
        Serial.print(", Expect:");
        Serial.println(DAP_PAYLOAD_TYPE_JOYSTICKUART);
        printOutRaw();
        uart_index = 0; //clear buffer, and bytes alignment
      }
    }
  }

  // UART timeout check
  if (millis() - UART_last > UART_TIMEOUT_IN_MS)
  {
    UART_last = millis();
    Serial.print(millis());
    Serial.println(": UART timeout");
    restart_UART();
  }

  // bootloader mode
  if (BOOTSEL||RESET_BOOTLOADER_b)
  {
    RESET_BOOTLOADER_b=false;
    reset_usb_boot(0, 0);
  }
  //debug out
  if(DEBUG_OUTPUT_b)
  {
    temp = millis() - runtime_last;
    runtime_last = millis();
    if (millis() - debug_message_last > DEBUGOUT_INTERVAL)
    {
      Serial.print(millis());
      Serial.print(":runtime interval: ");
      Serial.print(temp);
      Serial.println(" ms");
      debug_message_last = millis();
      printOutRaw();
    }

  }

}

void restart_UART()
{
  Serial2.end();
  digitalWrite(handshakeGPIO, LOW);
  delay(3000);
  Serial2.begin(baud);
  digitalWrite(handshakeGPIO, HIGH);
}

void printOutRaw()
{
  Serial.print(millis());
  Serial.print(":Raw: ");
  for (int i = 0; i < sizeof(DAP_JoystickUART_State); i++)
  {
    Serial.print(uart_buffer_raw[i], HEX);
    Serial.print(" ");
  }
  Serial.println();
}
