/**
 **************************************************
 *
 * @file        2.3_Photoresistor_Percent.ino
 * @brief       Calibrates the light range over ten seconds, then reports the light as a percentage of that range.
 *              For details, connection diagram and more, check out the example documentation at: <link placeholder>
 * @author      Borna Zelenika @ soldered.com
 ***************************************************
 */

const int LDR_PIN = 4;

// Calibration results, in raw ADC counts.
int lightMin = 4095;
int lightMax = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);

  analogReadResolution(12);
  analogSetPinAttenuation(LDR_PIN, ADC_11db);

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

void loop() {
  // Read the light and turn it into a percentage of the range you calibrated, from 0 (darkest) to 100 (brightest).
  int raw = analogRead(LDR_PIN);
  int pct = map(raw, lightMin, lightMax, 0, 100);
  pct = constrain(pct, 0, 100);

  Serial.println(pct);
  delay(100);
}
