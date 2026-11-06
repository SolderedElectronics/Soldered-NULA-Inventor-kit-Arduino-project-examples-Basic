/**
 **************************************************
 *
 * @file        3.2_Diode_Forward_Voltage.ino
 * @brief       Logs a diode's forward voltage against the current through it while you turn a pot, so you can see
 *              that the drop is a curve rather than the 0.7 V everybody quotes.
 *              For details, connection diagram and more, check out the example documentation at: <link placeholder>
 * @author      Borna Zelenika @ soldered.com
 ***************************************************
 */

/*
IO4 reads the voltage on the diode's anode and IO5 the voltage on its cathode, so the difference between the two
readings is the forward voltage. The cathode also sits on top of the sense resistor, so that same reading divided
by the resistance is the current flowing through the diode. The pot varies that current.
*/

const int ANODE_PIN = 4;
const int CATHODE_PIN = 5;

// The sense resistor, in ohms.
const float R_SENSE_OHMS = 330.0;

// ADC1 on this board reaches 4095 counts near 3140 mV, not 3300, so the anode pin runs out of range before the pot
// does. An anode reading this high is a pinned ADC, not a measurement.
const float ADC_CLIP_MV = 3100.0;

// Below this the sense resistor drops less than 50 mV, and this ADC reads about 19 mV low down there.
const float CURRENT_FLOOR_MA = 0.15;

void setup() {
  Serial.begin(115200);
  delay(1000);

  analogReadResolution(12);
  analogSetPinAttenuation(ANODE_PIN, ADC_11db);
  analogSetPinAttenuation(CATHODE_PIN, ADC_11db);

  Serial.println();
  Serial.println("Turn the pot slowly from one end to the other.");
  Serial.println("Lines starting with # are readings the ADC could not take, not data.");
  Serial.println("mA,Vf_mV");
}

/*
Turn the pot from one end to the other over about twenty seconds. Measured on a 1N4148 the sweep runs from 0.156 mA
at 534 mV to 6.579 mA at 705 mV, about 105 mV per decade of current, so the 0.7 V everybody quotes is one point on
a slope at a current somebody else picked. Both ends of the sweep are limited by the ADC rather than the diode,
which is why readings are thrown away there instead of printed.
*/
void loop() {
  // A single reading carries a few counts of noise, which at the bottom of the sweep is a large slice of the
  // number, so every voltage here is the mean of 64 readings. Each block below adds up 64 readings and divides.

  // First anode reading.
  long anodeSum = 0;
  for (int i = 0; i < 64; i++) {
    anodeSum += analogReadMilliVolts(ANODE_PIN);
  }
  float vAnodeFirstMv = anodeSum / 64.0;

  // Cathode reading.
  long cathodeSum = 0;
  for (int i = 0; i < 64; i++) {
    cathodeSum += analogReadMilliVolts(CATHODE_PIN);
  }
  float vCathodeMv = cathodeSum / 64.0;

  // Second anode reading. The two pins are not read at the same instant and the pot is moving, so an anode reading
  // from either side of the cathode reading, averaged, cancels most of the skew. Without it a fast turn prints a
  // forward voltage that no diode has.
  long anodeSum2 = 0;
  for (int i = 0; i < 64; i++) {
    anodeSum2 += analogReadMilliVolts(ANODE_PIN);
  }
  float vAnodeSecondMv = anodeSum2 / 64.0;

  float vAnodeMv = (vAnodeFirstMv + vAnodeSecondMv) / 2.0;

  float vForwardMv = vAnodeMv - vCathodeMv;
  float currentMa = vCathodeMv / R_SENSE_OHMS;

  // Past this the anode sits above full scale, so the forward voltage silently reads low by however far past it is.
  if (vAnodeMv >= ADC_CLIP_MV) {
    Serial.println("# anode above ADC full scale, back the pot off");
    delay(100);
    return;
  }

  if (currentMa < CURRENT_FLOOR_MA) {
    delay(100);
    return;
  }

  Serial.print(currentMa, 3);
  Serial.print(",");
  Serial.println(vForwardMv, 0);

  delay(100);
}
