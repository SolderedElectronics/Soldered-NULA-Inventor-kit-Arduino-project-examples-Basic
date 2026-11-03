/**
 **************************************************
 *
 * @file        07e_Advanced_Transistor_Turn_On.ino
 * @brief       Reports the wiper voltage at which an NPN transistor starts to conduct and the wiper voltage at which
 *              it saturates, and prints the width of the linear region between them.
 *              For details, connection diagram and more, check out the example documentation at: <link placeholder>
 * @author      Borna Zelenika @ soldered.com
 ***************************************************
 */

// Same wiring as T21: IO48 feeds the pot, the wiper feeds the base through the 10 kOhm resistor, and four pins
// measure the result.
const int DRIVE_PIN = 48;
const int SENSE_A_PIN = 4;
const int SENSE_BASE_PIN = 5;
const int SENSE_C_PIN = 6;
const int SENSE_DRIVE_PIN = 7;

const float R_BASE_OHMS = 10000.0;

// As in T21: above 3100 mV the bottom of the load resistor is pinned at the ADC ceiling, which means nothing is
// conducting. Below 300 mV on the collector the transistor is saturated.
const float OFF_THRESHOLD_MV = 3100.0;
const float SATURATED_COLLECTOR_MV = 300.0;

// What has been found in this sweep. Both are cleared when the pot goes back to the off end.
bool onsetFound = false;
bool saturationFound = false;
float onsetWiperMv = 0;

// The mean of 16 readings. This sketch only compares voltages, so fewer readings than in T21 are enough, and the
// pot can be turned a little faster.
float averaged(int pin) {
  long sum = 0;
  for (int i = 0; i < 16; i++) {
    sum += analogReadMilliVolts(pin);
  }
  return sum / 16.0;
}

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

  Serial.println();
  Serial.println("Turn the pot slowly up from the ground end.");
}

void loop() {
  float vWiper = averaged(SENSE_DRIVE_PIN);
  float vBase = averaged(SENSE_BASE_PIN);
  float vA = averaged(SENSE_A_PIN);
  float vC = averaged(SENSE_C_PIN);

  bool transistorOff = (vA > OFF_THRESHOLD_MV);

  if (transistorOff) {
    // Back at the off end, so forget the last sweep and wait for the next one.
    if (onsetFound) {
      Serial.println("Back to off. Turn the pot up again to repeat.");
    }
    onsetFound = false;
    saturationFound = false;
  } else {
    // The first reading with the transistor no longer off is the moment it starts to conduct.
    if (!onsetFound) {
      onsetFound = true;
      onsetWiperMv = vWiper;
      Serial.print("Started to conduct at a wiper of ");
      Serial.print(vWiper, 0);
      Serial.print(" mV, with the base at ");
      Serial.print(vBase, 0);
      Serial.println(" mV");
    }

    // The first reading with the collector pulled low is the moment it saturates.
    if (!saturationFound && vC < SATURATED_COLLECTOR_MV) {
      saturationFound = true;
      float baseUa = ((vWiper - vBase) / R_BASE_OHMS) * 1000.0;
      Serial.print("Saturated at a wiper of ");
      Serial.print(vWiper, 0);
      Serial.print(" mV, with ");
      Serial.print(baseUa, 1);
      Serial.println(" uA of base current");
      Serial.print("The linear region was ");
      Serial.print(vWiper - onsetWiperMv, 0);
      Serial.println(" mV of wiper travel wide");
    }
  }

  delay(50);
}
