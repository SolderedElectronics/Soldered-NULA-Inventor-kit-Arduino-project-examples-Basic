/**
 **************************************************
 *
 * @file        06e_Self_Calibrating_Night_Light.ino
 * @brief       A night light with no calibration step. It keeps track of the darkest and brightest light it has seen
 *              and lets those limits drift slowly inwards, so a change of room ends up calibrated by itself.
 *              For details, connection diagram and more, check out the example documentation at: <link placeholder>
 * @author      Borna Zelenika @ soldered.com
 ***************************************************
 */

// The divider junction, and the LED. Both are where they were in T13, so nothing needs rewiring.
const int LDR_PIN = 4;
const int LED_PIN = 38;

// 5 kHz is far above what the eye can follow, and 8 bits gives 256 brightness steps.
const int PWM_FREQ = 5000;
const int PWM_RESOLUTION = 8;

// Smooths the reading, as in T13.
const float EMA_ALPHA = 0.05;
float ema = 0;

// The learned range, in raw ADC counts: the lowest and highest smoothed reading seen lately.
int lightMin = 0;
int lightMax = 0;

// Every DECAY_MS each limit moves 1 count towards the current reading. At 2000 ms that is 30 counts a minute, so a
// limit that is 600 counts out of date takes about 20 minutes to come in.
const unsigned long DECAY_MS = 2000;
unsigned long lastDecayMs = 0;

// The two limits never come closer than this. Without a floor, a steady room would shrink the range until the
// smallest flicker swung the LED from dark to full brightness.
const int MIN_SPAN = 200;

// How often the range and the brightness are printed.
const unsigned long PRINT_MS = 1000;
unsigned long lastPrintMs = 0;

// The gamma table from T13.
uint8_t gammaTable[256];

void setup() {
  Serial.begin(115200);
  delay(1000);

  analogReadResolution(12);
  analogSetPinAttenuation(LDR_PIN, ADC_11db);

  buildGammaTable();
  ledcAttach(LED_PIN, PWM_FREQ, PWM_RESOLUTION);

  // The first reading starts the average, and a small range centred on it is the first guess. The range grows
  // as soon as the room gets darker or brighter than that.
  ema = analogRead(LDR_PIN);
  lightMin = (int)ema - MIN_SPAN / 2;
  lightMax = (int)ema + MIN_SPAN / 2;

  Serial.println("ADAPTIVE: no calibration, the range is learned while the night light runs.");
}

void loop() {
  ema = ema + EMA_ALPHA * (analogRead(LDR_PIN) - ema);
  int smoothed = (int)ema;

  // Stretch the range if the room went past it, and let it drift inwards a little if it did not.
  updateRange(smoothed);

  // Darker room means a higher brightness, so the two ends of the map are the other way round, as in T13.
  int brightness = map(constrain(smoothed, lightMin, lightMax), lightMin, lightMax, 255, 0);
  ledcWrite(LED_PIN, gammaTable[brightness]);

  // Print once a second, using the clock instead of delay() so the LED keeps following the light.
  if (millis() - lastPrintMs >= PRINT_MS) {
    lastPrintMs = millis();

    Serial.print("range ");
    Serial.print(lightMin);
    Serial.print(" to ");
    Serial.print(lightMax);
    Serial.print("  brightness ");
    Serial.println(brightness);
  }

  delay(20);
}

// Keeps lightMin and lightMax around the readings of the last few minutes. A reading outside the range widens it at
// once. Otherwise, every DECAY_MS, each limit takes one step towards the current reading.
void updateRange(int reading) {
  if (reading < lightMin) {
    lightMin = reading;
  }
  if (reading > lightMax) {
    lightMax = reading;
  }

  if (millis() - lastDecayMs >= DECAY_MS) {
    lastDecayMs = millis();

    // Only shrink while the range is wider than the floor.
    if (lightMax - lightMin > MIN_SPAN) {
      if (lightMin < reading) {
        lightMin++;
      }
      if (lightMax > reading) {
        lightMax--;
      }
    }
  }
}

// Fills the lookup table once at startup, so the loop never has to do the arithmetic.
void buildGammaTable() {
  for (int i = 0; i < 256; i++) {
    gammaTable[i] = (uint8_t)(pow((float)i / 255.0, 2.2) * 255.0 + 0.5);
  }
}
