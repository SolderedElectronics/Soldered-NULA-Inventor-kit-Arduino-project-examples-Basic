/**
 **************************************************
 *
 * @file        5.2_Relay_Switching_With_Hysteresis_And_Lockout.ino
 * @brief       Switches a Qwiic relay from a pot. A hysteresis band keeps a noisy input from chattering the
 *              contacts, and a minimum switching interval keeps a request made during the lockout instead of
 *              dropping it.
 *              For details, connection diagram and more, check out the example documentation at: <link placeholder>
 * @author      GreenyHub
 ***************************************************
 */

#include <Wire.h>

// From the Library Manager, not this repository. Search for "Soldered Library for Relays", by Soldered.
#include "Relays-SOLDERED.h"

/*
The relay is on the Qwiic bus, the pot is on IO4, and the LED on IO38 shows what the sketch wants the relay to do,
which is not always what the relay is allowed to do.

The module's address is set by a switch on the module and can be anything from 0x30 to 0x37. Run the scanner from
example 5.1 if 0x30 gives you nothing.
*/
const uint8_t RELAY_ADDRESS = 0x30;

const int POT_PIN = 4;
const int LED_PIN = 38;

/*
  MODE_CLICK       switch it on and off once a second. Listen, and watch the settle time.
  MODE_HYSTERESIS  a band either side of the threshold, so a noisy input cannot chatter the contacts.
  MODE_LOCKOUT     a band, plus a minimum interval between switches.
*/
#define MODE_CLICK      0
#define MODE_HYSTERESIS 1
#define MODE_LOCKOUT    2

const int MODE = MODE_LOCKOUT;

// Threshold and band, in raw ADC counts out of 4095.
const int THRESHOLD = 2048;
const int BAND = 300;

// The shortest time the relay is allowed to stay in one state, in milliseconds.
const unsigned long LOCKOUT_MS = 5000;

// No address on this line: this library takes it in begin() instead.
CH_Relay Relay;

// requestedState is what the input says the relay should be doing; actualState is what it is actually doing. The
// distinction between them is the point of the second half of this page.
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

void loop() {
  switch (MODE) {
    case MODE_CLICK:
      clickMode();
      break;
    case MODE_HYSTERESIS:
      hysteresisOnly();
      break;
    case MODE_LOCKOUT:
      hysteresisWithLockout();
      break;
  }
}

// Everything driven so far settled in microseconds. This is a physical armature pulled across by a coil, and it
// takes five to ten milliseconds to get there and stop bouncing.
void clickMode() {
  Relay.relayControl(CHANNEL1, HIGH);
  digitalWrite(LED_PIN, HIGH);
  Serial.println("on");
  delay(1000);

  Relay.relayControl(CHANNEL1, LOW);
  digitalWrite(LED_PIN, LOW);
  Serial.println("off");
  delay(1000);
}

// The same band as 2.3, somewhere getting it wrong is audible. Relay contacts are rated in operations, in the tens
// or hundreds of thousands, and chatter spends that budget in minutes.
void hysteresisOnly() {
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

/*
The band handles noise. It does nothing about a genuine change arriving too soon after the last one, and for a relay
driving a compressor or a motor, switching back on straight away is worse than switching too often.

A request made during the lockout is not thrown away. It sits in requestedState and the relay moves to it the moment
the interval is up. Dropping it instead would leave the relay in whatever state happened to be current when the
timer ran out. The same pattern comes back in 7.1 and 7.4.
*/
void hysteresisWithLockout() {
  int pot = analogRead(POT_PIN);

  if (!requestedState && pot > THRESHOLD + BAND) {
    requestedState = true;
  } else if (requestedState && pot < THRESHOLD - BAND) {
    requestedState = false;
  }

  digitalWrite(LED_PIN, requestedState);

  unsigned long sinceSwitch = millis() - lastSwitchMs;
  unsigned long remaining = (sinceSwitch < LOCKOUT_MS) ? (LOCKOUT_MS - sinceSwitch) : 0;

  if (requestedState != actualState && remaining == 0) {
    actualState = requestedState;
    lastSwitchMs = millis();
    Relay.relayControl(CHANNEL1, actualState ? HIGH : LOW);
  }

  Serial.print("want ");
  Serial.print(requestedState ? "on " : "off");
  Serial.print("  relay ");
  Serial.print(actualState ? "on " : "off");
  Serial.print("  lockout ");
  Serial.print(remaining / 1000.0, 1);
  Serial.println(" s");

  delay(200);
}

// Nothing on the breadboard drives this coil and there is no flyback diode to fit: both are on the module, with the
// chip that listens on the bus. Compare 4.3, where the transistor and the diode were yours to get right.
