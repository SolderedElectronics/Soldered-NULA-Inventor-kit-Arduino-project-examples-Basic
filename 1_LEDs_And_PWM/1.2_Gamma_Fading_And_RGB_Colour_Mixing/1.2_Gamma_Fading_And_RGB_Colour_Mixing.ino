/**
 **************************************************
 *
 * @file        1.2_Gamma_Fading_And_RGB_Colour_Mixing.ino
 * @brief       FADE mode ramps an LED linearly and then through a gamma table so you can compare them back to back.
 *              COLOUR mode drives the onboard WS2812B from one pot feeding the hue.
 *              For details, connection diagram and more, check out the example documentation at: <link placeholder>
 * @author      Borna Zelenika @ soldered.com
 ***************************************************
 */

/*
Pick a mode and upload.

  FADE    ramps one LED on IO38 linearly, then through the gamma table, so you can compare them back to back
  COLOUR  reads one pot on IO4 as a hue and drives the RGB LED that is already on the board

The LED on IO38 needs a 330 Ohm resistor in series or it will be damaged.
*/
#define MODE_FADE   0
#define MODE_COLOUR 1

const int MODE = MODE_COLOUR;

const int LED_PIN = 38;

// The pot's wiper, same pin as example 0.2 used.
const int POT_PIN = 4;

// 5 kHz is far above what the eye can follow, and 8 bits gives 256 brightness steps.
const int PWM_FREQ = 5000;
const int PWM_RESOLUTION = 8;

// Fixed so the pot changes one thing only. A quarter brightness is plenty: the onboard LED is uncomfortable
// to look at wide open.
const float SATURATION = 1.0;
const float BRIGHTNESS = 0.25;

// Vision is roughly logarithmic, so a linear duty ramp spends nearly all its visible change in the bottom twenty
// counts and then appears to stall. Raising the input to the power of 2.2 undoes that.
uint8_t gamma8[256];

void setup() {
  Serial.begin(115200);
  delay(1000);

  if (MODE == MODE_FADE) {
    // Hardware PWM: once running, a timer in the chip generates the waveform and the processor is free.
    // ledcWrite() only changes the duty cycle, it does not do the switching.
    ledcAttach(LED_PIN, PWM_FREQ, PWM_RESOLUTION);

    for (int i = 0; i < 256; i++) {
      gamma8[i] = (uint8_t)(pow((float)i / 255.0, 2.2) * 255.0 + 0.5);
    }

    Serial.println("FADE mode: linear ramp, then gamma-corrected ramp. Watch the LED, not the numbers.");
  } else {
    analogReadResolution(12);
    analogSetPinAttenuation(POT_PIN, ADC_11db);

    Serial.println("COLOUR mode: hue on IO4.");
  }
}

void loop() {
  if (MODE == MODE_FADE) {
    fadeMode();
  } else {
    colourMode();
  }
}

void fadeMode() {
  // Even steps of duty. The LED brightens fast and then looks like it stopped, though the duty is still climbing.
  Serial.println("linear");
  for (int duty = 0; duty <= 255; duty++) {
    ledcWrite(LED_PIN, duty);
    delay(8);
  }
  for (int duty = 255; duty >= 0; duty--) {
    ledcWrite(LED_PIN, duty);
    delay(8);
  }

  delay(600);

  // Same step count, same timing, same code. Only the number written to the pin changed.
  Serial.println("gamma");
  for (int i = 0; i <= 255; i++) {
    ledcWrite(LED_PIN, gamma8[i]);
    delay(8);
  }
  for (int i = 255; i >= 0; i--) {
    ledcWrite(LED_PIN, gamma8[i]);
    delay(8);
  }

  delay(600);
}

void colourMode() {
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
// and COLOUR mode then does nothing at all.
