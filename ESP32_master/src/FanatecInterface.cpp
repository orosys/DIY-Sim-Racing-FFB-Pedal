// FanatecInterface.cpp

#include "FanatecInterface.h"

#define FANATEC_EVENT_THROTTLE_VIBRATION 0x40
#define FANATEC_EVENT_BREAK_VIBRATION 0x9B
#define FANATEC_EVENT_CALIBRATION_CLUTCH_MIN 0x66
#define FANATEC_EVENT_CALIBRATION_CLUTCH_MAX 0x51
#define FANATEC_EVENT_CALIBRATION_THROTTLE_MIN 0x1C
#define FANATEC_EVENT_CALIBRATION_THROTTLE_MAX 0x2B
#define FANATEC_EVENT_CALIBRATION_BREAK_MIN 0x21
#define FANATEC_EVENT_CALIBRATION_HANDBREAK_MIN 0x5B
#define FANATEC_EVENT_CALIBRATION_HANDBREAK_MAX 0x6C

// Constructor
FanatecInterface::FanatecInterface(int rxPin, int txPin, int plugPin)
    : _rxPin(rxPin), _txPin(txPin), _plugPin(plugPin), _serial(&Serial1),
      _throttle(0), _brake(0), _clutch(0), _handbrake(0),
      _connected(false), _connectedCallback(nullptr), _throttleCallback(nullptr), _breakCallback(nullptr), _initialized(false) {
}

// Initialization function
void FanatecInterface::begin() {
    // Initialize serial port
    _serial->begin(250000, SERIAL_8N1, _rxPin, _txPin);
    pinMode(_plugPin, INPUT_PULLDOWN);

    // Generate CRC table
    makeCRCTable(0x8C);
}

// Communication update function (to be called periodically in the loop)
void FanatecInterface::communicationUpdate() {
    bool detectState = isPlugged();
    if (!_initialized && detectState) {
        performCommunicationSteps();
        if (!_initialized) {
            // Call the connection callback if set
            if (_connectedCallback) {
                _connectedCallback(true);
            }
        }
        _initialized = true;
        _connected = true;
    }
    if (!detectState && _initialized) {
        _initialized = false;
        _connected = false;
        if (_connectedCallback) {
            _connectedCallback(false);
        }
    }
}

void FanatecInterface::update() {
    if (isPlugged()) {
        if (_connected) {
            static bool isThrottleEnabled = false;
            static bool isBreakEnabled = false;
            uint8_t rxBuffer[36];
            size_t rxIndex = 0;
            unsigned long startTime = millis();

            // Read expected number of bytes
            while (_serial->available()) {
                uint8_t receivedByte = _serial->read();
                rxBuffer[rxIndex++] = receivedByte;
            }
            if (rxIndex > 0) {
                if (isThrottleEnabled) {
                    if (_throttleCallback) {
                        _throttleCallback(0);
                    }
                    isThrottleEnabled = false;
                }
                if (isBreakEnabled) {
                    if (_breakCallback) {
                        _breakCallback(0);
                    }
                    isBreakEnabled = false;
                }

                if (rxIndex > 10 && rxBuffer[10] == FANATEC_EVENT_THROTTLE_VIBRATION) {
                    // 0x7B 0x0 0xFF 0x0 0x1 0x0 0x0 0x0 0x0 0x0 0x40 0x7D
                    if (_throttleCallback) {
                        _throttleCallback(rxBuffer[2]);
                    }
                    isThrottleEnabled = true;
                } else if (rxIndex > 10 && rxBuffer[10] == FANATEC_EVENT_BREAK_VIBRATION) {
                    // 0x7B 0x0 0x0 0xFF 0x1 0x0 0x0 0x0 0x0 0x0 0x9B 0x7D
                    if (_breakCallback) {
                        _breakCallback(rxBuffer[3]);
                    }
                    isBreakEnabled = true;
                } else if (rxIndex > 10 && rxBuffer[10] == FANATEC_EVENT_CALIBRATION_CLUTCH_MIN) {
                    Serial.println("[L]FANATEC Calibration Clutch Min ");
                    // 0x7B 0x3 0x1 0x2 0x0 0x0 0x0 0x0 0x0 0x0 0x66 0x7D
                } else if (rxIndex > 10 && rxBuffer[10] == FANATEC_EVENT_CALIBRATION_CLUTCH_MAX) {
                    Serial.println("[L]FANATEC Calibration Clutch Max ");
                    // 0x7B 0x3 0x1 0x2 0x1 0x0 0x0 0x0 0x0 0x0 0x51 0x7D
                } else if (rxIndex > 10 && rxBuffer[10] == FANATEC_EVENT_CALIBRATION_BREAK_MIN) {
                    Serial.println("[L]FANATEC Calibration Break Min ");
                    // 0x7B 0x3 0x1 0x1 0x0 0x0 0x0 0x0 0x0 0x0 0x21 0x7D
                } else if (rxIndex > 10 && rxBuffer[10] == FANATEC_EVENT_CALIBRATION_THROTTLE_MIN) {
                    Serial.println("[L]FANATEC Calibration Throttle Min ");
                    // 0x7B 0x3 0x1 0x0 0x0 0x0 0x0 0x0 0x0 0x0 0x1C 0x7D
                } else if (rxIndex > 10 && rxBuffer[10] == FANATEC_EVENT_CALIBRATION_THROTTLE_MAX) {
                    Serial.println("[L]FANATEC Calibration Throttle Max ");
                    // 0x7B 0x3 0x1 0x0 0x1 0x0 0x0 0x0 0x0 0x0 0x2B 0x7D
                } else if (rxIndex > 10 && rxBuffer[10] == FANATEC_EVENT_CALIBRATION_HANDBREAK_MIN) {
                    Serial.println("[L]FANATEC Calibration Handbreak Min ");
                    // 0x7B 0x3 0x1 0x3 0x0 0x0 0x0 0x0 0x0 0x0 0x5B 0x7D
                } else if (rxIndex > 10 && rxBuffer[10] == FANATEC_EVENT_CALIBRATION_HANDBREAK_MAX) {
                    Serial.println("[L]FANATEC Calibration Handbreak Max ");
                    // 0x7B 0x3 0x1 0x3 0x1 0x0 0x0 0x0 0x0 0x0 0x6C 0x7D
                }
            }
        }
        // Create and send pedal data packet
        uint8_t packet[12];
        createPacket(packet);
        _serial->write(packet, 12);
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
    _handbrake = value;
}

// Function to set the connection callback
void FanatecInterface::onConnected(void (*callback)(bool)) {
    _connectedCallback = callback;
}

void FanatecInterface::onThrottleVibration(void (*callback)(int)) {
    _throttleCallback = callback;
}

void FanatecInterface::onBreakVibration(void (*callback)(int)) {
    _breakCallback = callback;
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
        0x7B, 0x02, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x26, 0x7D,
        // Second message
        0x7B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xAA, 0x7D,
        // Third message
        0x7B, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5F, 0x7D
    };
    const uint8_t txData3[] = {
        // First message
        0x7B, 0x02, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x26, 0x7D,
        // Second message
        0x7B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xAA, 0x7D,
        // Third message
        0x7B, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5F, 0x7D
    };

    Step steps[] = {
        {250000, rxData1, sizeof(rxData1), txData1, sizeof(txData1)},
        {250000, rxData2, sizeof(rxData2), txData2, sizeof(txData2)},
        {115200, rxData3, sizeof(rxData3), txData3, sizeof(txData3)}
    };

    const int numSteps = sizeof(steps) / sizeof(steps[0]);

    int i = 0;
    while (i < numSteps) {
        changeBaudRate(steps[i].baudRate);

        delay(50);

        // Receive buffer
        uint8_t rxBuffer[36];
        size_t rxIndex = 0;
        unsigned long startTime = millis();

        // Read expected number of bytes
        while (rxIndex < steps[i].rxLength && (millis() - startTime) < 2000) {
            if (_serial->available()) {
                uint8_t receivedByte = _serial->read();
                rxBuffer[rxIndex++] = receivedByte;
            }
        }
        
        // Verify received data
        if (rxIndex == steps[i].rxLength && memcmp(rxBuffer, steps[i].rxData, rxIndex) == 0) {
            // Expected data received, send response
            _serial->write(steps[i].txData, steps[i].txLength);
            Serial.print("[L] ");
            Serial.print("FANATEC Send Data step ");
            Serial.print(i);
            Serial.print(": ");
            for (size_t j = 0; j < rxIndex; j++) {
                Serial.print("0x");
                Serial.print(rxBuffer[j], HEX);
                Serial.print(" ");
            }
            Serial.println();
            i++; // Move to next step
        } else {
            if (rxIndex > 0) {
                Serial.print("[L] ");
                Serial.print("FANATEC Received data in step ");
                Serial.print(i);
                Serial.print(": ");
                for (size_t j = 0; j < rxIndex; j++) {
                    Serial.print("0x");
                    Serial.print(rxBuffer[j], HEX);
                    Serial.print(" ");
                }
                Serial.print(" rxIndex ");
                Serial.print(rxIndex);
                Serial.println(" failed. Restarting from step 0.");
            }
            // Failure, restart from step 0
            i = 0;
        }
    }
}

void FanatecInterface::changeBaudRate(unsigned long baudrate) {
    _serial->updateBaudRate(baudrate);
    _serial->flush();
    while (_serial->available()) {
        _serial->read();
    }
    delay(50);
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