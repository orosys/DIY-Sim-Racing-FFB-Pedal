#pragma once
#include <Arduino.h>
#include <stdint.h>
#include <HardwareSerial.h>
#define baud 921600
#define handshakeGPIO 7
#define LED_PIN 25
#define UART_TIMEOUT_IN_MS 5000
bool RESET_BOOTLOADER_b = false;
bool DEBUG_OUTPUT_b= false;

