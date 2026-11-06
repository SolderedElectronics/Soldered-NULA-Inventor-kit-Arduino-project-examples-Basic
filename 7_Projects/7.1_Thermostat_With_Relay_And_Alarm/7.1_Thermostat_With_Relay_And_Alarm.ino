/**
 **************************************************
 *
 * @file        7.1_Thermostat_With_Relay_And_Alarm.ino
 * @brief       A cooling thermostat. The pot sets the temperature you want, the TMP117 measures the room,
 *              and the relay switches a fan on the moment the room goes above your number. It runs on for
 *              five seconds after the room drops back below, so a reading wobbling across the setpoint
 *              cannot chatter the relay. The LED and the buzzer both follow the relay, chirping once every
 *              two seconds for as long as it is closed, and the buzzer warbles continuously instead if the
 *              sensor stops answering.
 *              For details, connection diagram and more, check out the example documentation at: <link placeholder>
 * @author      Borna Zelenika @ soldered.com
 ***************************************************
 */

#include <Wire.h>

// From the Library Manager, not this repository. Search for "Soldered TMP117" and "Soldered Library for Relays".
#include "Soldered-TMP117.h"
#include "Relays-SOLDERED.h"

/*
The whole example in one line: above the number on the pot, the fan runs; five seconds below it, the fan stops.

Cup a hand over the sensor and the relay clicks in straight away. Let go, and once the reading has been back under
your number for five unbroken seconds it clicks out again.

The parts came from earlier tasks: the TMP117 from T22, the relay from T23, the pot from T9, the LED from T2, the
buzzer and its transistor from T17. The 330 Ohm on the LED is still not optional.
*/
const uint8_t TMP117_ADDRESS = 0x49;
const uint8_t RELAY_ADDRESS = 0x30;

const int POT_PIN = 4;
const int FAN_LED_PIN = 38;
const int BUZZER_PIN = 48;

// The resonant frequency you measured in T17. Yours will not be this one.
const int BUZZER_HZ = 2700;

// Setpoint range the pot maps across, in degrees. Room temperature wants to be somewhere in the middle of it.
const float SETPOINT_MIN_C = 15.0;
const float SETPOINT_MAX_C = 35.0;

/*
How long the room has to stay below the setpoint before the fan may stop. Switching on is immediate; the wait
restarts every time the temperature goes back above, so five continuous seconds is what it takes.

This is a hysteresis band done in time instead of in degrees, so that the number on the pot stays the number: the
fan comes on the instant you pass it. A band would have moved one of those two points somewhere else. Real
extractor fans use a run-on timer for the same reason, and T23 builds the band version on this same relay.
*/
const unsigned long OFF_DELAY_MS = 5000;

// How often the sensor is read and how often a status line is printed.
const unsigned long SENSOR_INTERVAL_MS = 250;
const unsigned long PRINT_INTERVAL_MS = 1000;

/*
The two alarm patterns, deliberately nothing like each other, because two alarms that sound the same are one alarm:

  fan running     one short chirp every two seconds, for as long as the LED is lit
  sensor fault    a continuous warble, on and off every 300 ms

The duty of 64 is the compromise T18 found: loudness peaks at half duty, and this carries across a room without
being unbearable next to your hand.
*/
const unsigned long OVER_CHIRP_ON_MS = 120;
const unsigned long OVER_CHIRP_OFF_MS = 1880;
const unsigned long FAULT_PHASE_MS = 300;
const int BUZZER_DUTY = 64;

// 0x49 because JP2 on the breakout ties ADD0 high. A missing module reads 0.0000 rather than failing, so asking for
// getDeviceID() and expecting 0x117 is the only honest test.
Soldered_TMP117 tmp(TMP117_ADDRESS);

// No address on this line: this library takes it in begin() instead.
CH_Relay Relay;

// Nothing here calls delay(), so every part that needs to wait keeps its own deadline and the loop runs thousands
// of times a second regardless of what any one part is doing.
float setpointC = 22.0;
float temperatureC = 0;
bool sensorOk = false;

bool fanOn = false;
unsigned long belowSinceMs = 0;

bool buzzerSounding = false;
unsigned long buzzerPhaseMs = 0;

unsigned long lastSensorMs = 0;
unsigned long lastPrintMs = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);

  Wire.begin();

  // No setAveraging() call. Every configuration call writes the sensor's EEPROM, and the TMP117 already sits at its
  // 0.0078 degree step with averaging off, as T22 measured.
  tmp.init(nullptr);

  Relay.begin(RELAY_ADDRESS);
  Relay.relayControl(CHANNEL1, LOW);

  pinMode(FAN_LED_PIN, OUTPUT);
  digitalWrite(FAN_LED_PIN, LOW);

  analogReadResolution(12);
  analogSetPinAttenuation(POT_PIN, ADC_11db);

  ledcAttach(BUZZER_PIN, BUZZER_HZ, 8);
  ledcWrite(BUZZER_PIN, 0);

  Serial.println();
  Serial.println("cooling thermostat running: above the pot's number the fan runs, below it the fan stops");
}

void loop() {
  // Part 1: read the pot. This happens on every pass, because it is cheap and there is no reason to make the knob
  // feel laggy.
  int pot = analogRead(POT_PIN);
  float target = SETPOINT_MIN_C + (pot / 4095.0) * (SETPOINT_MAX_C - SETPOINT_MIN_C);

  // Light smoothing, the exponential moving average from T13, so the setpoint does not jitter.
  setpointC = setpointC + 0.05 * (target - setpointC);

  // Part 2: read the sensor, but only when its 250 ms are up.
  if (millis() - lastSensorMs >= SENSOR_INTERVAL_MS) {
    lastSensorMs = millis();

    // Check before believing, every time. An unplugged TMP117 hands back a plausible 0.0000 that would leave this
    // thermostat certain the room is freezing.
    sensorOk = (tmp.getDeviceID() == 0x117);

    if (sensorOk) {
      temperatureC = tmp.getTemperature();
    }
  }

  // Part 3: decide whether the fan runs. The relay, the LED and fanOn are always changed together, so the three can
  // never disagree.
  if (!sensorOk) {
    // A failed read is not a temperature of zero, it is an absence of information. Stop the fan and make a noise.
    if (fanOn) {
      fanOn = false;
      Relay.relayControl(CHANNEL1, LOW);
      digitalWrite(FAN_LED_PIN, LOW);
    }
    belowSinceMs = 0;
  } else if (temperatureC > setpointC) {
    // Above the setpoint the fan runs, no delay and no conditions, and any shutdown counting down is forgotten. That
    // is what makes the five seconds have to be continuous.
    belowSinceMs = 0;

    if (!fanOn) {
      fanOn = true;
      Relay.relayControl(CHANNEL1, HIGH);
      digitalWrite(FAN_LED_PIN, HIGH);
    }
  } else {
    // Below it: note when that started and stop the fan only once the room has stayed under for the whole delay.
    if (belowSinceMs == 0) {
      belowSinceMs = millis();
    }

    if (fanOn && millis() - belowSinceMs >= OFF_DELAY_MS) {
      fanOn = false;
      Relay.relayControl(CHANNEL1, LOW);
      digitalWrite(FAN_LED_PIN, LOW);
    }
  }

  // Part 4: the buzzer keeps its own time, which is what lets it beep in a pattern without stopping the relay logic
  // or the sensor reads. A delay() here would freeze the whole thermostat for the length of a beep.
  //
  // The fault wins when both are true, because a temperature you cannot trust is more urgent than one you can. The
  // chirp follows the relay, run-on included, so light, click and sound are one state with three ways of showing it.
  if (!sensorOk) {
    // Sensor fault: a continuous warble, switching every 300 ms.
    if (millis() - buzzerPhaseMs >= FAULT_PHASE_MS) {
      buzzerPhaseMs = millis();

      if (buzzerSounding) {
        buzzerSounding = false;
        ledcWrite(BUZZER_PIN, 0);
      } else {
        buzzerSounding = true;
        ledcWrite(BUZZER_PIN, BUZZER_DUTY);
      }
    }
  } else if (fanOn) {
    // Fan running: one short chirp, then a long wait. The two halves have different lengths.
    if (buzzerSounding && millis() - buzzerPhaseMs >= OVER_CHIRP_ON_MS) {
      buzzerPhaseMs = millis();
      buzzerSounding = false;
      ledcWrite(BUZZER_PIN, 0);
    } else if (!buzzerSounding && millis() - buzzerPhaseMs >= OVER_CHIRP_OFF_MS) {
      buzzerPhaseMs = millis();
      buzzerSounding = true;
      ledcWrite(BUZZER_PIN, BUZZER_DUTY);
    }
  } else {
    // Nothing to report, so the buzzer is silent.
    if (buzzerSounding) {
      ledcWrite(BUZZER_PIN, 0);
      buzzerSounding = false;
    }

    // Leave the timestamp stale, so the next alarm starts on its first pass rather than a chirp late.
    buzzerPhaseMs = 0;
  }

  // Part 5: print one status line a second.
  if (millis() - lastPrintMs >= PRINT_INTERVAL_MS) {
    lastPrintMs = millis();

    if (!sensorOk) {
      Serial.println("SENSOR FAULT: nothing answering at 0x49. Fan off, alarm sounding.");
    } else {
      Serial.print("setpoint ");
      Serial.print(setpointC, 2);
      Serial.print(" C   temperature ");
      Serial.print(temperatureC, 2);
      Serial.print(" C   fan ");
      if (fanOn) {
        Serial.print("ON ");
      } else {
        Serial.print("off");
      }

      // Signed, so the sign alone tells you what the fan will do.
      Serial.print("   ");
      Serial.print(temperatureC - setpointC, 2);
      Serial.print(" C from the switch point");

      if (fanOn && belowSinceMs != 0) {
        unsigned long waited = millis() - belowSinceMs;
        if (waited < OFF_DELAY_MS) {
          Serial.print("   off in ");
          Serial.print((OFF_DELAY_MS - waited) / 1000.0, 1);
          Serial.print(" s");
        }
      }

      Serial.println();
    }
  }
}

/*
Read part 3 of loop() again. Two comparisons and a few lines that switch the relay, and it is a working thermostat.
Everything else here is about not being fooled: a sensor that answers with a device ID before it answers with a
temperature, and a buzzer for when it stops answering.
*/
