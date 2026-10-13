/**
 **************************************************
 *
 * @file        0.2_Reading_A_Potentiometer.ino
 * @brief       Reads a potentiometer and prints it two ways at once, as raw ADC counts and as millivolts, so the
 *              Serial Plotter draws both as traces you can watch while you turn the knob.
 *              For details, connection diagram and more, check out the example documentation at: <link placeholder>
 * @author      Soldered Electronics
 ***************************************************
 */

// The pin the potentiometer's wiper is on. Turning the knob moves it between 0 V and 3.3 V.
const int POT_PIN = 4;

// How long to wait between readings, in milliseconds. 20 ms gives fifty readings a second.
const int DELAY_MS = 20;

void setup() {
  Serial.begin(115200);

  // Time to open the Serial Plotter before the readings start.
  delay(1000);

  // The ESP32-S3's ADC is 12 bits, so analogRead() returns 0 to 4095.
  analogReadResolution(12);

  // ADC_11db is the widest setting and covers the whole 0 V to 3.3 V swing. On the narrower ones
  // anything above about 1 V reads 4095 and the top of the knob's travel looks flat.
  analogSetPinAttenuation(POT_PIN, ADC_11db);
}

void loop() {
  // Raw ADC counts. A count, not a voltage: nothing about it says what it was measuring.
  int raw = analogRead(POT_PIN);

  // The same measurement in millivolts, using calibration figures programmed into the chip at the factory.
  int mv = analogReadMilliVolts(POT_PIN);

  // Two numbers on one line separated by a space gives the Serial Plotter two traces.
  Serial.print(raw);
  Serial.print(" ");
  Serial.println(mv);

  // The plotter autoscales, so leave the pot alone and what fills the window is noise, magnified.
  // Example 2.2 is about that noise and what to do with it.
  delay(DELAY_MS);
}
