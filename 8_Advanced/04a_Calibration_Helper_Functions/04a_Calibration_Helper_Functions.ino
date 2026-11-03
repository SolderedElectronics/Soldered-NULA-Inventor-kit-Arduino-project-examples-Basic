/**
 **************************************************
 *
 * @file        04a_Calibration_Helper_Functions.ino
 * @brief       Checks the chip's factory millivolt figure against a two-point calibration you perform: turn the pot to each
 *              end, then compare your millivolts with the chip's while you turn it across the travel.
 *              For details, connection diagram and more, check out the example documentation at: <link placeholder>
 * @author      Borna Zelenika @ soldered.com
 ***************************************************
 */

/*
Every analog pin in this kit is IO4 to IO10, and that is not a style choice. IO11 upwards are ADC2 pins, and ADC2
shares hardware with the radio: the moment Wi-Fi starts, analogRead() on one stops returning anything useful.
Section 7 turns Wi-Fi on. IO8 and IO9 are ADC1 but carry the Qwiic bus, so the usable set is IO4-IO7 and IO10.
*/
const int POT_PIN = 4;

// Two-point calibration results, in raw counts. They are filled in by the two prompts in setup().
int calLow = 0;
int calHigh = 4095;

void setup() {
  Serial.begin(115200);
  delay(1000);

  analogReadResolution(12);
  analogSetPinAttenuation(POT_PIN, ADC_11db);

  // The chip ships with a factory calibration, which analogReadMilliVolts() uses. This measures your own to compare.
  Serial.println();
  Serial.println("CALIBRATION");
  Serial.println("Turn the pot fully towards GND, then send any character.");
  waitForInput();
  calLow = averagedRead();
  Serial.print("  low point: ");
  Serial.print(calLow);
  Serial.print(" counts, factory says ");
  Serial.print(analogReadMilliVolts(POT_PIN));
  Serial.println(" mV");

  Serial.println("Now turn it fully towards 3V3, then send any character.");
  waitForInput();
  calHigh = averagedRead();
  Serial.print("  high point: ");
  Serial.print(calHigh);
  Serial.print(" counts, factory says ");
  Serial.print(analogReadMilliVolts(POT_PIN));
  Serial.println(" mV");

  // Some pots turn the other way. Without this swap a negative span maps the line backwards and inverts everything.
  if (calLow > calHigh) {
    int swap = calLow;
    calLow = calHigh;
    calHigh = swap;
    Serial.println("  (endpoints swapped: this pot turns the other way)");
  }

  Serial.println();
  Serial.println("counts | your calibration | factory calibration");
}

/*
Turn the knob slowly across the whole travel and watch the gap between the two columns. A two-point fit is a
straight line through the ends and the ADC is not straight, so they agree at the bottom, bow about 70 mV apart near
2500 counts, close again near 3500, and the fit reads high from there. Two points pin down the ends and nothing
else, which is why the factory calibration uses far more than two.

The 3300 below is a statement about the pot, not the ADC. The wiper really does reach 3.3 V; the converter
clips near 3140 mV and cannot see it.
*/
void loop() {
  int raw = analogRead(POT_PIN);
  int factoryMv = analogReadMilliVolts(POT_PIN);

  // Guard against a division by zero if the calibration was run without turning the pot.
  int span = calHigh - calLow;
  if (span == 0) {
    span = 1;
  }
  float yourMv = (float)(raw - calLow) * 3300.0 / span;

  Serial.print(raw);
  Serial.print(" ");
  Serial.print(yourMv, 0);
  Serial.print(" ");
  Serial.println(factoryMv);

  delay(100);
}

// Averages 200 readings, so one noisy sample cannot spoil a calibration point.
int averagedRead() {
  long sum = 0;
  for (int i = 0; i < 200; i++) {
    sum += analogRead(POT_PIN);
  }
  return sum / 200;
}

// Waits until you send a character from the Serial Monitor.
void waitForInput() {
  while (Serial.available()) {
    Serial.read();
  }
  while (Serial.available() == 0) {
    delay(10);
  }
  while (Serial.available()) {
    Serial.read();
  }
}
