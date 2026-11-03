/**
 **************************************************
 *
 * @file        04c_Hysteresis_Helper_Function.ino
 * @brief       Calibrates the light range, then switches an LED on two thresholds, so light sitting right on the switching
 *              point cannot make it flicker.
 *              For details, connection diagram and more, check out the example documentation at: <link placeholder>
 * @author      Borna Zelenika @ soldered.com
 ***************************************************
 */

const int LDR_PIN = 4;
const int LED_PIN = 38;

// Calibration results, in raw ADC counts.
int lightMin = 4095;
int lightMax = 0;

// Thresholds, as a percentage of the calibrated range.
const int THRESHOLD_PCT = 50;
const int HYSTERESIS_PCT = 8;

bool ledOn = false;

void setup() {
  Serial.begin(115200);
  delay(1000);

  analogReadResolution(12);
  analogSetPinAttenuation(LDR_PIN, ADC_11db);

  pinMode(LED_PIN, OUTPUT);

  // Ten seconds of hand-waving, keeping the lowest and highest readings seen. A percentage of this sensor's range in
  // this room is honest; converting to lux would be a fiction, because a photoresistor is not linear in light level.
  Serial.println();
  Serial.println("CALIBRATING for 10 seconds. Cover the sensor, then light it up.");

  unsigned long start = millis();
  while (millis() - start < 10000) {
    int raw = analogRead(LDR_PIN);
    if (raw < lightMin) {
      lightMin = raw;
    }
    if (raw > lightMax) {
      lightMax = raw;
    }
    delay(5);
  }

  // A range of zero would make every later division fail.
  if (lightMax - lightMin < 10) {
    lightMax = lightMin + 10;
  }

  Serial.print("Range: ");
  Serial.print(lightMin);
  Serial.print(" to ");
  Serial.println(lightMax);
  Serial.println();
}

// Switching on takes more light than switching off, so noise cannot move the output back. Same mechanism as the
// relay in T30 and the thermostat in T33.
void loop() {
  int pct = lightPercent();

  int onAbove = THRESHOLD_PCT + HYSTERESIS_PCT;
  int offBelow = THRESHOLD_PCT - HYSTERESIS_PCT;

  if (!ledOn && pct > onAbove) {
    ledOn = true;
  } else if (ledOn && pct < offBelow) {
    ledOn = false;
  }

  digitalWrite(LED_PIN, ledOn);

  Serial.print(pct);
  Serial.print(" ");
  Serial.println(ledOn ? 100 : 0);
  delay(50);
}

// Turns a reading into a percentage of the range you calibrated, from 0 (darkest) to 100 (brightest).
int lightPercent() {
  int raw = analogRead(LDR_PIN);
  int pct = map(raw, lightMin, lightMax, 0, 100);
  return constrain(pct, 0, 100);
}
