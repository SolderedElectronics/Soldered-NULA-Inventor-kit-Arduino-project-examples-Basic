/**
 **************************************************
 *
 * @file        09a_Scan_Fixed_Width.ino
 * @brief       Scans the I2C bus and prints every address that answers, so you can see what is really connected.
 *              For details, connection diagram and more, check out the example documentation at: <link placeholder>
 * @author      Borna Zelenika @ soldered.com
 ***************************************************
 */

#include <Wire.h>

// The Qwiic bus is IO8 and IO9 on this board and is what a bare Wire.begin() picks, so there is no pin to define.
// A module is reached by its address instead.
const uint8_t TMP117_ADDRESS = 0x49;

void setup() {
  Serial.begin(115200);
  delay(1000);

  Wire.begin();

  Serial.println();
}

/*
Every I2C exchange starts with the controller calling out an address, and a device that recognises it answers with
one acknowledge bit. Calling out all 126 in turn is all a scanner does, and it is the first thing to reach for when
a Qwiic project stops working.

The probe below writes one byte. The textbook scanner writes nothing at all, but on this board that reports success
for about half the bus, a different half each pass. One real byte is a real transaction and the acknowledge is the
truth.

Two addresses answer here and only one is yours: 0x51 is the real-time clock on the board itself. Pull the Qwiic
cable while the scan runs and 0x49 disappears from the next block while 0x51 stays.
*/
void loop() {
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
