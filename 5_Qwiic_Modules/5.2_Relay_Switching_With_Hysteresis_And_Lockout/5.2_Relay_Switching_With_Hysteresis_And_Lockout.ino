/**
 **************************************************
 *
 * @file        5.2_Relay_Switching_With_Hysteresis_And_Lockout.ino
 * @brief       Switches a Qwiic relay from a pot. A hysteresis band keeps a noisy input from chattering the contacts, and a minimum interval between switches protects them from fast requests.
 *              For details, connection diagram and more, check out the example documentation at: <link placeholder>
 * @author      Borna Zelenika @ soldered.com
 ***************************************************
 */

#include <Wire.h>

// From the Library Manager, not this repository. Search for "Soldered Library for Relays", by Soldered.
#include "Relays-SOLDERED.h"

/*
The relay is on the Qwiic bus, the pot is on IO4, and the LED on IO38 shows what the sketch wants the relay to do,
which is not always what the relay is allowed to do.

The module's address is set by a switch on the module and can be anything from 0x30 to 0x37. Run the scanner from
T21 if 0x30 gives you nothing.
*/
const int RELAY_ADDRESS = 0x30;

const int POT_PIN = 4;
const int LED_PIN = 38;

// Threshold and band, in raw ADC counts out of 4095.
const int THRESHOLD = 2048;
const int BAND = 300;

// The shortest time the relay is allowed to stay in one state, in milliseconds.
const unsigned long LOCKOUT_MS = 5000;

// No address on this line: this library takes it in begin() instead.
CH_Relay Relay;

// requestedState is what the knob says the relay should be doing; actualState is what it is actually doing. The
// distinction between them is the point of this task.
bool requestedState = false;
bool actualState = false;
unsigned long lastSwitchMs = 0;

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

/*
The band handles noise. It does nothing about a genuine change arriving too soon after the last one, and for a relay
driving a compressor or a motor, switching back on straight away is worse than switching too often.

A request made during the lockout is not thrown away. It sits in requestedState and the relay moves to it the moment
the interval is up. Dropping it instead would leave the relay in whatever state happened to be current when the
timer ran out. The same pattern comes back in T26 and T29.
*/
void loop() {
  // Read the knob.
  int pot = analogRead(POT_PIN);

  // Work out what the knob is asking for. It only changes its mind once the knob is clearly past the band.
  if (!requestedState && pot > THRESHOLD + BAND) {
    requestedState = true;
  } else if (requestedState && pot < THRESHOLD - BAND) {
    requestedState = false;
  }

  // The LED always shows what was asked for.
  digitalWrite(LED_PIN, requestedState);

  // How much of the lockout is left. It stays 0 once the relay has rested for long enough.
  unsigned long remaining = 0;
  unsigned long sinceSwitch = millis() - lastSwitchMs;
  if (sinceSwitch < LOCKOUT_MS) {
    remaining = LOCKOUT_MS - sinceSwitch;
  }

  // Move the relay only if it is not where the knob wants it and the lockout is over.
  if (requestedState != actualState && remaining == 0) {
    actualState = requestedState;
    lastSwitchMs = millis();

    if (actualState) {
      Relay.relayControl(CHANNEL1, HIGH);
    } else {
      Relay.relayControl(CHANNEL1, LOW);
    }
  }

  // Print what was asked for, what the relay is doing and how long the lockout has left.
  Serial.print("want ");
  if (requestedState) {
    Serial.print("on ");
  } else {
    Serial.print("off");
  }

  Serial.print("  relay ");
  if (actualState) {
    Serial.print("on ");
  } else {
    Serial.print("off");
  }

  Serial.print("  lockout ");
  Serial.print(remaining / 1000.0, 1);
  Serial.println(" s");

  delay(200);
}

// Nothing on the breadboard drives this coil and there is no flyback diode to fit: both are on the module, with the
// chip that listens on the bus. Compare T17, where the transistor and the diode were yours to get right.
