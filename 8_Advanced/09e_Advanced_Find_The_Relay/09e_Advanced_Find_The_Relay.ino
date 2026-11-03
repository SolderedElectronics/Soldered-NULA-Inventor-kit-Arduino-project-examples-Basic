/**
 **************************************************
 *
 * @file        09e_Advanced_Find_The_Relay.ino
 * @brief       Looks for the relay on the Qwiic bus instead of assuming its address, then switches it on and off
 *              slowly so you can hear it.
 *              For details, connection diagram and more, check out the example documentation at: <link placeholder>
 * @author      Borna Zelenika @ soldered.com
 ***************************************************
 */

#include <Wire.h>

// From the Library Manager. Search for "Soldered Library for Relays", by Soldered.
#include "Relays-SOLDERED.h"

// The address switch on the module can put the relay anywhere from 0x30 to 0x37.
const uint8_t FIRST_RELAY_ADDRESS = 0x30;
const uint8_t LAST_RELAY_ADDRESS = 0x37;

CH_Relay Relay;

// Stays 0 until the scan finds something.
uint8_t relayAddress = 0;
bool relayOn = false;

// Asks each possible relay address in turn and returns the first one that answers, or 0 if none does.
uint8_t findRelay() {
  for (uint8_t address = FIRST_RELAY_ADDRESS; address <= LAST_RELAY_ADDRESS; address++) {
    Wire.beginTransmission(address);
    Wire.write((uint8_t)0x00);
    if (Wire.endTransmission() == 0) {
      return address;
    }
  }
  return 0;
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Wire.begin();

  relayAddress = findRelay();
  Serial.println();

  if (relayAddress == 0) {
    Serial.println("no relay answered between 0x30 and 0x37. Check the Qwiic cable.");
    return;
  }

  Serial.print("relay found at 0x");
  Serial.println(relayAddress, HEX);

  Relay.begin(relayAddress);
}

/*
If setup() found nothing, relayAddress is still 0 and loop() does nothing at all, so a missing relay is reported once
and then left alone instead of being talked to.
*/
void loop() {
  if (relayAddress == 0) {
    return;
  }

  relayOn = !relayOn;
  Relay.relayControl(CHANNEL1, relayOn ? HIGH : LOW);

  Serial.println(relayOn ? "relay on" : "relay off");

  // Six seconds is far slower than the contacts need, and it keeps the number of operations low.
  delay(6000);
}
