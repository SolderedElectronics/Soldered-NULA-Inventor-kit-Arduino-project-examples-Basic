/**
 **************************************************
 *
 * @file        05b_Colour_Function.ino
 * @brief       Drives the onboard WS2812B from one pot feeding the hue.
 *              For details, connection diagram and more, check out the example documentation at: <link placeholder>
 * @author      Borna Zelenika @ soldered.com
 ***************************************************
 */

// The pot's wiper, same pin as T10 used.
const int POT_PIN = 4;

// Fixed so the pot changes one thing only. A quarter brightness is plenty: the onboard LED is uncomfortable
// to look at wide open.
const float SATURATION = 1.0;
const float BRIGHTNESS = 0.25;

void setup() {
  Serial.begin(115200);
  delay(1000);

  analogReadResolution(12);
  analogSetPinAttenuation(POT_PIN, ADC_11db);

  Serial.println("Colour: hue on IO4.");
}

void loop() {
  int raw = analogRead(POT_PIN);

  // Hue is the colour as an angle: 0 red, 120 green, 240 blue, 360 back to red. One knob walks through every
  // colour there is, which three separate red, green and blue levels could not do.
  float hue = (raw / 4095.0) * 360.0;

  uint8_t r, g, b;
  hsvToRgb(hue, SATURATION, BRIGHTNESS, &r, &g, &b);

  // The LED is a WS2812B: IO2 sends it a serial message rather than a voltage, and a controller inside the
  // package sets the three channels. rgbLedWrite() handles the GRB byte order and the strict timing for you.
  rgbLedWrite(RGB_BUILTIN, r, g, b);

  delay(20);
}

// Turns a hue, a saturation and a brightness into the three levels the LED wants.
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

// Before section 6: opening the JP1 solder jumper disconnects this LED to stop its leakage during deep sleep,
// and this sketch then does nothing at all.
