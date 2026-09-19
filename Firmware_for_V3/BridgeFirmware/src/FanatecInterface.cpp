// FanatecInterface.cpp

#include "FanatecInterface.h"
#include <EEPROM.h>

#define ADDR_HANDBREAK_MIN 0
#define ADDR_HANDBREAK_MAX 4
#define EEPROM_SIZE 64
const uint8_t patternCalibrationHandbreakMin[] = {0x7B, 0x3, 0x1, 0x3, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x5B, 0x7D};
const uint8_t patternCalibrationHandbreakMax[] = {0x7B, 0x3, 0x1, 0x3, 0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x6C, 0x7D};

uint16_t _valCalibrationHandBreakMin;
uint16_t _valCalibrationHandBreakMax;
uint16_t _lastHandBreak;

bool matchesPattern(const uint8_t* buffer, const uint8_t* pattern, size_t length) {
    return memcmp(buffer, pattern, length) == 0;
}

// Constructor
FanatecInterface::FanatecInterface(int rxPin, int txPin, int plugPin)
    : _rxPin(rxPin), _txPin(txPin), _plugPin(plugPin), _serial(&Serial1),
      _throttle(0), _brake(0), _clutch(0), _handbrake(0),
      _connected(false), _connectedCallback(nullptr), _initialized(false),
      _plugState(false), _lastRawPlugState(false), _plugChangedAt(0),
      _handshakeStep(0), _handshakeMatched(0), _stepStartedAt(0) {
}

// Initialization function
void FanatecInterface::begin() {
    // Initialize serial port
    _serial->begin(250000, SERIAL_8N1, _rxPin, _txPin);
    _lastBaudrate = 250000;
    pinMode(_plugPin, INPUT_PULLDOWN);

    if (!EEPROM.begin(EEPROM_SIZE)) {
        Serial.println("[L] Failed to initialise EEPROM");
        return;
    }

    int readMin, readMax;
    EEPROM.get(ADDR_HANDBREAK_MIN, readMin);
    EEPROM.get(ADDR_HANDBREAK_MAX, readMax);

    _valCalibrationHandBreakMin = readMin;
    _valCalibrationHandBreakMax = readMax;

    // Generate CRC table
    makeCRCTable(0x8C);
    _plugChangedAt = millis();
    _initialized = true;
}

// Communication update function (to be called periodically in the loop)
void FanatecInterface::communicationUpdate() {
    if (!_initialized) return;

    const bool rawPlugState = isPlugged();
    const unsigned long now = millis();
    if (rawPlugState != _lastRawPlugState) {
        _lastRawPlugState = rawPlugState;
        _plugChangedAt = now;
    }
    // Ignore contact bounce, but never advance the handshake while the pin is low.
    if (rawPlugState != _plugState && now - _plugChangedAt >= 30) {
        _plugState = rawPlugState;
        if (!_plugState) {
            resetConnection();
        } else {
            _stepStartedAt = now;
            Serial.println("[L] FANATEC cable detected, waiting for wheelbase.");
        }
    }
    if (!_plugState || !rawPlugState || _connected) return;

    performCommunicationSteps();
}

void FanatecInterface::update() {
    if (isPlugged()) {
        if (isConnected()) {
            uint8_t rxBuffer[48];
            size_t rxIndex = 0;

            // Read expected number of bytes
            while (rxIndex < sizeof(rxBuffer) && _serial->available()) {
                uint8_t receivedByte = _serial->read();
                rxBuffer[rxIndex++] = receivedByte;
            }
            if (rxIndex == sizeof(patternCalibrationHandbreakMin) && matchesPattern(rxBuffer, patternCalibrationHandbreakMin, sizeof(patternCalibrationHandbreakMin))) {
                _valCalibrationHandBreakMin = _lastHandBreak;
                EEPROM.put(ADDR_HANDBREAK_MIN, _lastHandBreak);
                EEPROM.commit();
                Serial.print("[L] HANDBREAK MIN New value saved: ");
                Serial.println(_lastHandBreak);
            } else if (rxIndex == sizeof(patternCalibrationHandbreakMax) && matchesPattern(rxBuffer, patternCalibrationHandbreakMax, sizeof(patternCalibrationHandbreakMax))) {
                _valCalibrationHandBreakMax = _lastHandBreak;
                EEPROM.put(ADDR_HANDBREAK_MAX, _lastHandBreak);
                EEPROM.commit();
                Serial.print("[L] HANDBREAK MAX New value saved: ");
                Serial.println(_lastHandBreak);
            }
            // Create and send pedal data packet
            uint8_t packet[12];
            createPacket(packet);
            _serial->write(packet, 12);
        }
    }
}

bool FanatecInterface::isPlugged() {
    return digitalRead(_plugPin);
}

// Functions to set pedal values
void FanatecInterface::setThrottle(uint16_t value) {
    _throttle = value;
}

void FanatecInterface::setBrake(uint16_t value) {
    _brake = value;
}

void FanatecInterface::setClutch(uint16_t value) {
    _clutch = value;
}

void FanatecInterface::setHandbrake(uint16_t value) {
    _lastHandBreak = value;
    uint16_t newValue = value;
    if (newValue < _valCalibrationHandBreakMin) {
        newValue = _valCalibrationHandBreakMin;
    } else if (newValue > _valCalibrationHandBreakMax) {
        newValue = _valCalibrationHandBreakMax;
    }
    _handbrake = map(newValue, _valCalibrationHandBreakMin, _valCalibrationHandBreakMax, 0, 65535);
}

// Function to set the connection callback
void FanatecInterface::onConnected(void (*callback)(bool)) {
    _connectedCallback = callback;
}

// Check if connected to the Fanatec device
bool FanatecInterface::isConnected() {
    return _connected;
}

// Internal helper functions

void FanatecInterface::performCommunicationSteps() {
    // Define communication steps
    struct Step {
        unsigned long baudRate;
        const uint8_t* rxData;
        size_t rxLength;
        const uint8_t* txData;
        size_t txLength;
    };

    // First step data
    const uint8_t rxData1[] = {0x0A};
    const uint8_t txData1[] = {0x1A};

    // Second step data
    const uint8_t rxData2[] = {0x05};
    const uint8_t txData2[] = {0x15};

    // Third step data (combined message)
    const uint8_t rxData3[] = {
        // First message
        0x7B, 0x02, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x26, 0x7D,
        // Second message
        0x7B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0xAA, 0x7D,
        // Third message
        0x7B, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x5F, 0x7D
    };
    const uint8_t txData3[] = {
        // First message
        0x7B, 0x02, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x26, 0x7D,
        // Second message
        0x7B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0xAA, 0x7D,
        // Third message
        0x7B, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x5F, 0x7D
    };

    Step steps[] = {
        {250000, rxData1, sizeof(rxData1), txData1, sizeof(txData1)},
        {250000, rxData2, sizeof(rxData2), txData2, sizeof(txData2)},
        {115200, rxData3, sizeof(rxData3), txData3, sizeof(txData3)}
    };

    const unsigned long now = millis();
    if (_handshakeStep != 0 && now - _stepStartedAt >= 2000) {
        // Give control back to the task on every call; an absent/unresponsive
        // wheelbase must not trap us in a retry loop or hide an unplug event.
        resetConnection();
        return;
    }

    // Limit work even if a noisy UART continuously receives bytes.
    for (size_t budget = 0; budget < 64 && _serial->available(); ++budget) {
        const uint8_t receivedByte = _serial->read();
        const Step& step = steps[_handshakeStep];
        if (receivedByte == step.rxData[_handshakeMatched]) {
            ++_handshakeMatched;
        } else {
            // Re-sync at a new expected prefix after noise or a damaged byte.
            _handshakeMatched = receivedByte == step.rxData[0] ? 1 : 0;
            if (_handshakeStep == 1 && receivedByte == rxData1[0]) {
                // The wheelbase retried its first request before step 2.
                _serial->write(txData1, sizeof(txData1));
                _stepStartedAt = now;
            }
        }
        if (_handshakeMatched != step.rxLength) continue;

        _serial->write(step.txData, step.txLength);
        Serial.print("[L] FANATEC Send Data step ");
        Serial.println(_handshakeStep);
        _handshakeMatched = 0;
        ++_handshakeStep;
        _stepStartedAt = now;
        if (_handshakeStep == sizeof(steps) / sizeof(steps[0])) {
            _connected = true;
            if (_connectedCallback) _connectedCallback(true);
            return;
        }
        changeBaudRate(steps[_handshakeStep].baudRate);
    }
}

void FanatecInterface::resetConnection() {
    const bool wasConnected = _connected;
    _connected = false;
    _handshakeStep = 0;
    _handshakeMatched = 0;
    // Discard old-session bytes before listening for a fresh 250 kbaud request.
    for (size_t budget = 0; budget < 256 && _serial->available(); ++budget) {
        _serial->read();
    }
    changeBaudRate(250000);
    _stepStartedAt = millis();
    if (wasConnected && _connectedCallback) _connectedCallback(false);
}

void FanatecInterface::changeBaudRate(unsigned long baudrate) {
    if (_lastBaudrate != baudrate) {
        // Complete the response at the old baud rate before switching. Do not
        // flush RX after switching: that could discard the next handshake.
        _serial->flush();
        _serial->updateBaudRate(baudrate);
        _lastBaudrate = baudrate;
    }
}

void FanatecInterface::makeCRCTable(uint8_t poly) {
    for (int i = 0; i < 256; i++) {
        uint8_t crc = i;
        for (int j = 0; j < 8; j++) {
            bool bit = (crc & 0x01) != 0;
            crc >>= 1;
            if (bit) {
                crc ^= poly;
            }
        }
        _crcTable[i] = crc;
    }
}

uint8_t FanatecInterface::generateCRC(uint8_t* input, size_t length) {
    uint8_t crc = 0xFF;
    for (size_t i = 0; i < length; i++) {
        crc = _crcTable[input[i] ^ crc];
    }
    return crc;
}

void FanatecInterface::createPacket(uint8_t* packet) {
    packet[0] = 0x7B; // Start byte
    packet[1] = 0x01; // Command byte (send pedal data)

    // Add pedal data (little-endian)
    packet[2] = _throttle & 0xFF;
    packet[3] = (_throttle >> 8) & 0xFF;

    packet[4] = _brake & 0xFF;
    packet[5] = (_brake >> 8) & 0xFF;

    packet[6] = _clutch & 0xFF;
    packet[7] = (_clutch >> 8) & 0xFF;

    packet[8] = _handbrake & 0xFF;
    packet[9] = (_handbrake >> 8) & 0xFF;

    // Calculate CRC
    uint8_t crc = generateCRC(&packet[1], 9); // Exclude start byte for CRC
    packet[10] = crc;

    packet[11] = 0x7D; // End byte
}
