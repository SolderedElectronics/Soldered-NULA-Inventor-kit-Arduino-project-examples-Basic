/**
 **************************************************
 *
 * @file        11c_Thermostat_Needle_Gauge.ino
 * @brief       A cooling thermostat with a needle gauge. The pot sets the temperature you want, the TMP117
 *              measures the room and the relay switches a fan with a 0.5 degree band either side of your
 *              number. The servo needle shows how far the room is from the setpoint: centre means on target,
 *              left means colder, right means warmer. The onboard LED is red while the fan runs and blue while
 *              it does not. If the sensor stops answering, the fan stops, the LED flashes white and the buzzer
 *              warbles until you press the button on IO39.
 *              For details, connection diagram and more, check out the example documentation at: <link placeholder>
 * @author      Borna Zelenika @ soldered.com
 ***************************************************
 */

#include <Wire.h>

// From the Library Manager. Search for "Soldered TMP117" and "Soldered Library for Relays".
#include "Soldered-TMP117.h"
#include "Relays-SOLDERED.h"

const uint8_t TMP117_ADDRESS = 0x49;
const uint8_t RELAY_ADDRESS = 0x30;

const int POT_PIN = 4;
const int MUTE_PIN = 39;
const int SERVO_PIN = 47;
const int BUZZER_PIN = 48;

// From T22. Yours will be a different number.
const int BUZZER_HZ = 2700;

// From T24. Yours will be different numbers.
const int PULSE_MIN_US = 550;
const int PULSE_MAX_US = 2500;
const int SERVO_FREQ = 50;
const int SERVO_RESOLUTION = 14;
const long PERIOD_US = 20000;

// The pot sets the setpoint somewhere in this range, in degrees.
const float SETPOINT_MIN_C = 15.0;
const float SETPOINT_MAX_C = 35.0;

// The fan comes on this far above the setpoint and goes off this far below it, so a reading wobbling around the
// setpoint cannot make the relay chatter.
const float BAND_C = 0.5;

// The needle sits at one end when the room is this far from the setpoint or more.
const float GAUGE_RANGE_C = 5.0;

const uint8_t LED_BRIGHTNESS = 32;

const unsigned long SENSOR_INTERVAL_MS = 250;
const unsigned long SERVO_INTERVAL_MS = 30;
const unsigned long PRINT_INTERVAL_MS = 1000;
const unsigned long DEBOUNCE_MS = 25;

// How far the needle may move on each 30 ms step, from T25, so it glides instead of jumping.
const int SERVO_STEP_US = 20;

Soldered_TMP117 tmp(TMP117_ADDRESS);
CH_Relay Relay;

/*
A button that debounces itself. The pin number and the three pieces of state it needs are kept together in one
unit, so the mute button is one object instead of a handful of loose variables.
*/
struct Button {
  int pin;
  bool stable = HIGH;
  bool last = HIGH;
  unsigned long changedMs = 0;

  Button(int p) : pin(p) {}

  void begin() {
    pinMode(pin, INPUT_PULLUP);
  }

  // True exactly once for each press. The pull-up means LOW is pressed.
  bool wasPressed() {
    bool reading = digitalRead(pin);
    unsigned long now = millis();

    if (reading != last) {
      last = reading;
      changedMs = now;
    }

    if (now - changedMs > DEBOUNCE_MS && reading != stable) {
      stable = reading;
      return stable == LOW;
    }
    return false;
  }
};

Button muteButton(MUTE_PIN);

float setpointC = 22.0;
float temperatureC = 0;
bool sensorOk = false;
bool fanOn = false;
bool muted = false;

int currentPulseUs = (PULSE_MIN_US + PULSE_MAX_US) / 2;

unsigned long lastSensorMs = 0;
unsigned long lastServoMs = 0;
unsigned long lastPrintMs = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);

  Wire.begin();
  tmp.init(nullptr);

  Relay.begin(RELAY_ADDRESS);
  Relay.relayControl(CHANNEL1, LOW);

  muteButton.begin();

  analogReadResolution(12);
  analogSetPinAttenuation(POT_PIN, ADC_11db);

  ledcAttach(SERVO_PIN, SERVO_FREQ, SERVO_RESOLUTION);
  ledcAttach(BUZZER_PIN, BUZZER_HZ, 8);
  ledcWrite(BUZZER_PIN, 0);

  Serial.println();
  Serial.println("thermostat gauge running: needle in the middle means the room is at your number");
}

void loop() {
  readSetpoint();
  readSensor();
  controlFan();
  moveNeedle();
  showLed();
  soundAlarm();
  report();
}

// Turns a value from one range into a position in another and never goes outside it. Used for both the setpoint
// and the needle, so neither needs its own copy of the arithmetic.
float mapClamped(float x, float inLow, float inHigh, float outLow, float outHigh) {
  float t = (x - inLow) / (inHigh - inLow);
  t = constrain(t, 0.0f, 1.0f);
  return outLow + t * (outHigh - outLow);
}

void readSetpoint() {
  float target = mapClamped(analogRead(POT_PIN), 0, 4095, SETPOINT_MIN_C, SETPOINT_MAX_C);

  // Light smoothing, the smoothing from earlier in the manual, so the setpoint does not jitter.
  setpointC = setpointC + 0.05 * (target - setpointC);
}

void readSensor() {
  if (millis() - lastSensorMs < SENSOR_INTERVAL_MS) {
    return;
  }
  lastSensorMs = millis();

  // Ask before believing: an unplugged TMP117 reads a plausible 0.0000.
  sensorOk = (tmp.getDeviceID() == 0x117);
  if (sensorOk) {
    temperatureC = tmp.getTemperature();
  }
}

void setFan(bool on) {
  if (on == fanOn) {
    return;
  }
  fanOn = on;
  Relay.relayControl(CHANNEL1, on ? HIGH : LOW);
}

void controlFan() {
  if (!sensorOk) {
    setFan(false);
    return;
  }

  // Between the two limits nothing changes, which is exactly what stops the relay from chattering.
  if (temperatureC > setpointC + BAND_C) {
    setFan(true);
  } else if (temperatureC < setpointC - BAND_C) {
    setFan(false);
  }
}

void moveNeedle() {
  if (millis() - lastServoMs < SERVO_INTERVAL_MS) {
    return;
  }
  lastServoMs = millis();

  // With no sensor the needle goes to the middle rather than pointing at an old reading.
  float error = sensorOk ? (temperatureC - setpointC) : 0.0;
  int target = (int)mapClamped(error, -GAUGE_RANGE_C, GAUGE_RANGE_C, PULSE_MIN_US, PULSE_MAX_US);

  int difference = constrain(target - currentPulseUs, -SERVO_STEP_US, SERVO_STEP_US);
  currentPulseUs += difference;

  uint32_t duty = (uint32_t)((float)currentPulseUs * (float)(1UL << SERVO_RESOLUTION) / (float)PERIOD_US);
  ledcWrite(SERVO_PIN, duty);
}

void showLed() {
  if (!sensorOk) {
    // Flashing white cannot be mistaken for a temperature.
    bool on = (millis() / 300) % 2;
    uint8_t level = on ? LED_BRIGHTNESS : 0;
    rgbLedWrite(RGB_BUILTIN, level, level, level);
  } else if (fanOn) {
    rgbLedWrite(RGB_BUILTIN, LED_BRIGHTNESS, 0, 0);
  } else {
    rgbLedWrite(RGB_BUILTIN, 0, 0, LED_BRIGHTNESS);
  }
}

void soundAlarm() {
  // A press silences the alarm, and the next fault sounds again because the mute is forgotten once the sensor
  // answers.
  if (muteButton.wasPressed() && !sensorOk) {
    muted = true;
  }
  if (sensorOk) {
    muted = false;
  }

  bool warble = (millis() / 300) % 2;
  ledcWrite(BUZZER_PIN, (!sensorOk && !muted && warble) ? 128 : 0);
}

void report() {
  if (millis() - lastPrintMs < PRINT_INTERVAL_MS) {
    return;
  }
  lastPrintMs = millis();

  if (!sensorOk) {
    Serial.println("SENSOR FAULT: nothing answering at 0x49. Fan off.");
    return;
  }

  Serial.print("setpoint ");
  Serial.print(setpointC, 2);
  Serial.print(" C   temperature ");
  Serial.print(temperatureC, 2);
  Serial.print(" C   fan ");
  Serial.println(fanOn ? "ON" : "off");
}
