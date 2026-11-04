/**
 **************************************************
 *
 * @file        02g_Knight_Rider_Fade.ino
 * @brief       A bright spot that bounces along five LEDs on IO38 to IO42 and leaves a fading tail behind it.
 *              Each LED is dimmed with hardware PWM, using a table of brightness values by distance from the spot.
 *              For details, connection diagram and more, check out the example documentation at: <link placeholder>
 * @author      Borna Zelenika @ soldered.com
 ***************************************************
 */

// The five LEDs, left to right as they sit on the breadboard, each with its own 330 Ohm resistor.
const int LED_PINS[5] = {38, 39, 40, 41, 42};
const int LED_COUNT = 5;

// 5 kHz is far above what the eye can follow, and 8 bits gives 256 brightness steps (0 to 255).
const int PWM_FREQ = 5000;
const int PWM_RESOLUTION = 8;

// How long the spot stays on each LED, in milliseconds.
const int STEP_MS = 100;

// How many steps the tail is long, counting the spot itself.
const int TAIL_LENGTH = 4;

// The brightness table. Entry 0 is the spot itself, entry 1 is the LED it just left, entry 2 the one before that.
// The values fall by about a third each time, because the eye sees small differences in a dim light more easily.
const uint8_t TAIL_BRIGHTNESS[TAIL_LENGTH] = {255, 90, 28, 6};

// The LEDs the spot has visited, newest first. trail[0] is where the spot is now, trail[1] where it was one step ago.
int trail[TAIL_LENGTH];

// Which way the spot is heading: 1 is to the right, -1 is to the left.
int direction = 1;

// Writes a brightness to every LED. An LED takes the highest value the table gives for any step of the trail
// that sits on it, so an LED the spot has just crossed twice (at a turn) is not dimmed by the older visit.
void showTrail() {
  for (int led = 0; led < LED_COUNT; led++) {
    uint8_t brightness = 0;

    for (int step = 0; step < TAIL_LENGTH; step++) {
      if (trail[step] == led && TAIL_BRIGHTNESS[step] > brightness) {
        brightness = TAIL_BRIGHTNESS[step];
      }
    }

    ledcWrite(LED_PINS[led], brightness);
  }
}

// Moves the spot one LED along, turning it round at either end, and shifts the old positions down the trail.
void moveSpot() {
  // Every old position moves one place back in the trail. The oldest one drops off the end.
  for (int step = TAIL_LENGTH - 1; step > 0; step--) {
    trail[step] = trail[step - 1];
  }

  // Take one step in the current direction. If that would leave the row, turn round first.
  int next = trail[0] + direction;
  if (next < 0 || next >= LED_COUNT) {
    direction = -direction;
    next = trail[0] + direction;
  }

  trail[0] = next;
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("SCANNER: a bright spot with a fading tail bounces along the row.");

  // Hand each pin to a hardware PWM channel, so the processor does not have to switch the LEDs itself.
  for (int led = 0; led < LED_COUNT; led++) {
    ledcAttach(LED_PINS[led], PWM_FREQ, PWM_RESOLUTION);
  }

  // Start with the whole trail on the first LED.
  for (int step = 0; step < TAIL_LENGTH; step++) {
    trail[step] = 0;
  }
}

void loop() {
  // Show the spot and its tail, wait one step, then move the spot on.
  showTrail();
  delay(STEP_MS);
  moveSpot();
}
