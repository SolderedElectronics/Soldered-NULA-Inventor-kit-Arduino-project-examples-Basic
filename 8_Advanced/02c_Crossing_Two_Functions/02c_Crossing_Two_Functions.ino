/**
 **************************************************
 *
 * @file        02c_Crossing_Two_Functions.ino
 * @brief       A car traffic light on IO38 to IO40 with a pedestrian light on IO41 and IO42 beside it. The
 *              pedestrian light is set inside the same four phases, so the two can never both be green.
 *              For details, connection diagram and more, check out the example documentation at: <link placeholder>
 * @author      Borna Zelenika @ soldered.com
 ***************************************************
 */

// The five lamps, left to right as they sit on the breadboard, each with its own 330 Ohm resistor.
const int CAR_RED_PIN = 38;
const int CAR_YELLOW_PIN = 39;
const int CAR_GREEN_PIN = 40;
const int PED_RED_PIN = 41;
const int PED_GREEN_PIN = 42;

// How long one step lasts, in milliseconds. Every phase is a multiple of it.
const int STEP_MS = 500;

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("CROSSING: the same car light, with a pedestrian light that is its opposite.");

  pinMode(CAR_RED_PIN, OUTPUT);
  pinMode(CAR_YELLOW_PIN, OUTPUT);
  pinMode(CAR_GREEN_PIN, OUTPUT);
  pinMode(PED_RED_PIN, OUTPUT);
  pinMode(PED_GREEN_PIN, OUTPUT);
}

/*
The pedestrian light is not a second sequence running on its own: it is set inside the same four phases, which is
what guarantees the two can never both be green.
*/
void loop() {
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
  digitalWrite(CAR_RED_PIN, red);
  digitalWrite(CAR_YELLOW_PIN, yellow);
  digitalWrite(CAR_GREEN_PIN, green);
}

// The same idea for the pedestrian lamps.
void setPedLight(bool red, bool green) {
  digitalWrite(PED_RED_PIN, red);
  digitalWrite(PED_GREEN_PIN, green);
}
