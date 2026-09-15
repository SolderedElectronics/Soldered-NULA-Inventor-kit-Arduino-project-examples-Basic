/**
 **************************************************
 *
 * @file        5.1_Scanning_The_Bus_And_Reading_Temperature.ino
 * @brief       Finds out what is really on the I2C bus, then reads temperature from the TMP117 continuously and
 *              checks every reading is a real one.
 *              For details, connection diagram and more, check out the example documentation at: <link placeholder>
 * @author      GreenyHub
 ***************************************************
 */

#include <Wire.h>

// From the Library Manager, not this repository. If the header cannot be found, open
// Sketch -> Include Library -> Manage Libraries and search for "Soldered TMP117", by Soldered.
#include "Soldered-TMP117.h"

// The Qwiic bus is IO8 and IO9 on this board and is what a bare Wire.begin() picks, so there is no pin to define.
// A module is reached by its address instead.
const uint8_t TMP117_ADDRESS = 0x49;

/*
  MODE_SCAN  list every address that answers. Pull the Qwiic cable while it runs and watch the list shrink.
  MODE_READ  read temperature twice a second, checking each reading before printing it.
*/
#define MODE_SCAN 0
#define MODE_READ 1

const int MODE = MODE_SCAN;

// The address is passed in because JP2 on the breakout ties ADD0 high, putting the sensor at 0x49.
Soldered_TMP117 tmp(TMP117_ADDRESS);

void setup() {
  Serial.begin(115200);
  delay(1000);

  Wire.begin();

  if (MODE != MODE_SCAN) {
    // Continuous conversion, with a new-data callback or nullptr for none. Nothing here checks whether it worked,
    // which is the problem MODE_READ is about.
    tmp.init(nullptr);
  }

  Serial.println();
}

void loop() {
  switch (MODE) {
    case MODE_SCAN:
      scanBus();
      break;
    case MODE_READ:
      readTemperature();
      break;
  }
}

/*
Every I2C exchange starts with the controller calling out an address, and a device that recognises it answers with
one acknowledge bit. Calling out all 126 in turn is all a scanner does, and it is the first thing to reach for when
a Qwiic project stops working.

The probe below writes one byte. The textbook scanner writes nothing at all, but on this board that reports success
for about half the bus, a different half each pass. One real byte is a real transaction and the acknowledge is the
truth. Probing with a read is cleaner still but misses devices that never answer reads, such as the relay in 5.2.

Two addresses answer here and only one is yours: 0x51 is the real-time clock on the board itself. Pull the Qwiic
cable while the scan runs and 0x49 disappears from the next block while 0x51 stays.
*/
void scanBus() {
  Serial.println("scanning I2C bus...");

  int found = 0;
  for (uint8_t address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    Wire.write((uint8_t)0x00);

    if (Wire.endTransmission() == 0) {
      Serial.print("  0x");
      if (address < 16) {
        Serial.print("0");
      }
      Serial.print(address, HEX);

      if (address == TMP117_ADDRESS) {
        Serial.print("  TMP117");
      } else if (address >= 0x30 && address <= 0x37) {
        Serial.print("  relay");
      } else if (address == 0x51) {
        Serial.print("  onboard RTC, part of the board");
      }
      Serial.println();
      found++;
    }
  }

  if (found == 0) {
    Serial.println("  nothing answered. Check the cable and that a module is plugged in.");
  }

  Serial.println();
  delay(3000);
}

/*
Pull the Qwiic cable out while this runs. The readings do not stop and do not turn into an error: they become
0.0000, at the same rate as before, from a sensor lying on the table. The library asks the bus for two bytes, gets
nothing, and returns the zero it started with, and 0.0000 is a number a cold room could plausibly be at.

The check is cheap. The TMP117's device ID register always reads 0x117, and if it does not, whatever
getTemperature() just said is not a measurement.
*/
void readTemperature() {
  double celsius = tmp.getTemperature();
  uint16_t id = tmp.getDeviceID();

  Serial.print(celsius, 4);
  Serial.print(" C");

  if (id != 0x117) {
    Serial.print("   <-- device ID reads 0x");
    Serial.print(id, HEX);
    Serial.print(", not 0x117. Nothing is answering, so this number is meaningless.");
  }

  Serial.println();
  delay(500);
}

/*
setAveraging() is deliberately left alone here. The sensor reports in steps of 0.0078 C and already sits near that
step with no averaging, so there is little noise to remove, while 64 samples stretches one reading to most of a
second. Averaging earns its keep on a noisy input, which is what the ADC in 2.2 was.

For 6.2: a Qwiic cable carries 3V3, GND, SDA and SCL and nothing else. The TMP117's alert output is not among them,
so that example needs one ordinary jumper wire as well as the cable.
*/
