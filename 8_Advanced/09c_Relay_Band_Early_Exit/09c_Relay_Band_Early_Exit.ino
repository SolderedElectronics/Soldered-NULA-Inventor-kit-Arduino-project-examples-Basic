/**
 **************************************************
 *
 * @file        09c_Relay_Band_Early_Exit.ino
 * @brief       Switches a Qwiic relay from a pot, with a hysteresis band but no minimum interval between switches.
 *              For details, connection diagram and more, check out the example documentation at: <link placeholder>
 * @author      Borna Zelenika @ soldered.com
 ***************************************************
 */

#include <Wire.h>

// From the Library Manager, not this repository. Search for "Soldered Library for Relays", by Soldered.
#include "Relays-SOLDERED.h"

/*
The relay is on the Qwiic bus, the pot is on IO4, and the LED on IO38 shows the state the relay was switched to.

The module's address is set by a switch on the module and can be anything from 0x30 to 0x37. Run the scanner from
T28 if 0x30 gives you nothing.
*/
const uint8_t RELAY_ADDRESS = 0x30;

const int POT_PIN = 4;
const int LED_PIN = 38;

// Threshold and band, in raw ADC counts out of 4095.
const int THRESHOLD = 2048;
const int BAND = 300;

// No address on this line: this library takes it in begin() instead.
CH_Relay Relay;

bool actualState = false;

void setup() {
  Serial.begin(115200);
  delay(1000);

  Wire.begin();
  Relay.begin(RELAY_ADDRESS);

  analogReadResolution(12);
  analogSetPinAttenuation(POT_PIN, ADC_11db);

  pinMode(LED_PIN, OUTPUT);

  Serial.println();
}

// The same band as T15, somewhere getting it wrong is audible. Relay contacts are rated in operations, in the tens
// or hundreds of thousands, and chatter spends that budget in minutes.
void loop() {
  int pot = analogRead(POT_PIN);

  if (!actualState && pot > THRESHOLD + BAND) {
    actualState = true;
  } else if (actualState && pot < THRESHOLD - BAND) {
    actualState = false;
  } else {
    delay(20);
    return;
  }

  Relay.relayControl(CHANNEL1, actualState ? HIGH : LOW);
  digitalWrite(LED_PIN, actualState);

  Serial.print("switched to ");
  Serial.print(actualState ? "on" : "off");
  Serial.print(" at ");
  Serial.println(pot);

  delay(20);
}
