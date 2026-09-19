#pragma once
struct FakeEEPROM {
    bool begin(int) { return true; }
    template<class T> void get(int address, T& value) { value=address == 0 ? 0 : 4095; }
    template<class T> void put(int, T) {}
    void commit() {}
};
extern FakeEEPROM EEPROM;
