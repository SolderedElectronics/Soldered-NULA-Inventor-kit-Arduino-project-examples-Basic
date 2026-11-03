/**
 **************************************************
 *
 * @file        02a_Running_Light_Loop.ino
 * @brief       A lit spot walking along a row of five LEDs on IO38 to IO42. It steps one LED to the right every half
 *              second and starts again at the left after the last one.
 *              For details, connection diagram and more, check out the example documentation at: <link placeholder>
 * @author      Borna Zelenika @ soldered.com
 ***************************************************
 */

// Five LEDs on five pins, left to right as they sit on the breadboard, each with its own 330 Ohm resistor. IO38 to IO42
// are next to each other on the header, none of them has an analog input on it, and nothing on the board uses them.
const int LED_PINS[5] = {38, 39, 40, 41, 42};
const int LED_COUNT = 5;

// How long one step lasts, in milliseconds.
const int STEP_MS = 500;

// Where the lit spot currently is. It has to survive from one pass of loop() to the next, so it lives out here
// rather than inside the function.
int runningPosition = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("RUNNING: a lit spot walks along the row and starts again.");

  // Every one of the five pins is an output.
  for (int i = 0; i < LED_COUNT; i++) {
    pinMode(LED_PINS[i], OUTPUT);
  }
}

void loop() {
  // Everything off, so only the new position is lit and the old one does not have to be cleared by name.
  allOff();

  digitalWrite(LED_PINS[runningPosition], HIGH);
  delay(STEP_MS);

  // Step along, and wrap back to the start after the last LED.
  runningPosition = runningPosition + 1;
  if (runningPosition >= LED_COUNT) {
    runningPosition = 0;
  }
}

// Turns every LED off.
void allOff() {
  for (int i = 0; i < LED_COUNT; i++) {
    digitalWrite(LED_PINS[i], LOW);
  }
}
