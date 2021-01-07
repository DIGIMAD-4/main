// This is used to define the address of all the modules in the system.
// A device will only accept incoming messages if the TO header matches
// the node's ADDRESS. Addresses 0xFB, 0xFC, 0xFD, and 0xFE are assigned
// to the beacons. Trackers use addresses 0x00-0xFA. Address 0xFF is the
// broadcast address and will be accepted by every device.
#define ADDRESS 0x00

#include <SPI.h>
#include <RH_RF95.h>
#include <RHReliableDatagram.h>
#define RFM95_CS 8
#define RFM95_RST 4
#define RFM95_INT 3
#define RF95_FREQ 433.0
#define LED 13

RH_RF95 rf95(RFM95_CS, RFM95_INT);
RHReliableDatagram rf95_manager(rf95, ADDRESS);
bool ledactive = true;

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
    rf95.setModemConfig(RH_RF95::Bw500Cr45Sf128);
    rf95.setTxPower(23, false);
    rf95_manager.setRetries(0);
}

// In a tight loop, ack all packets received.
void loop()
{
    if(rf95_manager.recvfromAck(0, 0)) toggle();
}