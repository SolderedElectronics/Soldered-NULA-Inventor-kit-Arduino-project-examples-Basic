/**
 **************************************************
 *
 * @file        1.2_Gamma_Fading_And_RGB_Colour_Mixing.ino
 * @brief       Ramps an LED linearly and then with a gamma curve so you can compare them back to back.
 *              For details, connection diagram and more, check out the example documentation at: <link placeholder>
 * @author      Borna Zelenika @ soldered.com
 ***************************************************
 */

// The LED on IO38 needs a 330 Ohm resistor in series or it will be damaged.
const int LED_PIN = 38;

// 5 kHz is far above what the eye can follow, and 8 bits gives 256 brightness steps.
const int PWM_FREQ = 5000;
const int PWM_RESOLUTION = 8;

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Hardware PWM: once running, a timer in the chip generates the waveform and the processor is free.
  // ledcWrite() only changes the duty cycle, it does not do the switching.
  ledcAttach(LED_PIN, PWM_FREQ, PWM_RESOLUTION);

  Serial.println("Fade: linear ramp, then gamma-corrected ramp. Watch the LED, not the numbers.");
}

void loop() {
  // Even steps of duty. The LED brightens fast and then looks like it stopped, though the duty is still climbing.
  Serial.println("linear");

  // Count up from 0 to 255, writing each number straight to the LED.
  for (int duty = 0; duty <= 255; duty++) {
    ledcWrite(LED_PIN, duty);
    delay(8);
  }

  // Count back down from 255 to 0.
  for (int duty = 255; duty >= 0; duty--) {
    ledcWrite(LED_PIN, duty);
    delay(8);
  }

  delay(600);

  // Same step count, same timing. Only the number written to the LED is bent first.
  // Vision is roughly logarithmic, so the linear ramp spends nearly all its visible change in the bottom twenty
  // counts. Raising the step to the power of 2.2 undoes that.
  Serial.println("gamma");

  // Count up from 0 to 255.
  for (int step = 0; step <= 255; step++) {
    // Turn the step into a fraction between 0 and 1, bend it with the power of 2.2, then scale it back to 0-255.
    // The 0.5 rounds the result to the nearest whole number.
    float fraction = step / 255.0;
    float curved = pow(fraction, 2.2);
    int duty = curved * 255.0 + 0.5;

    ledcWrite(LED_PIN, duty);
    delay(8);
  }

  // Count back down from 255 to 0, bending each step the same way.
  for (int step = 255; step >= 0; step--) {
    float fraction = step / 255.0;
    float curved = pow(fraction, 2.2);
    int duty = curved * 255.0 + 0.5;

    ledcWrite(LED_PIN, duty);
    delay(8);
  }

  delay(600);
}
