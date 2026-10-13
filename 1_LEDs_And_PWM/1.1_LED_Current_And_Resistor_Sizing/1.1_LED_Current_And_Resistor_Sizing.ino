/**
 **************************************************
 *
 * @file        1.1_LED_Current_And_Resistor_Sizing.ino
 * @brief       The board measures its own LED circuit: forward voltage, actual current, and what resistor you would
 *              need for 5, 10 and 20 mA. Swap the 330 Ohm for the 10 kOhm and run it again.
 *              For details, connection diagram and more, check out the example documentation at: <link placeholder>
 * @author      Soldered Electronics
 ***************************************************
 */

// IO38 drives the LED. IO4 reads the junction between the LED and its resistor, which is the only voltage here
// that has to be measured rather than worked out.
const int LED_PIN = 38;
const int SENSE_PIN = 4;

// The resistor you actually fitted, in ohms. Change this when you swap the resistor: every number below depends
// on it being right.
const float R_OHMS = 330;

const float VSUPPLY_MV = 3300.0;

// The currents we want a resistor recommendation for, in milliamps.
const float TARGETS_MA[] = {5.0, 10.0, 20.0};
const int TARGET_COUNT = 3;

void setup() {
  Serial.begin(115200);
  delay(1000);

  analogReadResolution(12);
  analogSetPinAttenuation(SENSE_PIN, ADC_11db);

  pinMode(LED_PIN, OUTPUT);

  // The LED has to be lit for there to be a current to measure.
  digitalWrite(LED_PIN, HIGH);
  delay(50);
}

void loop() {
  // Average a batch. Every other number here is derived from this one, so a few counts of wobble would show up
  // three times over. Example 2.2 is about that noise.
  long sum = 0;
  for (int i = 0; i < 64; i++) {
    sum += analogReadMilliVolts(SENSE_PIN);
  }
  // The resistor sits between the pin and the junction, so the junction is the LED's anode and this
  // reading is the forward voltage itself. Nothing here has to be worked out.
  float vForwardMv = sum / 64.0;

  // Whatever the supply gives that does not land across the LED lands across the resistor.
  float vResistorMv = VSUPPLY_MV - vForwardMv;

  // Ohm's law, and the LED is in series with the resistor so this is its current too.
  // Millivolts divided by ohms gives milliamps directly.
  float currentMa = vResistorMv / R_OHMS;

  Serial.println();
  Serial.print("Resistor fitted:   ");
  Serial.print(R_OHMS, 0);
  Serial.println(" ohm");

  Serial.print("Across resistor:   ");
  Serial.print(vResistorMv, 0);
  Serial.println(" mV");

  Serial.print("LED forward volts: ");
  Serial.print(vForwardMv / 1000.0, 3);
  Serial.println(" V");

  Serial.print("LED current:       ");
  Serial.print(currentMa, 3);
  Serial.println(" mA");

  // The same two equations run backwards: R = (supply - forward voltage) / target current.
  Serial.println("For this LED, to get:");
  for (int i = 0; i < TARGET_COUNT; i++) {
    float rNeeded = (VSUPPLY_MV - vForwardMv) / TARGETS_MA[i];

    Serial.print("  ");
    Serial.print(TARGETS_MA[i], 0);
    Serial.print(" mA, fit ");
    Serial.print(rNeeded, 0);
    Serial.println(" ohm");
  }

  delay(2000);
}
