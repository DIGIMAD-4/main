// This is used to define the address of all the modules in the system.
// A device will only accept incoming messages if the TO header matches
// the node's ADDRESS. Addresses 0xFB, 0xFC, 0xFD, and 0xFE are assigned
// to the beacons. Trackers use addresses 0x00-0xFA. Address 0xFF is the
// broadcast address and will be accepted by every device.
#define ADDRESS 0xFB

#include <SPI.h>
#include <RH_RF95.h>
#define RFM95_CS 8
#define RFM95_RST 4
#define RFM95_INT 3
#define RF95_FREQ 433.0
#define LED 13

RH_RF95 rf95(RFM95_CS, RFM95_INT);

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

// Set up the radio and the addresses.
void setup()
{
    pinMode(LED, OUTPUT);
    pinMode(RFM95_RST, OUTPUT);
    digitalWrite(RFM95_RST, LOW);
    delay(10);
    digitalWrite(RFM95_RST, HIGH);
    delay(10);
    if(!rf95.init()) fault();
    if(!rf95.setFrequency(RF95_FREQ)) fault();
    rf95.setTxPower(23, false);
    rf95.setThisAddress(ADDRESS);
    rf95.setHeaderFrom(ADDRESS);
    // TODO: Set up Serial1 and Serial2.
}

void loop()
{
    // TODO: State machine.
}