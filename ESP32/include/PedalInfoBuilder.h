#pragma once
#include "Arduino.h"
#include "Main.h"
class PedalInfoBuilder
{
    public:
    char logString[200];
    char logESPNOWString[200];
    void BuildString(uint8_t pedaltype,const char* board, float loadcellShift, float loadcellVariance, float psuVoltage,long maxEndStop, uint32_t currentPosition);
    void LogClear();
    void BuildESPNOWInfo(uint8_t pedalType, int32_t* rssi);
};
PedalInfoBuilder pedalInfoBuilder;

void PedalInfoBuilder::BuildString(uint8_t pedalType,const char* board, float loadcellShift, float loadcellVariance, float psuVoltage,long maxEndStop, uint32_t currentPosition)
{
    snprintf(logString, sizeof(logString),
                "Pedal ID: %d\nBoard: %s\nLoadcell shift= %.3f kg\nLoadcell variance= %.3f kg\nPSU voltage:%.1f V\nMax endstop:%lu\nCurrentPos:%d\0",
                 pedalType, board, loadcellShift, loadcellVariance, psuVoltage,maxEndStop,currentPosition);
}
void PedalInfoBuilder::LogClear()
{
    memset(logString, 0, sizeof(logString));
    memset(logESPNOWString, 0, sizeof(logESPNOWString));
}
void PedalInfoBuilder::BuildESPNOWInfo(uint8_t pedalType, int32_t* rssi)
{
    snprintf(logESPNOWString, sizeof(logESPNOWString),
            "Pedal:%d\nPedal:0 rssi:%ld dbm\nPedal:1 rssi:%ld dbm\nPedal:2 rssi:%ld dbm\nBridge rssi:%ld dbm\n\0",pedalType, rssi[0],rssi[1],rssi[2],rssi[3]);
}

