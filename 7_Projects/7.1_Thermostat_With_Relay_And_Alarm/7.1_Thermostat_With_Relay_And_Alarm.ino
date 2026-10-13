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
 * @author      Soldered Electronics
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

The parts came from earlier examples: TMP117 and relay from 5.1 and 5.2, the pot from 2.2, the LED from 0.1, the
buzzer and its transistor from 4.3. The 330 Ohm on the LED is still not optional.
*/
const uint8_t TMP117_ADDRESS = 0x49;
const uint8_t RELAY_ADDRESS = 0x30;

const int POT_PIN = 4;
const int FAN_LED_PIN = 38;
const int BUZZER_PIN = 48;

// The resonant frequency you measured in example 4.3. Yours will not be this one.
const int BUZZER_HZ = 2700;

// Setpoint range the pot maps across, in degrees. Room temperature wants to be somewhere in the middle of it.
const float SETPOINT_MIN_C = 15.0;
const float SETPOINT_MAX_C = 35.0;

/*
How long the room has to stay below the setpoint before the fan may stop. Switching on is immediate; the wait
restarts every time the temperature goes back above, so five continuous seconds is what it takes.

This is a hysteresis band done in time instead of in degrees, so that the number on the pot stays the number: the
fan comes on the instant you pass it. A band would have moved one of those two points somewhere else. Real
extractor fans use a run-on timer for the same reason, and 5.2 builds the band version on this same relay.
*/
const unsigned long OFF_DELAY_MS = 5000;

// 0x49 because JP2 on the breakout ties ADD0 high. A missing module reads 0.0000 rather than failing, so
// getDeviceID() == 0x117 is the only honest test.
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

const unsigned long SENSOR_INTERVAL_MS = 250;
const unsigned long PRINT_INTERVAL_MS = 1000;

/*
The two alarm patterns, deliberately nothing like each other, because two alarms that sound the same are one alarm:

  fan running     one short chirp every two seconds, for as long as the LED is lit
  sensor fault    a continuous warble, on and off every 300 ms

The duty of 64 is the compromise 4.3 found: loudness peaks at half duty, and this carries across a room without
being unbearable next to your hand.
*/
const unsigned long OVER_CHIRP_ON_MS = 120;
const unsigned long OVER_CHIRP_OFF_MS = 1880;
const unsigned long FAULT_PHASE_MS = 300;
const int BUZZER_DUTY = 64;

void setup() {
  Serial.begin(115200);
  delay(1000);

  Wire.begin();

  // No setAveraging() call. Every configuration call writes the sensor's EEPROM, and the TMP117 already sits at its
  // 0.0078 degree step with averaging off, as 5.1 measured.
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
  // Four jobs, each with its own timing, none waiting for any of the others. That is the whole architecture.
  serviceSetpoint();
  serviceSensor();
  serviceControl();
  serviceOutputs();
}

// The pot is read every pass. It is cheap and there is no reason to make the knob feel laggy.
void serviceSetpoint() {
  int pot = analogRead(POT_PIN);
  float target = SETPOINT_MIN_C + (pot / 4095.0) * (SETPOINT_MAX_C - SETPOINT_MIN_C);

  // Light smoothing, the exponential average from example 2.2, so the setpoint does not jitter.
  setpointC = setpointC + 0.05 * (target - setpointC);
}

void serviceSensor() {
  if (millis() - lastSensorMs < SENSOR_INTERVAL_MS) {
    return;
  }
  lastSensorMs = millis();

  // Check before believing, every time. An unplugged TMP117 hands back a plausible 0.0000 that would leave this
  // thermostat certain the room is freezing.
  sensorOk = (tmp.getDeviceID() == 0x117);

  if (sensorOk) {
    temperatureC = tmp.getTemperature();
  }
}

void serviceControl() {
  // A failed read is not a temperature of zero, it is an absence of information. Stop the fan and make a noise.
  if (!sensorOk) {
    setFan(false);
    belowSinceMs = 0;
    return;
  }

  // Above the setpoint the fan runs, no delay and no conditions, and any shutdown counting down is forgotten. That
  // is what makes the five seconds have to be continuous.
  if (temperatureC > setpointC) {
    belowSinceMs = 0;
    setFan(true);
    return;
  }

  // Below it: note when that started and stop the fan only once the room has stayed under for the whole delay.
  if (belowSinceMs == 0) {
    belowSinceMs = millis();
  }

  if (fanOn && millis() - belowSinceMs >= OFF_DELAY_MS) {
    setFan(false);
  }
}

// One place that touches the relay, so the LED, the relay and the printed state can never disagree.
void setFan(bool on) {
  if (on == fanOn) {
    return;
  }
  fanOn = on;
  Relay.relayControl(CHANNEL1, on ? HIGH : LOW);
  digitalWrite(FAN_LED_PIN, on ? HIGH : LOW);
}

void serviceOutputs() {
  serviceBuzzer();

  if (millis() - lastPrintMs >= PRINT_INTERVAL_MS) {
    lastPrintMs = millis();
    printStatus();
  }
}

// The buzzer keeps its own time, which is what lets it beep in a pattern without stopping the relay logic or the
// sensor reads. A delay() here would freeze the whole thermostat for the length of a beep.
void serviceBuzzer() {
  // The fault wins when both are true, because a temperature you cannot trust is more urgent than one you can. The
  // chirp follows the relay, run-on included, so light, click and sound are one state with three ways of showing it.
  bool fault = !sensorOk;
  bool running = sensorOk && fanOn;

  if (!fault && !running) {
    if (buzzerSounding) {
      ledcWrite(BUZZER_PIN, 0);
      buzzerSounding = false;
    }
    // Leave the timestamp stale, so the next alarm starts on its first pass rather than a chirp late.
    buzzerPhaseMs = 0;
    return;
  }

  // How long the current half of the pattern lasts. The chirp is asymmetric, the warble is not.
  unsigned long phaseMs;
  if (fault) {
    phaseMs = FAULT_PHASE_MS;
  } else {
    phaseMs = buzzerSounding ? OVER_CHIRP_ON_MS : OVER_CHIRP_OFF_MS;
  }

  if (millis() - buzzerPhaseMs < phaseMs) {
    return;
  }
  buzzerPhaseMs = millis();

  buzzerSounding = !buzzerSounding;
  ledcWrite(BUZZER_PIN, buzzerSounding ? BUZZER_DUTY : 0);
}

void printStatus() {
  if (!sensorOk) {
    Serial.println("SENSOR FAULT: nothing answering at 0x49. Fan off, alarm sounding.");
    return;
  }

  Serial.print("setpoint ");
  Serial.print(setpointC, 2);
  Serial.print(" C   temperature ");
  Serial.print(temperatureC, 2);
  Serial.print(" C   fan ");
  Serial.print(fanOn ? "ON " : "off");

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

/*
Read serviceControl() again. Two comparisons and one call, and it is a working thermostat. Everything else here is
about not being fooled: a sensor that answers with a device ID before it answers with a temperature, a buzzer for
when it stops answering, and one function that owns the relay.
*/
