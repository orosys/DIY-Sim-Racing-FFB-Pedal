#include "RP2040PicoUART.h"

RP2040PicoUART::RP2040PicoUART(int rxPin, int txPin, int handshakePin, int baud)
{
    SerialExt= new HardwareSerial(2);
    SerialExt->begin(baud,SERIAL_8N1,rxPin,txPin);
    pinMode(handshakePin, INPUT_PULLDOWN);
    _txPin=txPin;
    _rxPin=rxPin;
    _baud=baud;
    _handshakePin=handshakePin;
}


void RP2040PicoUART::UARTSendPacket(uint8_t* data, size_t len)
{
    size_t avail = SerialExt->availableForWrite();
    if (avail >= len) 
    {
        if(digitalRead(_handshakePin)==HIGH)
        {   
            SerialExt->write(data, len);
        }
        
    } 
    else 
    {
        Serial.println("TX buffer full, rp2040 off, restart Serial?");
        SerialExt->end();
        delay(10000);
        SerialExt->begin(_baud, SERIAL_8N1, _rxPin, _txPin);  // RX=16, TX=15
    }
}

void RP2040PicoUART::UARTrestart()
{
    SerialExt->end();
    delay(10000);
    SerialExt->begin(_baud, SERIAL_8N1, _rxPin, _txPin);
}