/**
 **************************************************
 *
 * @file        09d_Advanced_Thermostat.ino
 * @brief       Switches a Qwiic relay from the TMP117 temperature, like a thermostat. Every reading is checked before it
 *              is used, a hysteresis band keeps the relay from chattering, and a lockout limits how often it may switch.
 *              For details, connection diagram and more, check out the example documentation at: <link placeholder>
 * @author      Borna Zelenika @ soldered.com
 ***************************************************
 */

#include <Wire.h>

// From the Library Manager. Search for "Soldered TMP117" and "Soldered Library for Relays", both by Soldered.
#include "Soldered-TMP117.h"
#include "Relays-SOLDERED.h"

// Both modules share the Qwiic bus and are told apart by their addresses, which are the ones from T29 and T30.
const uint8_t TMP117_ADDRESS = 0x49;
const uint8_t RELAY_ADDRESS = 0x30;

// The LED on IO38 shows what the program wants the relay to do, as it did in T30.
const int LED_PIN = 38;

// The relay switches on above SET_POINT + HALF_BAND and off below SET_POINT - HALF_BAND, in degrees Celsius.
const double SET_POINT = 25.0;
const double HALF_BAND = 0.5;

// The shortest time the relay is allowed to stay in one state, in milliseconds.
const unsigned long LOCKOUT_MS = 10000;

Soldered_TMP117 tmp(TMP117_ADDRESS);
CH_Relay Relay;

bool requestedState = false;
bool actualState = false;
unsigned long lastSwitchMs = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);

  Wire.begin();
  tmp.init(nullptr);
  Relay.begin(RELAY_ADDRESS);

  pinMode(LED_PIN, OUTPUT);

  Serial.println();
}

/*
The reading is checked first, exactly as in T29. If the device ID is wrong, the temperature is not a measurement, so
the program leaves the relay where it is and tries again instead of acting on a number it cannot trust.
*/
void loop() {
  double celsius = tmp.getTemperature();
  uint16_t id = tmp.getDeviceID();

  if (id != 0x117) {
    Serial.println("device ID reads wrong, so the temperature is not trusted. Check the Qwiic cable.");
    delay(500);
    return;
  }

  // The hysteresis band from T30, with degrees in place of ADC counts.
  if (!requestedState && celsius > SET_POINT + HALF_BAND) {
    requestedState = true;
  } else if (requestedState && celsius < SET_POINT - HALF_BAND) {
    requestedState = false;
  }

  digitalWrite(LED_PIN, requestedState);

  // The lockout from T30: the relay only follows the request once it has rested for long enough.
  bool restedLongEnough = (millis() - lastSwitchMs) >= LOCKOUT_MS;
  if (requestedState != actualState && restedLongEnough) {
    actualState = requestedState;
    lastSwitchMs = millis();
    Relay.relayControl(CHANNEL1, actualState ? HIGH : LOW);
  }

  Serial.print(celsius, 2);
  Serial.print(" C  want ");
  Serial.print(requestedState ? "on " : "off");
  Serial.print("  relay ");
  Serial.println(actualState ? "on " : "off");

  delay(500);
}
