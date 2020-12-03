// This is used to define the address of all the modules in the system.
// A device will only accept incoming messages if the TO header matches
// the node's ADDRESS. Addresses 0xFB, 0xFC, 0xFD, and 0xFE are assigned
// to the beacons. Trackers use addresses 0x00-0xFA. Address 0xFF is the
// broadcast address and will be accepted by every device.
#define ADDRESS 0xFB

#include <Arduino.h>
#include "wiring_private.h"
#include <SPI.h>
#include <RH_RF95.h>
#include <RHReliableDatagram.h>
#include "crc16.h"
#define RFM95_CS 8
#define RFM95_RST 4
#define RFM95_INT 3
#define RF95_FREQ 433.0
#define LED 13

RH_RF95 rf95(RFM95_CS, RFM95_INT);
RHReliableDatagram rf95_manager(rf95, ADDRESS);
Crc16 crc;
uint8_t expectedBytes;

// Second serial port on TX = 11 and RX = 10.
Uart Serial2(&sercom1, 11, 10, SERCOM_RX_PAD_0, UART_TX_PAD_2);
void SERCOM1_Handler()
{
    Serial2.IrqHandler();
}

// On fault, hang and blink rapidly.
void fault()
{
    while (1)
    {
        digitalWrite(LED, HIGH);
        delay(100);
        digitalWrite(LED, LOW);
        delay(100);
    }
}

void setup()
{
    // Set up the radio and the addresses.
    pinMode(LED, OUTPUT);
    pinMode(RFM95_RST, OUTPUT);
    digitalWrite(RFM95_RST, LOW);
    delay(10);
    digitalWrite(RFM95_RST, HIGH);
    delay(10);
    if(!rf95.init()) fault();
    if(!rf95.setFrequency(RF95_FREQ)) fault();
    rf95.setModemConfig(RH_RF95::Bw500Cr45Sf128);
    rf95.setTxPower(23, false);
    rf95_manager.setRetries(0);

    // Setup the crc
    crc.clearCrc();

    // Setup serial1 and serial2.
    Serial1.begin(115200);
    Serial2.begin(115200);
    pinPeripheral(10, PIO_SERCOM);
    pinPeripheral(11, PIO_SERCOM);

    // Depending on which ADDRESS we are, we expect a different amount of bytes.
    if(ADDRESS == 0xFB)      expectedBytes = 3*6 + 2;
    else if(ADDRESS == 0xFC) expectedBytes = 2*6 + 2;
    else if(ADDRESS == 0xFD) expectedBytes = 1*6 + 2;
    else if(ADDRESS == 0xFE) expectedBytes = 0*6 + 2;
}

#define BUF_SIZE (4*6 + 2)
uint8_t buf[BUF_SIZE]; // A buffer enough for our application.
void loop()
{
    int16_t rssi;
    unsigned long waittime;
    uint8_t destination;
    bool success;
    int32_t attempts;
    // TODO: State machine.

    ///////////////////////////////////////////////////////////////////////////
    // Full example of sending a ping:
    success = false;
    attempts = 3;
    while(!success && attempts > 0)
    {
        // Attempt a ping.
        waittime = micros();
        success = rf95_manager.sendToWait(0, 0, destination);
        waittime = micros() - waittime;
        rssi = rf95.lastRssi();

        // This will happen once every approximately 70 minutes, it means that the microsecond counter has overflowed.
        // In this case, we return out of the function, the next loop through will still be in the same section of the
        // state machine and thus the measurement will be redone.
        if(waittime > 600000UL) return;

        // Decrement remaining attempts.
        --attempts;
    }

    if(success)
    {
        // At this point, an accurate measurement of signal strength is in rssi and an accurate measurement of time
        // spent waiting on a response in microseconds is in waittime
    } 
    else 
    { 
        // We tried three times, and failed to get a response each time. Cancel the measurement and return to the raspberry pi
        // that the tracker is offline.
    }
    ///////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////
    // Full example of checking for a correct crc on receipt from port B:
    if(Serial2.peek() == 0x55)
    {
        // Depending on where we are in the state machine, the sent message was
        // either corrupted, or the tracker is offline and we must pass the byte
        // on.
    }
    else if(Serial2.peek() == 0xAA)
    {
        // Successful ACK.
    }
    else if(Serial2.available() >= expectedBytes)
    {
        size_t read = Serial2.readBytes(buf, BUF_SIZE);
        while(Serial2.available()) Serial2.read();
        if(crc.XModemCrc(buf, 0, read-2) != ((buf[read-2] << 8)|(buf[read-1])))
        {
            // Crc doesn't match.
        }
        else
        {
            // Crc matches.
        }
    }
    ///////////////////////////////////////////////////////////////////////////
}