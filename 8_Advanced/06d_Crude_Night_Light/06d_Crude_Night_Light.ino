/**
 **************************************************
 *
 * @file        06d_Crude_Night_Light.ino
 * @brief       A night light with no smoothing and no gamma correction, to show what those two corrections fix.
 *              For details, connection diagram and more, check out the example documentation at: <link placeholder>
 * @author      Borna Zelenika @ soldered.com
 ***************************************************
 */

// This is the first program in the kit that does a job rather than demonstrating an idea, and it is made entirely of
// two you have already built. The photoresistor divider is exactly the one from T10, and the PWM output is exactly
// the one from T12.

// The divider junction, and the LED. Both are where they were in T10 and T12, so nothing needs rewiring between
// those examples and this one.
const int LDR_PIN = 4;
const int LED_PIN = 38;

// 5 kHz is far above what the eye can follow, and 8 bits gives 256 brightness steps. Same as T12.
const int PWM_FREQ = 5000;
const int PWM_RESOLUTION = 8;

// Calibration results, in raw ADC counts. They start at the opposite ends so that the first reading replaces both.
int lightMin = 4095;
int lightMax = 0;

// How long the calibration runs for, in milliseconds.
const int CALIBRATION_MS = 10000;

void setup() {
  Serial.begin(115200);
  delay(1000);

  analogReadResolution(12);
  analogSetPinAttenuation(LDR_PIN, ADC_11db);

  // ledcAttach returns false if the channel could not be set up, and without it nothing would light while the
  // rest of the sketch carried on looking healthy.
  if (!ledcAttach(LED_PIN, PWM_FREQ, PWM_RESOLUTION)) {
    Serial.println("ledcAttach failed, the LED will not light.");
  }

  // Calibration. Record the darkest and brightest readings this sensor sees over ten seconds, so the brightness
  // afterwards is a share of your room rather than of the room this was written in. Straight out of T11.
  Serial.print("CALIBRATING for ");
  Serial.print(CALIBRATION_MS / 1000);
  Serial.println(" seconds. Cover the sensor, then light it up.");

  unsigned long startMs = millis();
  while (millis() - startMs < CALIBRATION_MS) {
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

void loop() {
  int raw = analogRead(LDR_PIN);

  // Straight from the reading to the pin, with no smoothing and no gamma. Watch the LED twitch.
  // Darker room means a higher brightness, so the two ends of the map are the other way round.
  int brightness = map(constrain(raw, lightMin, lightMax), lightMin, lightMax, 255, 0);
  ledcWrite(LED_PIN, brightness);

  Serial.print("raw ");
  Serial.print(raw);
  Serial.print("  brightness ");
  Serial.println(brightness);

  delay(20);
}
