/**
 **************************************************
 *
 * @file        4.3_Driving_A_Buzzer_Through_A_Transistor.ino
 * @brief       Drives a passive buzzer through an NPN transistor from the + row, with a flyback diode.
 *              Sweeps 500 Hz to 5 kHz so you can hear where its resonance is.
 *              For details, connection diagram and more, check out the example documentation at: <link placeholder>
 * @author      Borna Zelenika @ soldered.com
 ***************************************************
 */

/*
IO48 drives the base of the transistor through a 10 kOhm resistor, and the transistor switches the buzzer against
VCC.

The transistor is here because the pin cannot do this job: it gives a few tens of milliamps, while the coil is a
low resistance that will take as much current as it can get.

The band on the 1N4148 marks the cathode and belongs on the positive side. Fitted the other way round it conducts
all the time and shorts the buzzer out.
*/
const int DRIVE_PIN = 48;

const int PWM_RESOLUTION = 8;

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();

  ledcAttach(DRIVE_PIN, 1000, PWM_RESOLUTION);
  ledcWrite(DRIVE_PIN, 0);
}

/*
Listen rather than watch. Somewhere in the range the buzzer gets dramatically louder over a span of a few hundred
hertz, then drops back. That peak is mechanical resonance: driving the disc at its natural frequency makes each
push land in step with the movement already happening.

Write the frequency down. Two buzzers from the same batch will not peak at quite the same place.
*/
void loop() {
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
