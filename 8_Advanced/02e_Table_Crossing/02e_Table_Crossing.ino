/**
 **************************************************
 *
 * @file        02e_Table_Crossing.ino
 * @brief       A car traffic light on IO38 to IO40 with a pedestrian light on IO41 and IO42, where the whole
 *              sequence is a table. Each row says which lamps are on and for how many steps.
 *              For details, connection diagram and more, check out the example documentation at: <link placeholder>
 * @author      Borna Zelenika @ soldered.com
 ***************************************************
 */

// One row of the table: the state of the five lamps, and how many steps the phase lasts.
struct Phase {
  bool carRed;
  bool carYellow;
  bool carGreen;
  bool pedRed;
  bool pedGreen;
  int steps;
};

// The five lamps, left to right as they sit on the breadboard, each with its own 330 Ohm resistor.
// In order: car red, car yellow, car green, pedestrian red, pedestrian green.
const int LAMP_PINS[5] = {38, 39, 40, 41, 42};

// The whole sequence. A row is only ever one complete state of all five lamps, so the two lights can never be
// green together unless a row says so, and no row does.
const Phase PHASES[4] = {
  // car R    car Y    car G    ped R    ped G    steps
  {  true,    false,   false,   false,   true,    4 },   // cars stopped, people may cross
  {  true,    true,    false,   true,    false,   2 },   // cars getting ready, people stopped
  {  false,   false,   true,    true,    false,   4 },   // cars going
  {  false,   true,    false,   true,    false,   2 }    // cars slowing
};
const int PHASE_COUNT = 4;

// How long one step lasts, in milliseconds.
const int STEP_MS = 500;

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("TABLE: the crossing runs from a table of phases.");

  for (int i = 0; i < 5; i++) {
    pinMode(LAMP_PINS[i], OUTPUT);
  }
}

void loop() {
  // Show each row of the table in turn, then start again.
  for (int p = 0; p < PHASE_COUNT; p++) {
    showPhase(PHASES[p]);
    delay(STEP_MS * PHASES[p].steps);
  }
}

// Writes all five lamps from one row of the table.
void showPhase(const Phase &phase) {
  digitalWrite(LAMP_PINS[0], phase.carRed);
  digitalWrite(LAMP_PINS[1], phase.carYellow);
  digitalWrite(LAMP_PINS[2], phase.carGreen);
  digitalWrite(LAMP_PINS[3], phase.pedRed);
  digitalWrite(LAMP_PINS[4], phase.pedGreen);
}
