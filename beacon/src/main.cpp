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
#define RFM95_CS 8
#define RFM95_RST 4
#define RFM95_INT 3
#define RF95_FREQ 433.0
#define LED 13

#define STATE_IDLE 0
#define STATE_PING 1
#define STATE_WAIT 2
uint8_t activeState;

uint8_t tid = 0xFF;
uint8_t bid = 0xFF;
int16_t rssi = 0x7FFF;
unsigned long waittime = 0xFFFFFFFFUL;
bool ledactive = true;

RH_RF95 rf95(RFM95_CS, RFM95_INT);
RHReliableDatagram rf95_manager(rf95, ADDRESS);
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

void toggle()
{
    ledactive = !ledactive;
    if(ledactive) digitalWrite(LED, HIGH);
    else          digitalWrite(LED, LOW);
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

    // Setup serial1 and serial2.
    Serial1.begin(115200);
    Serial2.begin(115200);
    pinPeripheral(10, PIO_SERCOM);
    pinPeripheral(11, PIO_SERCOM);

    // Depending on which ADDRESS we are, we expect a different amount of bytes.
    if(ADDRESS == 0xFB)      expectedBytes = 3*7;
    else if(ADDRESS == 0xFC) expectedBytes = 2*7;
    else if(ADDRESS == 0xFD) expectedBytes = 1*7;
    else if(ADDRESS == 0xFE) expectedBytes = 0*7;

    // Set the active state to the idle state;
    activeState = STATE_IDLE;
}

#define BUF_SIZE (4*7 + 1)
uint8_t buf[BUF_SIZE]; // A buffer enough for our application.
void loop()
{
    // If we're in the idle state, we spin in a tight loop until we have 2 bytes on serial1.
    if(activeState == STATE_IDLE)
    {
        toggle();
        if(Serial1.available() >= 2)
        {
            tid = Serial1.read();
            bid = Serial1.read();
            Serial1.write(0xAA);
            activeState = STATE_PING;
        }
    }

    // If we're in the ping state, we ping!
    if(activeState == STATE_PING)
    {
        bool success = false;
        uint8_t attempts = 3;
        while(!success && attempts > 0)
        {
            // Attempt a ping.
            toggle();
            waittime = micros();
            success = rf95_manager.sendtoWait(0, 0, tid);
            waittime = micros() - waittime;
            rssi = rf95.lastRssi();
            if(waittime > 600000UL) return;
            --attempts;
        }

        if(success)
        {
            if(bid == 3)
            {
                buf[0] = bid;
                buf[1] = (rssi >> 0) & 0xFF;
                buf[2] = (rssi >> 8) & 0xFF;
                buf[3] = (waittime >> 0)  & 0xFF;
                buf[4] = (waittime >> 8)  & 0xFF;
                buf[5] = (waittime >> 16) & 0xFF;
                buf[6] = (waittime >> 24) & 0xFF;
                Serial1.write(buf, 7);
            }
            else
            {
                Serial2.write(tid);
                Serial2.write(bid+1);
                activeState = STATE_WAIT;
            }
        } 
        else 
        { 
            Serial1.write(0x55);
            activeState = STATE_IDLE;
            return;
        }
    }

    // If we're in the wait state, we simply wait for the data to come back.
    if(activeState == STATE_WAIT)
    {
        // Wait for ack.
        toggle();
        while(Serial2.available() == 0);

        if(Serial2.read() == 0x55)
        {
            Serial1.write(0x55);
            activeState = STATE_IDLE;
            return;
        }

        // Wait for data.
        toggle();
        while(Serial2.available() < expectedBytes);
        Serial2.readBytes(buf, BUF_SIZE);
        while(Serial2.available()) Serial2.read();
        buf[expectedBytes + 0] = bid;
        buf[expectedBytes + 1] = (rssi >> 0) & 0xFF;
        buf[expectedBytes + 2] = (rssi >> 8) & 0xFF;
        buf[expectedBytes + 3] = (waittime >> 0)  & 0xFF;
        buf[expectedBytes + 4] = (waittime >> 8)  & 0xFF;
        buf[expectedBytes + 5] = (waittime >> 16) & 0xFF;
        buf[expectedBytes + 6] = (waittime >> 24) & 0xFF;
        toggle();
        Serial1.write(buf, expectedBytes + 7);
        activeState = STATE_IDLE;
        return;
    }
}