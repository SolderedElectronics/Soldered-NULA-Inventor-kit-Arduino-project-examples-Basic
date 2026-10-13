/**
 **************************************************
 *
 * @file        4.3_Driving_A_Buzzer_Through_A_Transistor.ino
 * @brief       Drives a passive buzzer through an NPN transistor from the + row, with a flyback diode.
 *              Sweeps 500 Hz to 5 kHz to find its resonance, then steps the duty cycle at that frequency.
 *              For details, connection diagram and more, check out the example documentation at: <link placeholder>
 * @author      Soldered Electronics
 ***************************************************
 */

/*
IO48 drives the base of the transistor through a 10 kOhm resistor, and the transistor switches the buzzer against
VCC. IO4 reads the pot, the volume knob in MODE_VOLUME.

The transistor is here because the pin cannot do this job: it gives a few tens of milliamps, while the coil is a
low resistance that will take as much current as it can get.

The band on the 1N4148 marks the cathode and belongs on the positive side. Fitted the other way round it conducts
all the time and shorts the buzzer out.
*/
const int DRIVE_PIN = 48;
const int POT_PIN = 4;

/*
  MODE_LEVEL       drive it with a steady high level and listen to the silence.
  MODE_SWEEP       500 Hz to 5 kHz over twenty seconds. Note where it gets dramatically louder.
  MODE_DUTY        hold the resonant frequency and step the duty through 5, 10, 25 and 50 %.
  MODE_VOLUME      the same idea with your hand on it: the pot sets the duty, so the pot sets the loudness.
*/
#define MODE_LEVEL  0
#define MODE_SWEEP  1
#define MODE_DUTY   2
#define MODE_VOLUME 3

const int MODE = MODE_VOLUME;

// The resonant frequency you measured in MODE_SWEEP. Examples 7.1 and 7.4 want the same number, so it is worth
// writing on the back of the kit box.
const int RESONANT_HZ = 2700;

const int PWM_RESOLUTION = 8;

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();

  if (MODE == MODE_LEVEL) {
    // A passive buzzer is a coil and a metal disc with no oscillator inside, so it needs an alternating signal. A
    // steady high pushes the disc once and holds it: a faint click, then silence, with current flowing throughout.
    pinMode(DRIVE_PIN, OUTPUT);
    digitalWrite(DRIVE_PIN, HIGH);
    Serial.println("LEVEL: a steady HIGH. Silence, and current flowing. Do not leave this running.");
  } else {
    analogReadResolution(12);
    analogSetPinAttenuation(POT_PIN, ADC_11db);

    ledcAttach(DRIVE_PIN, 1000, PWM_RESOLUTION);
    ledcWrite(DRIVE_PIN, 0);

    if (MODE == MODE_VOLUME) {
      // ledcWriteTone() also forces the duty back to 50 %, and in this mode the duty is yours to set.
      ledcWriteTone(DRIVE_PIN, RESONANT_HZ);
      ledcWrite(DRIVE_PIN, 0);
      Serial.println("VOLUME: turn the pot. Duty and loudness follow it.");
    }
  }
}

void loop() {
  switch (MODE) {
    case MODE_LEVEL:
      delay(1000);
      break;
    case MODE_SWEEP:
      sweepMode();
      break;
    case MODE_DUTY:
      dutyMode();
      break;
    case MODE_VOLUME:
      volumeMode();
      break;
  }
}

/*
Listen rather than watch. Somewhere in the range the buzzer gets dramatically louder over a span of a few hundred
hertz, then drops back. That peak is mechanical resonance: driving the disc at its natural frequency makes each
push land in step with the movement already happening.

Write the frequency down. Two buzzers from the same batch will not peak at quite the same place.
*/
void sweepMode() {
  Serial.println("sweeping 500 Hz to 5000 Hz over 20 seconds");

  for (int hz = 500; hz <= 5000; hz += 25) {
    ledcWriteTone(DRIVE_PIN, hz);
    Serial.println(hz);
    delay(110);
  }

  ledcWrite(DRIVE_PIN, 0);
  Serial.println("done");
  delay(3000);
}

// Frequency decides the pitch; duty decides how long the coil is energised each cycle, so roughly how hard the disc
// is pushed. It gets louder as the duty rises, and the transistor gets warmer with it.
void dutyMode() {
  const int duties[] = {5, 10, 25, 50};

  for (int i = 0; i < 4; i++) {
    Serial.print("duty ");
    Serial.print(duties[i]);
    Serial.println(" %");

    // ledcWriteTone() leaves the duty at 50 %, so the duty is written second to override it.
    ledcWriteTone(DRIVE_PIN, RESONANT_HZ);
    ledcWrite(DRIVE_PIN, (duties[i] * 255) / 100);
    delay(1500);
  }

  // Stop it with a duty of zero, not a level on the pin. A steady level is the MODE_LEVEL case: silent, and drawing
  // current through the coil until something gets warm.
  ledcWrite(DRIVE_PIN, 0);
  delay(1500);

  /*
  Now take the 1N4148 out and run this again with your finger on the transistor. A coil resists change in its
  current, so switching it off spikes the voltage the other way, hard enough to punch through the transistor over
  time. The diode gives that current a harmless loop to die out in, which is why it is fitted backwards relative to
  the supply and does nothing until the spike arrives. Without it the transistor runs noticeably warmer. Put it back.
  */
}

/*
The pot maps to 0 to 50 %, not 0 to 100 %, and that upper limit is the interesting part. A 50 % square wave spends
half of every cycle pushing the disc and half letting it go, which is the most movement you can get. Push the duty
higher and the signal approaches the steady level from MODE_LEVEL, so the sound falls away again.

No filtering on the reading: the pot's dozen counts of jitter is under a tenth of a percent of duty here and you
cannot hear it. Examples that act on a threshold do need the averaging from 2.2.
*/
void volumeMode() {
  int raw = analogRead(POT_PIN);

  int dutyPercent = (raw * 50) / 4095;
  ledcWrite(DRIVE_PIN, (dutyPercent * 255) / 100);

  Serial.print("duty ");
  Serial.print(dutyPercent);
  Serial.println(" %");

  delay(150);
}
