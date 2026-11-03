/**
 **************************************************
 *
 * @file        02d_Bouncing_Light.ino
 * @brief       A lit spot that travels along five LEDs on IO38 to IO42 and turns round at each end, so it bounces
 *              from side to side. Uses the same wiring as the running light.
 *              For details, connection diagram and more, check out the example documentation at: <link placeholder>
 * @author      Borna Zelenika @ soldered.com
 ***************************************************
 */

// The five LEDs, left to right as they sit on the breadboard, each with its own 330 Ohm resistor.
const int LED_PINS[5] = {38, 39, 40, 41, 42};
const int LED_COUNT = 5;

// How long the spot stays on each LED, in milliseconds. Shorter than the running light, so the bounce looks smooth.
const int STEP_MS = 150;

// Which LED is lit, and which way the spot is heading: 1 is to the right, -1 is to the left.
int position = 0;
int direction = 1;

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("BOUNCE: a lit spot travels along the row and turns round at each end.");

  // Every one of the five pins is an output.
  for (int i = 0; i < LED_COUNT; i++) {
    pinMode(LED_PINS[i], OUTPUT);
  }
}

void loop() {
  // Light only the LED at the current position.
  for (int i = 0; i < LED_COUNT; i++) {
    digitalWrite(LED_PINS[i], i == position ? HIGH : LOW);
  }
  delay(STEP_MS);

  // Take one step in the current direction.
  position = position + direction;

  // On reaching either end, turn round, so the next step goes back the way the spot came.
  if (position == 0 || position == LED_COUNT - 1) {
    direction = -direction;
  }
}
