/**
 **************************************************
 *
 * @file        07a_Measuring_An_RC_Time_Constant.ino
 * @brief       Charges a capacitor through a resistor, captures the rise into a buffer, and prints the measured
 *              time constant next to the calculated one. Do it with the 10 uF, then with the 100 nF.
 *              For details, connection diagram and more, check out the example documentation at: <link placeholder>
 * @author      Borna Zelenika @ soldered.com
 ***************************************************
 */

/*
IO38 charges the capacitor through the resistor, IO4 reads the voltage across the capacitor as it rises, and the
button on IO21 starts a capture.

The 10 uF is an electrolytic and is polarised: the leg next to the stripe is the negative one. Fitting it backwards
is one of the few ways to damage a part on this page.
*/
const int DRIVE_PIN = 38;
const int SENSE_PIN = 4;
const int BUTTON_PIN = 21;

/*
The parts you actually fitted. Change C_FARADS to 0.0000001 when you swap the 10 uF for the 100 nF.

  100 nF = 0.0000001 F
   10 uF = 0.00001 F

The measurement comes from the samples, so getting this wrong does not break it. It makes the calculated figure
printed beside it meaningless, and it paces the sampling for the wrong part.
*/
const float R_OHMS = 10000.0;
const float C_FARADS = 0.00001;

const float VSUPPLY_MV = 3300.0;

// Nothing is printed until the capture is over. A Serial.print() in the sampling loop costs tens of microseconds,
// and with the 100 nF the whole event lasts about five milliseconds.
const int SAMPLES = 512;
uint16_t valueMv[SAMPLES];
uint32_t timeUs[SAMPLES];

/*
One trip round the sampling loop costs about 51.8 us, nearly all of it inside analogReadMilliVolts(). Charging
finishes after about five time constants, so a capture aimed at three of them puts the 63.2 % crossing well inside,
and the gap below is whatever is left after the loop has spent its own 51.8 us.

The 10 uF wants 586 us between samples, so the gap is about 534 us and the rise is covered comfortably: on this
bench the crossing landed at sample 181 of 512. Work out the same two numbers for the 100 nF before you fit it,
and decide whether this loop can keep up with them.

Without the gap the 10 uF capture covers 26 ms of a 500 ms event and never reaches 63.2 % at all. When you need
more than this loop can give, analogContinuous() hands sampling to the ADC's own DMA engine.
*/
const float LOOP_COST_US = 51.8;
const float SPAN_TAUS = 3.0;
const float WANTED_INTERVAL_US = SPAN_TAUS * R_OHMS * C_FARADS * 1000000.0 / SAMPLES;
const int SAMPLE_GAP_US = (WANTED_INTERVAL_US > LOOP_COST_US) ? (int)(WANTED_INTERVAL_US - LOOP_COST_US) : 0;

void setup() {
  Serial.begin(115200);
  delay(1000);

  analogReadResolution(12);
  analogSetPinAttenuation(SENSE_PIN, ADC_11db);

  pinMode(DRIVE_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  digitalWrite(DRIVE_PIN, LOW);

  Serial.println();
  Serial.print("R = ");
  Serial.print(R_OHMS, 0);
  Serial.print(" ohm, C = ");
  Serial.print(C_FARADS * 1000000.0, 3);
  Serial.print(" uF, gap = ");
  Serial.print(SAMPLE_GAP_US);
  Serial.println(" us per sample");
  Serial.println("Press the button to charge the capacitor and capture the rise.");
}

void loop() {
  if (digitalRead(BUTTON_PIN) != LOW) {
    return;
  }

  // Empty the capacitor first, or the next capture starts halfway up. Five time constants is the usual rule.
  digitalWrite(DRIVE_PIN, LOW);
  delay((int)(5000 * R_OHMS * C_FARADS) + 20);

  // Throw one reading away before starting the clock. The first conversion after the ADC has been idle costs about
  // 344 us against 51.8 us, and it lands where the voltage is moving fastest, so the faster the rise the further
  // that one slow reading shifts the 63.2 % crossing.
  analogReadMilliVolts(SENSE_PIN);

  // Anything between these two lines is time the rise happened without you watching.
  uint32_t startUs = micros();
  digitalWrite(DRIVE_PIN, HIGH);

  for (int i = 0; i < SAMPLES; i++) {
    valueMv[i] = analogReadMilliVolts(SENSE_PIN);
    timeUs[i] = micros() - startUs;
    if (SAMPLE_GAP_US > 0) {
      delayMicroseconds(SAMPLE_GAP_US);
    }
  }

  reportCapture();

  // Wait for the button to be released before arming again.
  while (digitalRead(BUTTON_PIN) == LOW) {
    delay(10);
  }
  delay(200);
}

void reportCapture() {
  // A capacitor fills quickly at first and then slowly, because the closer it gets to the supply the smaller the
  // voltage difference driving current into it. Tau is the time to reach 63.2 %, and it is R times C, nothing else.
  float tauCalculatedUs = R_OHMS * C_FARADS * 1000000.0;
  float targetMv = VSUPPLY_MV * 0.632;

  // Find the first sample above the 63.2 % line.
  int crossing = -1;
  for (int i = 0; i < SAMPLES; i++) {
    if (valueMv[i] >= targetMv) {
      crossing = i;
      break;
    }
  }

  float totalUs = timeUs[SAMPLES - 1];
  float intervalUs = totalUs / SAMPLES;

  Serial.println();
  Serial.print("Capture covered ");
  Serial.print(totalUs / 1000.0, 2);
  Serial.print(" ms at about ");
  Serial.print(intervalUs, 1);
  Serial.println(" us per sample");

  Serial.print("Calculated R x C: ");
  Serial.print(tauCalculatedUs / 1000.0, 3);
  Serial.println(" ms");

  if (crossing < 0) {
    Serial.println("Never reached 63.2 %. The capture is shorter than the rise.");
  } else {
    Serial.print("Measured to 63.2 %: ");
    Serial.print(timeUs[crossing] / 1000.0, 3);
    Serial.print(" ms, from sample ");
    Serial.print(crossing);
    Serial.print(" of ");
    Serial.println(SAMPLES);

    // How many samples landed on the rise decides whether to believe the answer. Below twenty, the figure is
    // worth a few percent at best, so say so rather than print it as though it were solid.
    if (crossing < 20) {
      Serial.println("Only a handful of samples on the rise. Treat this figure with suspicion.");
    }
  }

  /*
  Both ends of the CSV lie, and neither is the capacitor's fault. The flat top of the 100 nF capture reads about
  3139 mV rather than 3300 because that is the top of this ADC's range, and the first samples of the 10 uF capture
  read 0 mV while the capacitor is plainly charging. A measurement taken in the middle is worth more than one at
  either extreme, which is the other reason tau is defined at 63.2 %.
  */
  Serial.println();
  Serial.println("us,mV");
  for (int i = 0; i < SAMPLES; i++) {
    Serial.print(timeUs[i]);
    Serial.print(",");
    Serial.println(valueMv[i]);
  }
}
