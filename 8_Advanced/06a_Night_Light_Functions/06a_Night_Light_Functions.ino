/**
 **************************************************
 *
 * @file        06a_Night_Light_Functions.ino
 * @brief       A night light that fades up as the room gets dark, combining the photoresistor divider from T14 with
 *              the gamma-corrected PWM from T16. The reading is averaged so the light does not twitch.
 *              For details, connection diagram and more, check out the example documentation at: <link placeholder>
 * @author      Borna Zelenika @ soldered.com
 ***************************************************
 */

// The same night light, with the reading averaged and the brightness gamma corrected.

// The divider junction, and the LED. Both are where they were in T14 and T16, so nothing needs rewiring between
// those examples and this one.
const int LDR_PIN = 4;
const int LED_PIN = 38;

// 5 kHz is far above what the eye can follow, and 8 bits gives 256 brightness steps. Same as T16.
const int PWM_FREQ = 5000;
const int PWM_RESOLUTION = 8;

// The exponential moving average from T11. One number, nudged towards each reading, so the light does not flicker
// when a cloud crosses or someone walks past.
const float EMA_ALPHA = 0.05;
float ema = 0;
bool emaStarted = false;

// Calibration results, in raw ADC counts, filled in by runCalibration().
int lightMin = 4095;
int lightMax = 0;

// How long the calibration runs for, in milliseconds.
const int CALIBRATION_MS = 10000;

// Vision is roughly logarithmic, so a linear duty ramp spends nearly all its visible change in the bottom twenty
// counts. Raising the input to the power of 2.2 undoes that. Same table as T16.
uint8_t gammaTable[256];

void setup() {
  Serial.begin(115200);
  delay(1000);

  analogReadResolution(12);
  analogSetPinAttenuation(LDR_PIN, ADC_11db);

  buildGammaTable();

  // ledcAttach returns false if the channel could not be set up, and without it nothing would light while the
  // rest of the sketch carried on looking healthy.
  if (!ledcAttach(LED_PIN, PWM_FREQ, PWM_RESOLUTION)) {
    Serial.println("ledcAttach failed, the LED will not light.");
  }

  runCalibration();
}

void loop() {
  int raw = analogRead(LDR_PIN);

  // Smooth the reading first, so a hand passing over the sensor does not make the light jump.
  if (!emaStarted) {
    ema = raw;
    emaStarted = true;
  }
  ema = ema + EMA_ALPHA * (raw - ema);

  // Darker room means a higher brightness, so the two ends of the map are the other way round.
  int wanted = map(constrain((int)ema, lightMin, lightMax), lightMin, lightMax, 255, 0);

  // The gamma table turns a number that climbs evenly into a brightness that looks like it climbs evenly.
  ledcWrite(LED_PIN, gammaTable[wanted]);

  Serial.print("light ");
  Serial.print((int)ema);
  Serial.print("  wanted ");
  Serial.print(wanted);
  Serial.print("  sent ");
  Serial.println(gammaTable[wanted]);

  delay(20);
}

/*
Records the darkest and brightest readings this sensor sees over ten seconds, so the thresholds afterwards are a
percentage of your room rather than of the room this was written in. Straight out of T15.
*/
void runCalibration() {
  Serial.print("CALIBRATING for ");
  Serial.print(CALIBRATION_MS / 1000);
  Serial.println(" seconds. Cover the sensor, then light it up.");

  unsigned long startMs = millis();
  while (millis() - startMs < (unsigned long)CALIBRATION_MS) {
    int raw = analogRead(LDR_PIN);

    if (raw < lightMin) {
      lightMin = raw;
    }
    if (raw > lightMax) {
      lightMax = raw;
    }

    delay(10);
  }

  // A range of nearly nothing means the sensor never saw both ends, and every reading after this would be noise
  // stretched across the whole brightness scale.
  if (lightMax - lightMin < 100) {
    Serial.println("Range is too narrow to use. Cover the sensor and light it during the ten seconds.");
  }

  Serial.print("Range: ");
  Serial.print(lightMin);
  Serial.print(" to ");
  Serial.println(lightMax);
}

// Fills the lookup table once at startup, so the loop never has to do the arithmetic.
void buildGammaTable() {
  for (int i = 0; i < 256; i++) {
    gammaTable[i] = (uint8_t)(pow((float)i / 255.0, 2.2) * 255.0 + 0.5);
  }
}
