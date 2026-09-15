/**
 **************************************************
 *
 * @file        2.3_Photoresistor_Divider_With_Hysteresis.ino
 * @brief       Measure the photoresistor's own resistance, pick a divider resistor from that measurement, calibrate
 *              the light range, then switch an LED on one threshold and on two.
 *              For details, connection diagram and more, check out the example documentation at: <link placeholder>
 * @author      GreenyHub
 ***************************************************
 */

/*
The board cannot measure resistance, only voltage. Pairing the photoresistor with a fixed resistor turns its
changing resistance into a changing voltage at the junction, and IO4 reads that junction.

  MODE_MEASURE_R   work out the photoresistor's resistance right now. Run it covered, then under a torch.
  MODE_CALIBRATE   record the darkest and brightest readings over ten seconds and report light as a percentage.
  MODE_SINGLE      LED switched by one threshold. Hold the light at the boundary and watch it fail.
  MODE_HYSTERESIS  LED switched by two thresholds. Same light, steady output.
*/
#define MODE_MEASURE_R  0
#define MODE_CALIBRATE  1
#define MODE_SINGLE     2
#define MODE_HYSTERESIS 3

const int MODE = MODE_HYSTERESIS;

const int LDR_PIN = 4;
const int LED_PIN = 38;

// The fixed resistor you actually fitted. Start with the kit's 10 kOhm, then change this to whatever you pick
// after MODE_MEASURE_R, or the calculation lies.
const float R_FIXED_OHMS = 10000.0;

const float VSUPPLY_MV = 3300.0;

// Calibration results, in raw ADC counts.
int lightMin = 4095;
int lightMax = 0;
bool calibrated = false;

// Thresholds, as a percentage of the calibrated range.
const int THRESHOLD_PCT = 50;
const int HYSTERESIS_PCT = 8;

bool ledOn = false;

void setup() {
  Serial.begin(115200);
  delay(1000);

  analogReadResolution(12);
  analogSetPinAttenuation(LDR_PIN, ADC_11db);

  pinMode(LED_PIN, OUTPUT);

  if (MODE != MODE_MEASURE_R) {
    runCalibration();
  }
}

void loop() {
  switch (MODE) {
    case MODE_MEASURE_R:
      measureResistance();
      break;
    case MODE_CALIBRATE:
      reportPercent();
      break;
    case MODE_SINGLE:
      singleThreshold();
      break;
    case MODE_HYSTERESIS:
      hysteresisThreshold();
      break;
  }
}

/*
In a divider the two voltages are in the same ratio as the two resistances, so the unknown falls out:

  R_ldr = R_fixed * (Vsupply - Vjunction) / Vjunction

Run it covered by your hand, then under a phone torch. Then fit a resistor near the geometric middle of the two
results, the square root of them multiplied, set R_FIXED_OHMS above, and see whether the reading now covers more of
the 0 to 4095 range than the 10 kOhm did.
*/
void measureResistance() {
  float vMv = averagedMillivolts();

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

/*
Ten seconds of hand-waving, keeping the lowest and highest readings seen. A percentage of this sensor's range in
this room is honest; converting to lux would be a fiction, because a photoresistor is not linear in light level or
in anything else convenient and parts vary enormously.
*/
void runCalibration() {
  Serial.println();
  Serial.println("CALIBRATING for 10 seconds. Cover the sensor, then light it up.");

  unsigned long start = millis();
  while (millis() - start < 10000) {
    int raw = analogRead(LDR_PIN);
    if (raw < lightMin) {
      lightMin = raw;
    }
    if (raw > lightMax) {
      lightMax = raw;
    }
    delay(5);
  }

  // A range of zero would make every later division fail.
  if (lightMax - lightMin < 10) {
    lightMax = lightMin + 10;
  }
  calibrated = true;

  Serial.print("Range: ");
  Serial.print(lightMin);
  Serial.print(" to ");
  Serial.println(lightMax);
  Serial.println();
}

int lightPercent() {
  int raw = analogRead(LDR_PIN);
  int pct = map(raw, lightMin, lightMax, 0, 100);
  return constrain(pct, 0, 100);
}

void reportPercent() {
  Serial.println(lightPercent());
  delay(100);
}

// Hold the light steady right at the boundary. The reading wobbles a few counts, every wobble crosses the line,
// and the LED flickers. Nothing is wrong with the code.
void singleThreshold() {
  int pct = lightPercent();

  ledOn = (pct > THRESHOLD_PCT);
  digitalWrite(LED_PIN, ledOn);

  Serial.print(pct);
  Serial.print(" ");
  Serial.println(ledOn ? 100 : 0);
  delay(50);
}

// Switching on takes more light than switching off, so noise cannot move the output back. Same mechanism as the
// relay in 5.2 and the thermostat in 7.1.
void hysteresisThreshold() {
  int pct = lightPercent();

  int onAbove = THRESHOLD_PCT + HYSTERESIS_PCT;
  int offBelow = THRESHOLD_PCT - HYSTERESIS_PCT;

  if (!ledOn && pct > onAbove) {
    ledOn = true;
  } else if (ledOn && pct < offBelow) {
    ledOn = false;
  }

  digitalWrite(LED_PIN, ledOn);

  Serial.print(pct);
  Serial.print(" ");
  Serial.println(ledOn ? 100 : 0);
  delay(50);
}

float averagedMillivolts() {
  long sum = 0;
  for (int i = 0; i < 64; i++) {
    sum += analogReadMilliVolts(LDR_PIN);
  }
  return sum / 64.0;
}

// The photoresistor sees the LED wired next to it. Here that only skews a threshold. In 7.3 it sends the tracker
// off course and looks exactly like a code bug.
