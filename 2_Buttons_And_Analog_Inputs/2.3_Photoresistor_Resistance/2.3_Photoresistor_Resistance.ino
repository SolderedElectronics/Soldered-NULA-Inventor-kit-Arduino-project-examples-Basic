/**
 **************************************************
 *
 * @file        2.3_Photoresistor_Resistance.ino
 * @brief       Measures the photoresistor's own resistance from the voltage at the junction of the divider, so you can pick
 *              a fixed resistor to pair it with.
 *              For details, connection diagram and more, check out the example documentation at: <link placeholder>
 * @author      Borna Zelenika @ soldered.com
 ***************************************************
 */

/*
The board cannot measure resistance, only voltage. Pairing the photoresistor with a fixed resistor turns its
changing resistance into a changing voltage at the junction, and IO4 reads that junction.
*/
const int LDR_PIN = 4;

// The fixed resistor you actually fitted. Start with the kit's 10 kOhm, then change this to whatever you pick
// after measuring, or the calculation lies.
const float R_FIXED_OHMS = 10000.0;

const float VSUPPLY_MV = 3300.0;

void setup() {
  Serial.begin(115200);
  delay(1000);

  analogReadResolution(12);
  analogSetPinAttenuation(LDR_PIN, ADC_11db);
}

/*
In a divider the two voltages are in the same ratio as the two resistances, so the unknown falls out:

  R_ldr = R_fixed * (Vsupply - Vjunction) / Vjunction

Run it covered by your hand, then under a phone torch. Then fit a resistor near the geometric middle of the two
results, the square root of them multiplied, set R_FIXED_OHMS above, and see whether the reading now covers more of
the 0 to 4095 range than the 10 kOhm did.
*/
void loop() {
  // Average 64 readings, so a few counts of wobble do not show up in the answer.
  long sum = 0;
  for (int i = 0; i < 64; i++) {
    sum += analogReadMilliVolts(LDR_PIN);
  }
  float vMv = sum / 64.0;

  // Below a few millivolts the arithmetic blows up and the answer means nothing.
  if (vMv < 5) {
    Serial.println("Reading too low to work with. Is the divider wired the right way round?");
    delay(500);
    return;
  }

  float rLdr = R_FIXED_OHMS * (VSUPPLY_MV - vMv) / vMv;

  Serial.print("junction ");
  Serial.print(vMv, 0);
  Serial.print(" mV,  photoresistor about ");
  if (rLdr > 1000) {
    Serial.print(rLdr / 1000.0, 1);
    Serial.println(" kOhm");
  } else {
    Serial.print(rLdr, 0);
    Serial.println(" ohm");
  }

  delay(500);
}
