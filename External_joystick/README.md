# DAP Joystick UART Output Format

This document describes the UART output data structure used for communication between the FFB Pedal Wireless Bridge system and external devices. The data is transmitted as a packed binary struct, ensuring compact and predictable layout. 

## Firmware Flashing for External Joystick Support

To enable external joystick inject support, flash the wireless bridge with the **`Bridge_with_external_joystick`** firmware tag.

## Packet Overview

The UART packet consists of two parts:

1. **`payloadjoystick`** – Main payload containing control and status information.  
2. **`JoystickPayloadFooter`** – Footer used for checksum validation.

These are wrapped in the composite structure `DAP_JoystickUART_State`.

---

## UART Packet Structure

### `struct payloadjoystick`

```c
struct __attribute__((packed)) payloadjoystick {
    uint8_t payloadtype;             // Identifies the type of payload, set to 215, serves as an alignment marker
    uint8_t key;                     // current key is set to 0x97, serves as an alignment marker
    uint8_t DAP_JOY_Version;         // Payload protocol version, current version set to 0x01
    int16_t controllerValue_i32[3];  // Joystick output value, max is set to 10000
    int8_t pedal_status;             // Pedal mode (see enum below)
    uint8_t pedalAvailability[3];    // Availability status of 3 pedals (0 = not available, 1 = available)
    uint8_t JoystickAction;          // Bitfield representing joystick actions, not use for now.
};
```

### `struct JoystickPayloadFooter`

```c
struct __attribute__((packed)) JoystickPayloadFooter {
    uint16_t checkSum;               // 16-bit checksum for data integrity
};
```

### `struct DAP_JoystickUART_State`

```c
struct __attribute__((packed)) DAP_JoystickUART_State {
    payloadjoystick _payloadjoystick;
    JoystickPayloadFooter _payloadfooter;
};
```

---

## Enum: Pedal Status

The `pedal_status` field indicates which pedal mode is currently active:

```c
enum Pedal_status {
    Pedal_status_Pedal = 0,        // Normal pedal
    Pedal_status_Rudder = 1,       // Rudder mode
    Pedal_status_RudderBrake = 2   // Rudder + Brake mode
};
```

---

## Integration Notes

- This structure is transmitted via **UART as a raw binary stream**.
- All structures are marked `__attribute__((packed))` to prevent compiler-inserted padding.
- Make sure the receiver parses with the same structure and byte alignment.
- A checksum is included at the end of each packet to verify data integrity.
- You should calculate the checksum of the `payloadjoystick` part and compare it with the received `checkSum`.

---

## Checksum

The checksum is a `uint16_t` value appended at the end of each packet. It should be calculated using the same algorithm on both sender and receiver. The common method is to sum all bytes (excluding the checksum itself) or use CRC16 if needed.
### Checksum Calculation

To ensure data integrity, a 16-bit checksum is appended to the end of each UART packet.  
The checksum is computed using a simple **Fletcher-like algorithm** over the payload data.

### Checksum Function

```c
uint16_t checksumCalculator(uint8_t *data, uint16_t length)
{
    uint16_t curr_crc = 0x0000;
    uint8_t sum1 = (uint8_t)curr_crc;
    uint8_t sum2 = (uint8_t)(curr_crc >> 8);
    int index;

    for (index = 0; index < length; index++)
    {
        sum1 = (sum1 + data[index]) % 255;
        sum2 = (sum2 + sum1) % 255;
    }

    return (sum2 << 8) | sum1;
}
```
---

## Example Use Case

- ESP32 or RP2040 MCU sends this struct over UART to a PC or host device.
- Host device reads fixed-length binary stream.
- Host validates `checkSum`.
- Host parses control values and pedal status for further processing (e.g., input emulation, telemetry, etc.).
- Baud is set to 921600
- handshake is required


## UART Connection Table

| ESP32-S3 Pin | Direction | RP2040 Pin     | ANY MCU Pin     | Description                                              |
|--------------|-----------|----------------|------------------|----------------------------------------------------------|
| GPIO15       | TX        | GPIO9          | RX               | ESP32-S3 transmits UART data to the other device         |
| GPIO16       | RX        | GPIO8          | TX               | ESP32-S3 receives UART data from the other device        |
| GPIO17       | Input     | GPIO7          | Any Digital Out  | Handshake pin. Drive HIGH (3.3V) to signal MCU is ready  |
| GND          | GND       | GND            | GND              | Make sure to connect all GND (ground) lines together and ensure that all devices operate at the same voltage level.|
### Handshake Pin Behavior (GPIO17)

- **GPIO17** on ESP32-S3 is used as a **handshake input**.
- The companion device (RP2040 or any MCU) should drive this pin **HIGH (3.3V)** after it is initialized and ready.
- ESP32-S3 will monitor this pin to determine when to begin UART communication.

> ⚠️ All signals must use **3.3V logic level** to avoid damaging ESP32-S3 or other devices.

# You can find the example code in RP2040_joystick folder.

