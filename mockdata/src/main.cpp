#include <Arduino.h>

uint8_t fakeResponse[26] = {
    0x03,                  // Tracker 3
    0x14,                  // RSSI
    0x00, 0x00, 0xF2, 0x32,// Delay
    0x02,                  // Tracker 2
    0x65,                  // RSSI
    0x00, 0x00, 0xE3, 0x93,// Delay
    0x01,                  // Tracker 1
    0x75,                  // RSSI
    0x00, 0x00, 0xA8, 0x62,// Delay
    0x00,                  // Tracker 0
    0x18,                  // RSSI
    0x00, 0x00, 0xC1, 0x12,// Delay
    0x9C, 0xF2             // XMODEM CRC-16
};

void setup() 
{
    Serial.begin(115200);
}

void loop() 
{
    // If we don't have two waiting bytes, return.
    if(Serial.available() < 2) return;
    
    // Otherwise, we read the tracker ID (that we discard...)
    Serial.read();

    // And we read the beacon ID (that we also discard...)
    Serial.read();

    // Then we respond with 0xAA
    Serial.write(0xAA);

    // Pretend the system is doing the measurement.
    delay(100);

    // Write the data back to the rpi, repeat until a 0xAA ack is received.
    uint8_t response = 0x55;
    while(response != 0xAA)
    {
        Serial.write(fakeResponse, 26);
        while(Serial.available() < 1);
        response = Serial.read();
    }
}