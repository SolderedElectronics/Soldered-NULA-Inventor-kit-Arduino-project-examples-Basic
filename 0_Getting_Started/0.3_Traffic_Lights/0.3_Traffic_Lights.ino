/**
 **************************************************
 *
 * @file        0.3_Traffic_Lights.ino
 * @brief       One LED, two LEDs alternating, a running light, a car light and a pedestrian crossing, in five modes
 *              idea each. Every mode uses the same five LEDs on the same five pins, so nothing gets rewired.
 *              For details, connection diagram and more, check out the example documentation at: <link placeholder>
 * @author      Soldered Electronics
 ***************************************************
 */

/*
Work through the modes in order. Each one adds exactly one idea to the one before it.

  MODE_ONE          one LED blinking, the smallest program that does anything visible
  MODE_ALTERNATE    two LEDs taking turns, which is the same program with the second half inverted
  MODE_RUNNING      a lit spot walking along the row, the first program with a position to remember
  MODE_TRAFFIC      a car light cycling red, red-yellow, green, yellow
  MODE_CROSSING     the same light, plus a pedestrian light that is the opposite of it

All five LEDs need their own 330 Ohm resistor in series.
*/
#define MODE_ONE       0
#define MODE_ALTERNATE 1
#define MODE_RUNNING   2
#define MODE_TRAFFIC   3
#define MODE_CROSSING  4

const int MODE = MODE_ALTERNATE;

// Five LEDs on five pins, left to right as they sit on the breadboard. IO38 to IO42 are next to each other on the
// header, none of them has an analog input on it, and nothing on the board uses them.
const int LED_PINS[5] = {38, 39, 40, 41, 42};
const int LED_COUNT = 5;

// Which of the five is which, for the traffic light modes. The car light uses the first three.
#define CAR_RED    0
#define CAR_YELLOW  1
#define CAR_GREEN  2
#define PED_RED    3
#define PED_GREEN  4

// How long one step lasts, in milliseconds. Everything in this sketch is a multiple of it, so changing this one
// number changes the speed of every mode.
const int STEP_MS = 500;

// Where the lit spot currently is, in MODE_RUNNING. It has to survive from one pass of loop() to the next, so it
// lives out here rather than inside the function.
int runningPosition = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Every one of the five pins is an output, whichever mode is selected.
  for (int i = 0; i < LED_COUNT; i++) {
    pinMode(LED_PINS[i], OUTPUT);
    digitalWrite(LED_PINS[i], LOW);
  }

  switch (MODE) {
    case MODE_ONE:
      Serial.println("ONE: the first LED blinks. Everything else stays dark.");
      break;

    case MODE_ALTERNATE:
      Serial.println("ALTERNATE: two LEDs take turns. One is always lit.");
      break;

    case MODE_RUNNING:
      Serial.println("RUNNING: a lit spot walks along the row and starts again.");
      break;

    case MODE_TRAFFIC:
      Serial.println("TRAFFIC: red, red and yellow, green, yellow, and round again.");
      break;

    case MODE_CROSSING:
      Serial.println("CROSSING: the same car light, with a pedestrian light that is its opposite.");
      break;
  }
}

void loop() {
  switch (MODE) {
    case MODE_ONE:
      runOne();
      break;

    case MODE_ALTERNATE:
      runAlternate();
      break;

    case MODE_RUNNING:
      runRunning();
      break;

    case MODE_TRAFFIC:
      runTraffic();
      break;

    case MODE_CROSSING:
      runCrossing();
      break;
  }
}

// One LED on, one LED off, forever. The smallest program in the manual that does something you can see.
void runOne() {
  digitalWrite(LED_PINS[0], HIGH);
  delay(STEP_MS);

  digitalWrite(LED_PINS[0], LOW);
  delay(STEP_MS);
}

// Two LEDs taking turns. The only new idea is that the second LED is always given the opposite of the first.
void runAlternate() {
  digitalWrite(LED_PINS[0], HIGH);
  digitalWrite(LED_PINS[1], LOW);
  delay(STEP_MS);

  digitalWrite(LED_PINS[0], LOW);
  digitalWrite(LED_PINS[1], HIGH);
  delay(STEP_MS);
}

/*
A lit spot walking along the row. This is the first program that has to remember something between one step and the
next: which LED is currently lit. That is what runningPosition is for, and it is why it lives outside this function.
*/
void runRunning() {
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

/*
A car traffic light, the four-phase kind: red, red and yellow together, green, yellow, and round again. Each phase is
a whole state of all three lamps rather than a change to one of them, which is what stops the light ever showing a
combination that does not exist in real life.
*/
void runTraffic() {
  setCarLight(HIGH, LOW, LOW);       // red
  delay(STEP_MS * 4);

  setCarLight(HIGH, HIGH, LOW);      // red and yellow, get ready
  delay(STEP_MS * 2);

  setCarLight(LOW, LOW, HIGH);       // green
  delay(STEP_MS * 4);

  setCarLight(LOW, HIGH, LOW);       // yellow
  delay(STEP_MS * 2);
}

/*
The same car light with a pedestrian light beside it. The pedestrian light is not a second sequence running on its
own: it is derived from the car light, which is what guarantees the two can never both be green.
*/
void runCrossing() {
  setCarLight(HIGH, LOW, LOW);       // cars stopped
  setPedLight(LOW, HIGH);            // so people may cross
  delay(STEP_MS * 4);

  setCarLight(HIGH, HIGH, LOW);      // cars getting ready
  setPedLight(HIGH, LOW);            // people already stopped
  delay(STEP_MS * 2);

  setCarLight(LOW, LOW, HIGH);       // cars going
  setPedLight(HIGH, LOW);            // people still stopped
  delay(STEP_MS * 4);

  setCarLight(LOW, HIGH, LOW);       // cars slowing
  setPedLight(HIGH, LOW);            // people still stopped
  delay(STEP_MS * 2);
}

// Sets all three car lamps in one call, so a phase is written as one line and cannot be left half changed.
void setCarLight(bool red, bool yellow, bool green) {
  digitalWrite(LED_PINS[CAR_RED], red);
  digitalWrite(LED_PINS[CAR_YELLOW], yellow);
  digitalWrite(LED_PINS[CAR_GREEN], green);
}

// The same idea for the pedestrian lamps.
void setPedLight(bool red, bool green) {
  digitalWrite(LED_PINS[PED_RED], red);
  digitalWrite(LED_PINS[PED_GREEN], green);
}

// Turns everything off. Used by the running light so each step only has to switch one LED on.
void allOff() {
  for (int i = 0; i < LED_COUNT; i++) {
    digitalWrite(LED_PINS[i], LOW);
  }
}
