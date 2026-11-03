/**
 **************************************************
 *
 * @file        07d_Advanced_Diode_Slope.ino
 * @brief       Remembers the lowest and highest current seen while you turn the pot, and works out the diode's
 *              slope in millivolts per tenfold change of current, and its ideality factor.
 *              For details, connection diagram and more, check out the example documentation at: <link placeholder>
 * @author      Borna Zelenika @ soldered.com
 ***************************************************
 */

// Same wiring as T20: IO4 reads the anode, IO5 reads the cathode, and the cathode sits on top of the 330 ohm
// resistor.
const int ANODE_PIN = 4;
const int CATHODE_PIN = 5;
const float R_SENSE_OHMS = 330.0;

// Readings the ADC cannot take, as in T20. Above the first the anode pin is pinned, and below the second the
// sense resistor drops too little voltage to measure well.
const float ADC_CLIP_MV = 3100.0;
const float CURRENT_FLOOR_MA = 0.15;

// The thermal voltage at room temperature is about 25.85 mV. It is a physical constant of the silicon, not a
// property of your diode.
const float THERMAL_VOLTAGE_MV = 25.85;

// The two ends of the sweep so far. haveData stays false until the first good reading arrives.
bool haveData = false;
float lowMa = 0;
float lowMv = 0;
float highMa = 0;
float highMv = 0;

// The mean of 64 readings, as in T20, because one reading is too noisy at the bottom of the sweep.
float averaged(int pin) {
  long sum = 0;
  for (int i = 0; i < 64; i++) {
    sum += analogReadMilliVolts(pin);
  }
  return sum / 64.0;
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  analogReadResolution(12);
  analogSetPinAttenuation(ANODE_PIN, ADC_11db);
  analogSetPinAttenuation(CATHODE_PIN, ADC_11db);

  Serial.println();
  Serial.println("Turn the pot slowly from one end to the other.");
}

void loop() {
  float vAnodeMv = averaged(ANODE_PIN);
  float vCathodeMv = averaged(CATHODE_PIN);

  float vForwardMv = vAnodeMv - vCathodeMv;
  float currentMa = vCathodeMv / R_SENSE_OHMS;

  // Throw away the readings the ADC cannot take.
  if (vAnodeMv >= ADC_CLIP_MV || currentMa < CURRENT_FLOOR_MA) {
    delay(100);
    return;
  }

  // Remember the lowest and the highest current seen so far, together with the forward voltage at each.
  if (!haveData || currentMa < lowMa) {
    lowMa = currentMa;
    lowMv = vForwardMv;
  }
  if (!haveData || currentMa > highMa) {
    highMa = currentMa;
    highMv = vForwardMv;
  }
  haveData = true;

  // The slope only means something once the two ends are well apart, so wait until the current has grown threefold.
  if (highMa > lowMa * 3.0) {
    float decades = log10(highMa / lowMa);
    float mvPerDecade = (highMv - lowMv) / decades;
    float idealityFactor = mvPerDecade / (THERMAL_VOLTAGE_MV * 2.302585);

    Serial.print("from ");
    Serial.print(lowMa, 3);
    Serial.print(" mA to ");
    Serial.print(highMa, 3);
    Serial.print(" mA: ");
    Serial.print(mvPerDecade, 0);
    Serial.print(" mV per tenfold, ideality factor ");
    Serial.println(idealityFactor, 2);
  }

  delay(250);
}
