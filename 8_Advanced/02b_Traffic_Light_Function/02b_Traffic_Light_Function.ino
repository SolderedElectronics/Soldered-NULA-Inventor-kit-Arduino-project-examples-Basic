/**
 **************************************************
 *
 * @file        02b_Traffic_Light_Function.ino
 * @brief       A car traffic light on IO38, IO39 and IO40, the four-phase kind: red, red and yellow together,
 *              green, yellow, and round again.
 *              For details, connection diagram and more, check out the example documentation at: <link placeholder>
 * @author      Borna Zelenika @ soldered.com
 ***************************************************
 */

// The three car lamps, left to right as they sit on the breadboard, each with its own 330 Ohm resistor. IO41 and IO42
// are not used here.
const int RED_PIN = 38;
const int YELLOW_PIN = 39;
const int GREEN_PIN = 40;

// How long one step lasts, in milliseconds. Every phase is a multiple of it.
const int STEP_MS = 500;

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("TRAFFIC: red, red and yellow, green, yellow, and round again.");

  pinMode(RED_PIN, OUTPUT);
  pinMode(YELLOW_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
}

/*
Each phase is a whole state of all three lamps rather than a change to one of them, which is what stops the light
ever showing a combination that does not exist in real life.
*/
void loop() {
  setCarLight(HIGH, LOW, LOW);       // red
  delay(STEP_MS * 4);

  setCarLight(HIGH, HIGH, LOW);      // red and yellow, get ready
  delay(STEP_MS * 2);

  setCarLight(LOW, LOW, HIGH);       // green
  delay(STEP_MS * 4);

  setCarLight(LOW, HIGH, LOW);       // yellow
  delay(STEP_MS * 2);
}

// Sets all three car lamps in one call, so a phase is written as one line and cannot be left half changed.
void setCarLight(bool red, bool yellow, bool green) {
  digitalWrite(RED_PIN, red);
  digitalWrite(YELLOW_PIN, yellow);
  digitalWrite(GREEN_PIN, green);
}
