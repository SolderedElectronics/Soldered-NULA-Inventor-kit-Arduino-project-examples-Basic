/**
 **************************************************
 *
 * @file        05c_Breathing_LED.ino
 * @brief       Makes the LED on IO38 breathe in and out smoothly, using a gamma table so the breath looks even.
 *              For details, connection diagram and more, check out the example documentation at: <link placeholder>
 * @author      Borna Zelenika @ soldered.com
 ***************************************************
 */

// The LED on IO38 needs a 330 Ohm resistor in series or it will be damaged.
const int LED_PIN = 38;

// 5 kHz is far above what the eye can follow, and 8 bits gives 256 brightness steps.
const int PWM_FREQ = 5000;
const int PWM_RESOLUTION = 8;

// How long one whole breath takes, in milliseconds: up, then back down.
const unsigned long BREATH_MS = 4000;

// The same gamma table as in the T16 section above. It bends an even number into an even-looking brightness.
uint8_t gamma8[256];

void setup() {
  Serial.begin(115200);
  delay(1000);

  ledcAttach(LED_PIN, PWM_FREQ, PWM_RESOLUTION);

  // Fill the gamma table once, so the loop only has to look the values up.
  for (int i = 0; i < 256; i++) {
    gamma8[i] = (uint8_t)(pow((float)i / 255.0, 2.2) * 255.0 + 0.5);
  }

  Serial.println("Breathing LED on IO38.");
}

void loop() {
  // How far through the current breath we are, as an angle from 0 to a full circle (TWO_PI is about 6.28).
  // The remainder (%) makes the time start again from 0 every BREATH_MS.
  float phase = (millis() % BREATH_MS) / (float)BREATH_MS * TWO_PI;

  // sin() swings between -1 and 1. Adding 1 and halving it gives a number that swings smoothly between 0 and 1.
  float level = (sin(phase) + 1.0) / 2.0;

  // Turn 0 to 1 into 0 to 255, bend it through the gamma table and send it to the LED.
  ledcWrite(LED_PIN, gamma8[(int)(level * 255.0)]);

  delay(10);
}
