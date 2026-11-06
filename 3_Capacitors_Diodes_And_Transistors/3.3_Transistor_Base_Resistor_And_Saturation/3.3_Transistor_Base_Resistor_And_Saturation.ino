/**
 **************************************************
 *
 * @file        3.3_Transistor_Base_Resistor_And_Saturation.ino
 * @brief       Sweeps the base drive of an NPN transistor with the potentiometer and measures load current, base
 *              current and the ratio between them. Finds the point where the transistor stops saturating, and ends
 *              with the calculation you would do to pick a base resistor.
 *              For details, connection diagram and more, check out the example documentation at: <link placeholder>
 * @author      Borna Zelenika @ soldered.com
 ***************************************************
 */

/*
The TO-92 legs are usually emitter, base, collector from left to right with the flat face towards you, but this
varies by part number. Check yours against its datasheet: getting it wrong costs you a transistor.

IO48 feeds the potentiometer, the wiper feeds the base through a 10 kOhm resistor, and four pins measure the
result: IO7 the wiper, IO5 the base itself, IO4 the bottom of the load resistor and IO6 the collector.

The 10 kOhm is not optional. The base-emitter junction is a diode, so a wiper near 3.3 V wired straight to the base
would put the whole supply across it and destroy the transistor.

Turn the pot slowly up from the ground end and watch the collector. The transistor is off for the first part of the
travel, passes through its linear region in a narrow band around a wiper voltage of 0.66 V, and is saturated for
the rest. The edge of saturation needs only about 5 uA of base current, which no combination of the kit's 10 kOhm
resistors gets near, which is why the base is swept with the pot.
*/
const int DRIVE_PIN = 48;

const int SENSE_A_PIN = 4;
const int SENSE_BASE_PIN = 5;
const int SENSE_C_PIN = 6;
const int SENSE_DRIVE_PIN = 7;

// The base resistor you actually fitted, in ohms.
const float R_BASE_OHMS = 10000.0;

// The load resistor in series with the LED, in ohms.
const float R_LOAD_OHMS = 330.0;

const float VSUPPLY_MV = 3300.0;

// Used by the design step at the end. A datasheet gain is a minimum, so assume a pessimistic one, and overdrive the
// base by five so the transistor stays well past the edge of saturation. 700 mV is what the base sits at.
const float PESSIMISTIC_GAIN = 50.0;
const float OVERDRIVE = 5.0;
const float V_BE_MV = 700.0;

void setup() {
  Serial.begin(115200);
  delay(1000);

  analogReadResolution(12);
  analogSetPinAttenuation(SENSE_A_PIN, ADC_11db);
  analogSetPinAttenuation(SENSE_BASE_PIN, ADC_11db);
  analogSetPinAttenuation(SENSE_C_PIN, ADC_11db);
  analogSetPinAttenuation(SENSE_DRIVE_PIN, ADC_11db);

  pinMode(DRIVE_PIN, OUTPUT);
  digitalWrite(DRIVE_PIN, HIGH);

  delay(50);
}

void loop() {
  // Every voltage below is the mean of 64 readings, because a single one carries a few counts of noise. Each block
  // adds up 64 readings and divides by 64.
  long driveSum = 0;
  for (int i = 0; i < 64; i++) {
    driveSum += analogReadMilliVolts(SENSE_DRIVE_PIN);
  }
  float vDrive = driveSum / 64.0;

  long aSum = 0;
  for (int i = 0; i < 64; i++) {
    aSum += analogReadMilliVolts(SENSE_A_PIN);
  }
  float vA = aSum / 64.0;

  long baseSum = 0;
  for (int i = 0; i < 64; i++) {
    baseSum += analogReadMilliVolts(SENSE_BASE_PIN);
  }
  float vBase = baseSum / 64.0;

  long cSum = 0;
  for (int i = 0; i < 64; i++) {
    cSum += analogReadMilliVolts(SENSE_C_PIN);
  }
  float vC = cSum / 64.0;

  // Everything in that branch is in series, so this is the current through the LED and the transistor too.
  float loadMa = (VSUPPLY_MV - vA) / R_LOAD_OHMS;

  // Both ends are measured rather than assumed: the pot is a 10 kOhm divider, so its wiper sags as soon as the base
  // draws anything. Notice how little the base voltage moves across the sweep, because it is a diode holding near
  // 0.7 V, which is why the design step at the end subtracts it instead of assuming the full supply reaches the
  // resistor.
  float baseMa = 0.0;
  if (vDrive > vBase) {
    baseMa = (vDrive - vBase) / R_BASE_OHMS;
  }

  /*
  This ratio is not the transistor's gain, and reading it as one is the trap the example is built around. While it
  is saturated the collector current is set by the load resistor and the supply, so more base current changes
  nothing and the ratio just falls as you turn the pot up.

  The real gain shows only at the edge. Turn slowly until the collector starts to climb: the ratio printed just
  before it does is the closest this circuit gets to the datasheet figure. On the bench that is 11 to 15 saturated
  against 250 at the edge, same transistor, same wiring. Take the edge figure as give or take a fifth, since it
  comes from a 53 mV difference between two ADC pins.
  */
  float ratio = 0;
  if (baseMa > 0.0001) {
    ratio = loadMa / baseMa;
  }

  Serial.println();
  Serial.print("wiper voltage:     ");
  Serial.print(vDrive, 0);
  Serial.println(" mV");

  Serial.print("base voltage:      ");
  Serial.print(vBase, 0);
  Serial.println(" mV");

  Serial.print("collector voltage: ");
  Serial.print(vC, 0);
  Serial.println(" mV");

  Serial.print("load current:      ");
  Serial.print(loadMa, 3);
  Serial.println(" mA");

  Serial.print("base current:      ");
  Serial.print(baseMa * 1000.0, 1);
  Serial.println(" uA");

  Serial.print("current ratio:     ");
  Serial.println(ratio, 0);

  // This ADC pins at 4095 near 3140 mV, as T15 found. Here that happens with the transistor off, when the bottom of
  // the load resistor sits at the full supply.
  bool transistorOff = false;
  if (vA > 3100) {
    transistorOff = true;
  }

  if (transistorOff) {
    Serial.println("state: off. Load current is at the ADC ceiling and the collector is a floating node, so both");
    Serial.println("       readings above are meaningless. Nothing is conducting.");
  } else if (vC < 300) {
    // As far on as it can get. Power is current times voltage, and the voltage across it is nearly nothing.
    Serial.println("state: saturated, behaving like a closed switch");
  } else {
    // The base has run out, so a whole volt is dropped inside the transistor at the load current. Touch it: the
    // warmth is the power the LED did not get.
    Serial.println("state: NOT saturated. Touch the transistor, it is dissipating the difference.");
  }

  // The design step. With nothing conducting it would run on the ceiling reading and print nonsense, so skip it then.
  // Start from the load current you need, divide by the pessimistic gain, and multiply by the overdrive. Overdriving
  // the base costs almost nothing, so err generous.
  if (!transistorOff) {
    float neededBaseMa = (loadMa / PESSIMISTIC_GAIN) * OVERDRIVE;
    float rRecommended = (VSUPPLY_MV - V_BE_MV) / neededBaseMa;

    Serial.print("safe base resistor for this load: about ");
    Serial.print(rRecommended / 1000.0, 1);
    Serial.println(" kOhm");

    // Convert what the pot is doing right now back into the resistor that would do the same job from 3.3 V. Sweep
    // to the edge and that is the largest base resistor this load could have got away with.
    if (baseMa > 0.0001) {
      float rEquivalent = (VSUPPLY_MV - V_BE_MV) / baseMa;
      Serial.print("this base current equals a ");
      Serial.print(rEquivalent / 1000.0, 1);
      Serial.println(" kOhm resistor driven from 3V3");
    }
  }

  delay(500);
}
