/**
 **************************************************
 *
 * @file        07g_Transistor_Curve_Tracer.ino
 * @brief       Prints a line of comma-separated values ten times a second while you sweep the base drive of an NPN
 *              transistor with the potentiometer: the wiper, base and collector voltages, the base and collector
 *              currents, and the region the transistor is working in. Paste the lines into a spreadsheet to plot them.
 *              For details, connection diagram and more, check out the example documentation at: <link placeholder>
 * @author      Borna Zelenika @ soldered.com
 ***************************************************
 */

// Same wiring as T16: IO48 feeds the pot, the wiper feeds the base through the 10 kOhm resistor, and four pins
// measure the result.
const int DRIVE_PIN = 48;
const int SENSE_A_PIN = 4;
const int SENSE_BASE_PIN = 5;
const int SENSE_C_PIN = 6;
const int SENSE_DRIVE_PIN = 7;

// The two resistors in the circuit, in ohms, and the supply the load branch hangs from, in millivolts.
const float R_BASE_OHMS = 10000.0;
const float R_LOAD_OHMS = 330.0;
const float VSUPPLY_MV = 3300.0;

// As in T16: above 3100 mV the bottom of the load resistor is pinned at the ADC ceiling, which means nothing is
// conducting. Below 300 mV on the collector the transistor is saturated.
const float OFF_THRESHOLD_MV = 3100.0;
const float SATURATED_COLLECTOR_MV = 300.0;

// The pause between lines. The four averaged readings take a few milliseconds more, so a line comes out about
// every 100 ms.
const int SAMPLE_MS = 100;

void setup() {
  Serial.begin(115200);
  delay(1000);

  analogReadResolution(12);
  analogSetPinAttenuation(SENSE_A_PIN, ADC_11db);
  analogSetPinAttenuation(SENSE_BASE_PIN, ADC_11db);
  analogSetPinAttenuation(SENSE_C_PIN, ADC_11db);
  analogSetPinAttenuation(SENSE_DRIVE_PIN, ADC_11db);

  // Switch the pot's supply on, as in T16.
  pinMode(DRIVE_PIN, OUTPUT);
  digitalWrite(DRIVE_PIN, HIGH);

  delay(50);

  // The first line is the column names. A spreadsheet uses it as the titles of the columns.
  Serial.println("wiper_mV,base_mV,collector_mV,base_uA,collector_mA,region");
}

void loop() {
  float wiperMv = averaged(SENSE_DRIVE_PIN);
  float baseMv = averaged(SENSE_BASE_PIN);
  float loadPointMv = averaged(SENSE_A_PIN);
  float collectorMv = averaged(SENSE_C_PIN);

  // The base current is the voltage across the 10 kOhm resistor divided by its resistance. Millivolts divided by
  // ohms is milliamps, so multiply by 1000 to get microamps.
  float baseUa = 0.0;
  if (wiperMv > baseMv) {
    baseUa = ((wiperMv - baseMv) / R_BASE_OHMS) * 1000.0;
  }

  // The collector current is the current through the 330 Ohm resistor and the LED, which are in series with it.
  // With nothing conducting the reading is stuck at the ADC ceiling, so the current is set to zero instead.
  float collectorMa = 0.0;
  if (loadPointMv <= OFF_THRESHOLD_MV) {
    collectorMa = (VSUPPLY_MV - loadPointMv) / R_LOAD_OHMS;
  }

  // One line of the table: the values separated by commas, with no spaces.
  Serial.print(wiperMv, 0);
  Serial.print(",");
  Serial.print(baseMv, 0);
  Serial.print(",");
  Serial.print(collectorMv, 0);
  Serial.print(",");
  Serial.print(baseUa, 1);
  Serial.print(",");
  Serial.print(collectorMa, 2);
  Serial.print(",");
  Serial.println(regionName(loadPointMv, collectorMv));

  delay(SAMPLE_MS);
}

// The mean of 16 readings of one pin, in millivolts. A single reading carries a few counts of noise.
float averaged(int pin) {
  long sum = 0;
  for (int i = 0; i < 16; i++) {
    sum += analogReadMilliVolts(pin);
  }
  return sum / 16.0;
}

// Says which region the transistor is in, from the two voltages that tell: the bottom of the load resistor and the
// collector. Cutoff means nothing flows, saturated means it is fully on, and active is the stretch in between.
const char *regionName(float loadPointMv, float collectorMv) {
  if (loadPointMv > OFF_THRESHOLD_MV) {
    return "cutoff";
  }
  if (collectorMv < SATURATED_COLLECTOR_MV) {
    return "saturated";
  }
  return "active";
}
