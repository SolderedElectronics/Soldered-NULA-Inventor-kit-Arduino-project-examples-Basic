/**
 **************************************************
 *
 * @file        05d_Colour_Cycle.ino
 * @brief       Cycles the onboard RGB LED round the colour wheel by itself, with the pot setting how fast.
 *              For details, connection diagram and more, check out the example documentation at: <link placeholder>
 * @author      Borna Zelenika @ soldered.com
 ***************************************************
 */

// The pot's wiper, same pin as T10 used.
const int POT_PIN = 4;

// Fixed so the pot changes one thing only: how fast the colour moves. A quarter brightness is plenty.
const float SATURATION = 1.0;
const float BRIGHTNESS = 0.25;

// Where we are on the colour wheel, in degrees, and when we last moved it.
float hue = 0;
unsigned long lastMs = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);

  analogReadResolution(12);
  analogSetPinAttenuation(POT_PIN, ADC_11db);

  lastMs = millis();

  Serial.println("Colour cycle: speed on IO4.");
}

void loop() {
  // The knob sets a speed from 0 to 360 degrees per second, so a full turn of the knob is one lap of the wheel
  // every second.
  float degreesPerSecond = (analogRead(POT_PIN) / 4095.0) * 360.0;

  // Move the hue by speed times the time that has passed, so the speed stays the same however long the loop takes.
  unsigned long now = millis();
  float elapsedSeconds = (now - lastMs) / 1000.0;
  lastMs = now;

  hue = hue + degreesPerSecond * elapsedSeconds;

  // Wrap round at the end of the wheel.
  if (hue >= 360.0) {
    hue = hue - 360.0;
  }

  uint8_t r, g, b;
  hsvToRgb(hue, SATURATION, BRIGHTNESS, &r, &g, &b);
  rgbLedWrite(RGB_BUILTIN, r, g, b);

  delay(20);
}

// Turns a hue, a saturation and a brightness into the three levels the LED wants. The same function as in the
// T17 section above.
void hsvToRgb(float h, float s, float v, uint8_t *r, uint8_t *g, uint8_t *b) {
  float c = v * s;
  float x = c * (1.0 - fabs(fmod(h / 60.0, 2.0) - 1.0));
  float m = v - c;

  float rf, gf, bf;

  if (h < 60)       { rf = c; gf = x; bf = 0; }
  else if (h < 120) { rf = x; gf = c; bf = 0; }
  else if (h < 180) { rf = 0; gf = c; bf = x; }
  else if (h < 240) { rf = 0; gf = x; bf = c; }
  else if (h < 300) { rf = x; gf = 0; bf = c; }
  else              { rf = c; gf = 0; bf = x; }

  *r = (uint8_t)((rf + m) * 255);
  *g = (uint8_t)((gf + m) * 255);
  *b = (uint8_t)((bf + m) * 255);
}
