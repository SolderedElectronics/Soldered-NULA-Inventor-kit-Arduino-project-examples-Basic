/**
 **************************************************
 *
 * @file        06b_Eased_Night_Light.ino
 * @brief       A night light that eases towards the brightness the room asks for, so it takes a few seconds to fade rather than following your hand.
 *              For details, connection diagram and more, check out the example documentation at: <link placeholder>
 * @author      Borna Zelenika @ soldered.com
 ***************************************************
 */

// The divider junction, and the LED. Both are where they were in T18, so nothing needs rewiring.
const int LDR_PIN = 4;
const int LED_PIN = 38;

// 5 kHz is far above what the eye can follow, and 8 bits gives 256 brightness steps.
const int PWM_FREQ = 5000;
const int PWM_RESOLUTION = 8;

// Smooths the reading, as in T18.
const float EMA_ALPHA = 0.05;
float ema = 0;

// How far the brightness may move on each pass of the loop. At 1 step every 20 ms, a full fade takes about 5 s.
const int STEP_PER_PASS = 1;

// The brightness the LED is actually showing right now, from 0 to 255.
int shown = 0;

// Calibration results, in raw ADC counts.
int lightMin = 4095;
int lightMax = 0;
const int CALIBRATION_MS = 10000;

// The gamma table from the T18 section above.
uint8_t gammaTable[256];

void setup() {
  Serial.begin(115200);
  delay(1000);

  analogReadResolution(12);
  analogSetPinAttenuation(LDR_PIN, ADC_11db);

  buildGammaTable();
  ledcAttach(LED_PIN, PWM_FREQ, PWM_RESOLUTION);
  runCalibration();

  // The first reading starts the average, or it would crawl up from zero for the first second.
  ema = analogRead(LDR_PIN);
}

void loop() {
  ema = ema + EMA_ALPHA * (analogRead(LDR_PIN) - ema);

  // Darker room means a higher brightness, so the two ends of the map are the other way round.
  int wanted = map(constrain((int)ema, lightMin, lightMax), lightMin, lightMax, 255, 0);

  // Instead of jumping to what the room asks for, take one small step towards it.
  shown = approach(shown, wanted, STEP_PER_PASS);

  ledcWrite(LED_PIN, gammaTable[shown]);

  Serial.print("wanted ");
  Serial.print(wanted);
  Serial.print("  shown ");
  Serial.println(shown);

  delay(20);
}

// Moves current towards target by at most step, and never goes past it.
int approach(int current, int target, int step) {
  if (current < target) {
    return min(current + step, target);
  }
  if (current > target) {
    return max(current - step, target);
  }
  return current;
}

// Records the darkest and brightest readings this sensor sees over ten seconds, as in T18.
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

  // A range of nearly nothing would make map() divide by zero, so widen it to a sensible minimum.
  if (lightMax - lightMin < 100) {
    Serial.println("Range is too narrow to use. Cover the sensor and light it during the ten seconds.");
    lightMax = lightMin + 100;
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
