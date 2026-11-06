/**
 **************************************************
 *
 * @file        2.3_Photoresistor_Hysteresis.ino
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
// relay in T23 and the thermostat in T26.
void loop() {
  // Read the light and turn it into a percentage of the range you calibrated, from 0 (darkest) to 100 (brightest).
  int raw = analogRead(LDR_PIN);
  int pct = map(raw, lightMin, lightMax, 0, 100);
  pct = constrain(pct, 0, 100);

  int onAbove = THRESHOLD_PCT + HYSTERESIS_PCT;
  int offBelow = THRESHOLD_PCT - HYSTERESIS_PCT;

  // Switch on only above the upper number and off only below the lower one. In between, nothing changes.
  if (!ledOn && pct > onAbove) {
    ledOn = true;
    digitalWrite(LED_PIN, HIGH);
  } else if (ledOn && pct < offBelow) {
    ledOn = false;
    digitalWrite(LED_PIN, LOW);
  }

  // The light as a percentage, then the LED as 100 when it is on and 0 when it is off.
  Serial.print(pct);
  Serial.print(" ");
  if (ledOn) {
    Serial.println(100);
  } else {
    Serial.println(0);
  }
  delay(50);
}
