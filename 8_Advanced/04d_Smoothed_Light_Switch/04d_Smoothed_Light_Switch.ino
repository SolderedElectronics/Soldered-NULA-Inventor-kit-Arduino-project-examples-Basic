/**
 **************************************************
 *
 * @file        04d_Smoothed_Light_Switch.ino
 * @brief       Smooths the photoresistor with a moving average, learns the darkest and brightest light it has
 *              seen, and switches an LED on two thresholds so it cannot flicker.
 *              For details, connection diagram and more, check out the example documentation at: <link placeholder>
 * @author      Borna Zelenika @ soldered.com
 ***************************************************
 */

const int LDR_PIN = 4;
const int LED_PIN = 38;

// The moving average from T11. Smaller means heavier smoothing and a slower response.
const float EMA_ALPHA = 0.1;

// The two thresholds from T15, as a percentage of the range the sketch has seen so far.
const int THRESHOLD_PCT = 50;
const int HYSTERESIS_PCT = 8;

// The range is never allowed to be narrower than this many counts, so the very first readings do not
// get stretched across the whole 0 to 100 scale.
const float MIN_SPAN = 200;

float smooth = 0;    // the smoothed reading
float lightMin = 0;  // the darkest smoothed reading seen so far
float lightMax = 0;  // the brightest smoothed reading seen so far
bool ledOn = false;

// Turns the smoothed reading into a percentage of the range seen so far.
int lightPercent() {
  float span = lightMax - lightMin;
  if (span < MIN_SPAN) {
    span = MIN_SPAN;
  }
  int pct = (int)((smooth - lightMin) * 100.0 / span);
  return constrain(pct, 0, 100);
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  analogReadResolution(12);
  analogSetPinAttenuation(LDR_PIN, ADC_11db);

  pinMode(LED_PIN, OUTPUT);

  // Start the average and the range at the first reading.
  smooth = analogRead(LDR_PIN);
  lightMin = smooth;
  lightMax = smooth;
}

void loop() {
  // Move the average a small step towards the new reading.
  smooth = smooth + EMA_ALPHA * (analogRead(LDR_PIN) - smooth);

  // Widen the range if this reading is darker or brighter than anything seen before.
  if (smooth < lightMin) {
    lightMin = smooth;
  }
  if (smooth > lightMax) {
    lightMax = smooth;
  }

  int pct = lightPercent();

  // Switch on only above the upper number and off only below the lower one. In between, nothing changes.
  if (!ledOn && pct > THRESHOLD_PCT + HYSTERESIS_PCT) {
    ledOn = true;
  } else if (ledOn && pct < THRESHOLD_PCT - HYSTERESIS_PCT) {
    ledOn = false;
  }

  digitalWrite(LED_PIN, ledOn);

  // Two traces for the Serial Plotter: the light as a percentage, and the LED as 100 when on and 0 when off.
  Serial.print(pct);
  Serial.print(" ");
  Serial.println(ledOn ? 100 : 0);

  delay(20);
}
