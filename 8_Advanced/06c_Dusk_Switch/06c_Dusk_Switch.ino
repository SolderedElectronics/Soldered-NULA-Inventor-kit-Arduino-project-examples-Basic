/**
 **************************************************
 *
 * @file        06c_Dusk_Switch.ino
 * @brief       A night light that switches fully on when the room gets dark and fully off when it is light again, fading both ways.
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

// The light level as a percentage of the calibrated range, 0 for the darkest you showed it and 100 for the
// brightest. The lamp comes on below 35 and goes off only above 50. The gap between the two stops it flickering
// on and off when the room sits right at the edge.
const int ON_BELOW = 35;
const int OFF_ABOVE = 50;

// Whether the lamp is meant to be on, and the brightness it is showing right now.
bool lampOn = false;
int shown = 0;

// How far the brightness may move on each pass: 2 steps every 20 ms is about 2.5 s for a full fade.
const int STEP_PER_PASS = 2;

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

  ema = analogRead(LDR_PIN);
}

void loop() {
  ema = ema + EMA_ALPHA * (analogRead(LDR_PIN) - ema);

  // The light level as a percentage of the room's own range.
  int percent = map(constrain((int)ema, lightMin, lightMax), lightMin, lightMax, 0, 100);

  // Two different limits depending on the current state: this is called hysteresis.
  if (!lampOn && percent < ON_BELOW) {
    lampOn = true;
  } else if (lampOn && percent > OFF_ABOVE) {
    lampOn = false;
  }

  // The lamp is either fully on or fully off, and the fade does the rest.
  int target = lampOn ? 255 : 0;
  shown = approach(shown, target, STEP_PER_PASS);

  ledcWrite(LED_PIN, gammaTable[shown]);

  Serial.print("light ");
  Serial.print(percent);
  Serial.print(" %  lamp ");
  Serial.println(lampOn ? "on" : "off");

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
