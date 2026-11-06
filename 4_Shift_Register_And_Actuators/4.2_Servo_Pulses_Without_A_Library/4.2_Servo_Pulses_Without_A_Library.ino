/**
 **************************************************
 *
 * @file        4.2_Servo_Pulses_Without_A_Library.ino
 * @brief       No servo library. Steps the pulse width in 10 us jumps so you can find your servo's real travel. Two
 *              capacitors sit across the servo's supply to absorb the current the motor pulls when it starts.
 *              For details, connection diagram and more, check out the example documentation at: <link placeholder>
 * @author      Borna Zelenika @ soldered.com
 ***************************************************
 */

/*
IO47 carries the servo's signal.

The servo's power comes from VCC, the board's input supply, and never from 3V3. VCC is the USB-C supply less the
board's input diode, about 4.7 V, and it is what the 3.3 V regulator itself runs from. That regulator is meant
to run the chip rather than start a motor. A 10 uF and a 100 nF go across the servo's own supply holes; the closing
comment explains why.
*/
const int SERVO_PIN = 47;

// The sweep has to run past the point where your servo gives up, or the numbers you write down are these rather
// than your servo's. If the arm is still moving when the sweep turns around, widen them.
const int SWEEP_MIN_US = 300;
const int SWEEP_MAX_US = 2700;

/*
The pulse repeats every 20 ms and that never changes. Only its width carries the position.

The S3's LEDC timers stop at 14 bits, so asking for 16 makes ledcAttach() fail and the pin never pulses at all. At
14 bits one pulse period is split into 16384 counts, so a pulse of a given width in microseconds is
width * 16384 / 20000 counts, and one count is about 1.2 us.
*/
const int SERVO_FREQ = 50;
const int SERVO_RESOLUTION = 14;
const int PERIOD_US = 20000;
const int DUTY_STEPS = 16384;

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Checking the return is the difference between a silent pin and knowing why it is silent.
  if (!ledcAttach(SERVO_PIN, SERVO_FREQ, SERVO_RESOLUTION)) {
    Serial.println("ledcAttach failed, the servo will not move");
  }

  // Start in the middle of the travel, with a pulse of 1500 us.
  ledcWrite(SERVO_PIN, 1500 * DUTY_STEPS / PERIOD_US);

  Serial.println();
  Serial.println("LIMITS: stepping the pulse width. Watch and listen to the arm.");
  Serial.println("Note the width where it stops moving, and the width where it starts buzzing.");
  delay(2000);
}

/*
Near each end two things happen in order: the arm stops moving, then the servo buzzes because it is pushing against
its own end stop. Stop at the buzz, because holding a servo there draws stall current continuously and will
eventually cook it.

The pair you end up with will not match the defaults in the next task. Mass-produced parts are not identical,
and calibrating is the correct response, not assuming the servo is broken.

This is the entire servo protocol: a pulse of a certain length, fifty times a second. No address, no
acknowledgement, no feedback. The servo has a potentiometer on its output shaft and turns the motor until that
matches the pulse. Each loop below changes the pulse width in steps of 10 us, which is about 240 steps each way, so a
counted loop is the sensible way to write them.
*/
void loop() {
  // Down from the widest pulse to the narrowest.
  for (int p = SWEEP_MAX_US; p >= SWEEP_MIN_US; p -= 10) {
    ledcWrite(SERVO_PIN, p * DUTY_STEPS / PERIOD_US);
    Serial.println(p);
    delay(120);
  }

  delay(1000);

  // And back up again.
  for (int p = SWEEP_MIN_US; p <= SWEEP_MAX_US; p += 10) {
    ledcWrite(SERVO_PIN, p * DUTY_STEPS / PERIOD_US);
    Serial.println(p);
    delay(120);
  }

  delay(1000);
}

/*
Why the two capacitors are here.

A motor starting is a sudden demand for current, and the voltage at the motor falls while the supply catches up. A
capacitor next to a hungry part is a local reservoir that covers the first gulp. Two values, because the 100 nF is
fast and handles the sharp switching edges while the 10 uF holds the charge for the motor spinning up.

They go across the servo's own supply holes, not down on the power rows: the wire between a capacitor and the part it
feeds has resistance, and a reservoir at the far end of the board arrives too late.

If a supply sags far enough the chip resets itself deliberately, which is a brownout. One SG90 on this board does
not get close, tried on VCC and 3V3, free-running and stalled. A bigger motor, a weak battery or several servos
will, and the fix is always decoupling close to the load.
*/
