/**
 **************************************************
 *
 * @file        04e_Knob_Sets_Threshold.ino
 * @brief       Calibrates the light range for ten seconds, then switches an LED on the photoresistor with two
 *              thresholds, where the middle of the two is set by a potentiometer.
 *              For details, connection diagram and more, check out the example documentation at: <link placeholder>
 * @author      Borna Zelenika @ soldered.com
 ***************************************************
 */

const int LDR_PIN = 4;
const int POT_PIN = 5;
const int LED_PIN = 38;

// How far above and below the knob's setting the light has to move before the LED changes.
const int HYSTERESIS_PCT = 8;

// Calibration results for the photoresistor, in raw ADC counts.
int lightMin = 4095;
int lightMax = 0;

bool ledOn = false;

// Rescales a raw reading from the range low to high onto 0 to 100, and keeps it inside that range.
int toPercent(int raw, int low, int high) {
  return constrain(map(raw, low, high, 0, 100), 0, 100);
}

// Ten seconds of watching the photoresistor, keeping the lowest and highest readings seen.
void calibrate() {
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

void setup() {
  Serial.begin(115200);
  delay(1000);

  analogReadResolution(12);
  analogSetPinAttenuation(LDR_PIN, ADC_11db);
  analogSetPinAttenuation(POT_PIN, ADC_11db);

  pinMode(LED_PIN, OUTPUT);

  calibrate();
}

void loop() {
  // The light as a percentage of the calibrated range, and the knob as a percentage of its whole turn.
  int light = toPercent(analogRead(LDR_PIN), lightMin, lightMax);
  int threshold = toPercent(analogRead(POT_PIN), 0, 4095);

  // The same two-threshold rule as T15, centred on wherever the knob is set.
  if (!ledOn && light > threshold + HYSTERESIS_PCT) {
    ledOn = true;
  } else if (ledOn && light < threshold - HYSTERESIS_PCT) {
    ledOn = false;
  }

  digitalWrite(LED_PIN, ledOn);

  // Three traces for the Serial Plotter: the light, the knob's setting and the LED.
  Serial.print(light);
  Serial.print(" ");
  Serial.print(threshold);
  Serial.print(" ");
  Serial.println(ledOn ? 100 : 0);

  delay(50);
}
