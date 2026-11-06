/**
 **************************************************
 *
 * @file        4.3b_Buzzer_Loudness_From_A_Pot.ino
 * @brief       Drives a passive buzzer through an NPN transistor at one fixed frequency, with a pot setting the duty
 *              cycle and so the loudness.
 *              For details, connection diagram and more, check out the example documentation at: <link placeholder>
 * @author      Borna Zelenika @ soldered.com
 ***************************************************
 */

/*
IO48 drives the base of the transistor through a 10 kOhm resistor, and the transistor switches the buzzer against
VCC. IO4 reads the pot, the volume knob.

The transistor is here because the pin cannot do this job: it gives a few tens of milliamps, while the coil is a
low resistance that will take as much current as it can get.

The band on the 1N4148 marks the cathode and belongs on the positive side. Fitted the other way round it conducts
all the time and shorts the buzzer out.
*/
const int DRIVE_PIN = 48;
const int POT_PIN = 4;

// The resonant frequency you measured in T17. T26 and T29 want the same number, so it is worth writing on
// the back of the kit box.
const int RESONANT_HZ = 2700;

const int PWM_RESOLUTION = 8;

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();

  analogReadResolution(12);
  analogSetPinAttenuation(POT_PIN, ADC_11db);

  ledcAttach(DRIVE_PIN, 1000, PWM_RESOLUTION);

  // ledcWriteTone() also forces the duty back to 50 %, and here the duty is yours to set.
  ledcWriteTone(DRIVE_PIN, RESONANT_HZ);
  ledcWrite(DRIVE_PIN, 0);
  Serial.println("VOLUME: turn the pot. Duty and loudness follow it.");
}

/*
The pot maps to 0 to 50 %, not 0 to 100 %, and that upper limit is the interesting part. A 50 % square wave spends
half of every cycle pushing the disc and half letting it go, which is the most movement you can get. Push the duty
higher and the signal approaches a steady level, so the sound falls away again.

No filtering on the reading: the pot's dozen counts of jitter is under a tenth of a percent of duty here and you
cannot hear it. Examples that act on a threshold do need averaging.
*/
void loop() {
  int raw = analogRead(POT_PIN);

  int dutyPercent = (raw * 50) / 4095;
  ledcWrite(DRIVE_PIN, (dutyPercent * 255) / 100);

  Serial.print("duty ");
  Serial.print(dutyPercent);
  Serial.println(" %");

  delay(150);
}
