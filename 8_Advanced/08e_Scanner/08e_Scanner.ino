/**
 **************************************************
 *
 * @file        08e_Scanner.ino
 * @brief       Runs a single lit LED back and forth along a row of eight LEDs on a 74HC595, with a pot setting the
 *              speed and output enable PWM setting the brightness.
 *              For details, connection diagram and more, check out the example documentation at: <link placeholder>
 * @author      Borna Zelenika @ soldered.com
 ***************************************************
 */

// The same four control pins as T26 and T27, and the pot on IO4.
const int DATA_PIN = 11;
const int CLOCK_PIN = 12;
const int LATCH_PIN = 10;
const int OE_PIN = 13;
const int POT_PIN = 4;

// How bright the lit LED is, from 0 (off) to 255 (full). A middle value is easy on the eyes.
const int BRIGHTNESS = 100;

// The time the dot spends on each LED, in milliseconds. The knob picks a value between these two.
const int SLOWEST_MS = 400;
const int FASTEST_MS = 40;

int dot = 0;        // which LED is lit, 0 to 7
int direction = 1;  // 1 means moving up the row, -1 means moving back down
unsigned long lastStepMs = 0;

void setup() {
  pinMode(LATCH_PIN, OUTPUT);
  digitalWrite(LATCH_PIN, LOW);

  pinMode(DATA_PIN, OUTPUT);
  pinMode(CLOCK_PIN, OUTPUT);

  analogReadResolution(12);
  analogSetPinAttenuation(POT_PIN, ADC_11db);

  // Output enable is active low, so the duty is inverted: a bigger BRIGHTNESS means a smaller duty.
  ledcAttach(OE_PIN, 5000, 8);
  ledcWrite(OE_PIN, 255 - BRIGHTNESS);
}

void loop() {
  // Turn the knob reading into the time to wait between steps. Knob down is slow, knob up is fast.
  int pot = analogRead(POT_PIN);
  long stepMs = map(pot, 0, 4095, SLOWEST_MS, FASTEST_MS);

  // Check the clock instead of using delay(), so the knob is read many times per step and a change is felt at once.
  if (millis() - lastStepMs >= (unsigned long)stepMs) {
    lastStepMs = millis();

    // Show the dot where it is now, then work out where it goes next.
    writeManual(1 << dot);

    dot += direction;
    if (dot >= 7) {
      direction = -1;
    } else if (dot <= 0) {
      direction = 1;
    }
  }
}

// Sends one byte to the 74HC595, most significant bit first, and then pulses the latch so all eight outputs change
// together.
void writeManual(uint8_t value) {
  for (int bit = 7; bit >= 0; bit--) {
    digitalWrite(DATA_PIN, (value >> bit) & 1);

    digitalWrite(CLOCK_PIN, HIGH);
    digitalWrite(CLOCK_PIN, LOW);
  }

  digitalWrite(LATCH_PIN, HIGH);
  digitalWrite(LATCH_PIN, LOW);
}
