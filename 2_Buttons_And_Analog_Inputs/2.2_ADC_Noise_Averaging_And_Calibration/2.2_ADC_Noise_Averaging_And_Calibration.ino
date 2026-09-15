/**
 **************************************************
 *
 * @file        2.2_ADC_Noise_Averaging_And_Calibration.ino
 * @brief       PLOT mode draws one pot three ways at once: raw, a 64-sample mean, and an exponential moving average.
 *              CALIBRATE mode checks the chip's factory millivolt figure against a two-point calibration you perform.
 *              For details, connection diagram and more, check out the example documentation at: <link placeholder>
 * @author      GreenyHub
 ***************************************************
 */

/*
Every analog pin in this kit is IO4 to IO10, and that is not a style choice. IO11 upwards are ADC2 pins, and ADC2
shares hardware with the radio: the moment Wi-Fi starts, analogRead() on one stops returning anything useful.
Section 7 turns Wi-Fi on. IO8 and IO9 are ADC1 but carry the Qwiic bus, so the usable set is IO4-IO7 and IO10.
*/
const int POT_PIN = 4;

#define MODE_PLOT      0
#define MODE_CALIBRATE 1

const int MODE = MODE_PLOT;

const int MEAN_SAMPLES = 64;

// The EMA keeps one number and nudges it towards each reading. Smaller alpha, heavier smoothing, slower response.
const float EMA_ALPHA = 0.05;
float ema = 0;
bool emaStarted = false;

// Two-point calibration results, in raw counts, filled in by CALIBRATE mode.
int calLow = 0;
int calHigh = 4095;

void setup() {
  Serial.begin(115200);
  delay(1000);

  analogReadResolution(12);
  analogSetPinAttenuation(POT_PIN, ADC_11db);

  if (MODE == MODE_CALIBRATE) {
    runCalibration();
  }
}

void loop() {
  if (MODE == MODE_PLOT) {
    plotMode();
  } else {
    calibratedRead();
  }
}

/*
Three traces on one graph. Open Tools -> Serial Plotter and leave the pot alone: the raw trace is not a line but a
band about 13 counts wide, and both filtered traces hold to 3. The mean costs 64 reads per answer; the EMA gets the
same steadiness from one.

Then turn slowly up towards 3V3. All three traces hit 4095 and the jitter vanishes, which is not the filters
working. At 11 dB this ADC reaches full scale near 3140 mV and the pot goes on to the rail, so the top slice of the
travel is clipped and steady and wrong at once. Filters fix noise, not range.
*/
void plotMode() {
  int raw = analogRead(POT_PIN);

  long sum = 0;
  for (int i = 0; i < MEAN_SAMPLES; i++) {
    sum += analogRead(POT_PIN);
  }
  int mean = sum / MEAN_SAMPLES;

  if (!emaStarted) {
    ema = raw;
    emaStarted = true;
  }
  ema = ema + EMA_ALPHA * (raw - ema);

  Serial.print(raw);
  Serial.print(" ");
  Serial.print(mean);
  Serial.print(" ");
  Serial.println((int)ema);

  delay(20);
}

// The chip ships with a factory calibration, which analogReadMilliVolts() uses. This measures your own to compare.
void runCalibration() {
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

The 3300 below is a statement about the pot, not the ADC. The wiper really does reach the rail; the converter
clips near 3140 mV and cannot see it.
*/
void calibratedRead() {
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

int averagedRead() {
  long sum = 0;
  for (int i = 0; i < 200; i++) {
    sum += analogRead(POT_PIN);
  }
  return sum / 200;
}

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

// The ADC draws a little current while it samples, so a high-resistance source reads low and slow. A 10 kOhm pot is
// fine. Example 2.3's photoresistor reaches hundreds of kilohms in the dark, and there it is real.
