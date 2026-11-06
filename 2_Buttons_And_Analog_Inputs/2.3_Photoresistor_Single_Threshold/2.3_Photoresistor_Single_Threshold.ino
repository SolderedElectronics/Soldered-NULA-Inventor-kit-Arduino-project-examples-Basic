/**
 **************************************************
 *
 * @file        2.3_Photoresistor_Single_Threshold.ino
 * @brief       Calibrates the light range, then switches an LED on one threshold, so you can watch it flicker when the light
 *              sits right on the switching point.
 *              For details, connection diagram and more, check out the example documentation at: <link placeholder>
 * @author      Borna Zelenika @ soldered.com
 ***************************************************
 */

const int LDR_PIN = 4;
const int LED_PIN = 38;

// Calibration results, in raw ADC counts.
int lightMin = 4095;
int lightMax = 0;

// The one threshold, as a percentage of the calibrated range.
const int THRESHOLD_PCT = 50;

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

// Hold the light steady right at the boundary. The reading wobbles a few counts, every wobble crosses the line,
// and the LED flickers. Nothing is wrong with the code.
void loop() {
  // Read the light and turn it into a percentage of the range you calibrated, from 0 (darkest) to 100 (brightest).
  int raw = analogRead(LDR_PIN);
  int pct = map(raw, lightMin, lightMax, 0, 100);
  pct = constrain(pct, 0, 100);

  // One number decides everything: above it the LED is on, at or below it the LED is off.
  if (pct > THRESHOLD_PCT) {
    digitalWrite(LED_PIN, HIGH);
    Serial.print(pct);
    Serial.print(" ");
    Serial.println(100);
  } else {
    digitalWrite(LED_PIN, LOW);
    Serial.print(pct);
    Serial.print(" ");
    Serial.println(0);
  }
  delay(50);
}
