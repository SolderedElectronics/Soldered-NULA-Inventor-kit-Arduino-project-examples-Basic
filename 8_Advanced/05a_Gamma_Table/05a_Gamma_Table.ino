/**
 **************************************************
 *
 * @file        05a_Gamma_Table.ino
 * @brief       Ramps an LED linearly and then through a gamma table so you can compare them back to back.
 *              For details, connection diagram and more, check out the example documentation at: <link placeholder>
 * @author      Borna Zelenika @ soldered.com
 ***************************************************
 */

// The LED on IO38 needs a 330 Ohm resistor in series or it will be damaged.
const int LED_PIN = 38;

// 5 kHz is far above what the eye can follow, and 8 bits gives 256 brightness steps.
const int PWM_FREQ = 5000;
const int PWM_RESOLUTION = 8;

// Vision is roughly logarithmic, so a linear duty ramp spends nearly all its visible change in the bottom twenty
// counts and then appears to stall. Raising the input to the power of 2.2 undoes that.
uint8_t gamma8[256];

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Hardware PWM: once running, a timer in the chip generates the waveform and the processor is free.
  // ledcWrite() only changes the duty cycle, it does not do the switching.
  ledcAttach(LED_PIN, PWM_FREQ, PWM_RESOLUTION);

  // Fill the gamma table once, so the loop only has to look the values up.
  for (int i = 0; i < 256; i++) {
    gamma8[i] = (uint8_t)(pow((float)i / 255.0, 2.2) * 255.0 + 0.5);
  }

  Serial.println("FADE mode: linear ramp, then gamma-corrected ramp. Watch the LED, not the numbers.");
}

void loop() {
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
