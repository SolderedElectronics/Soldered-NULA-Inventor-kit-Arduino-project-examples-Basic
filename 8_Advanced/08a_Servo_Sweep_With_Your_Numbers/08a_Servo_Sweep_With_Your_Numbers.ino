/**
 **************************************************
 *
 * @file        08a_Servo_Sweep_With_Your_Numbers.ino
 * @brief       No servo library. Maps a pot across your servo's measured limits through a rate limiter. Two capacitors
 *              sit across the servo's supply to absorb the current the motor pulls when it starts.
 *              For details, connection diagram and more, check out the example documentation at: <link placeholder>
 * @author      Borna Zelenika @ soldered.com
 ***************************************************
 */

/*
IO47 carries the servo's signal and IO4 reads the pot.

The servo's power comes from VCC, the board's input supply, and never from 3V3. VCC is the USB-C supply less the
board's input diode, about 4.7 V, and it is what the 3.3 V regulator itself runs from. That regulator is meant
to run the chip rather than start a motor. A 10 uF and a 100 nF go across the servo's own supply holes; the closing
comment explains why.
*/
const int SERVO_PIN = 47;
const int POT_PIN = 4;

/*
Your servo's real limits, in microseconds of pulse width. A width a servo can pass through is not a width it can
hold: the SG90 here still moved at 400, but parked below about 500 it hunts against its own end stop and never
settles, so the low limit sits above the sweep figure rather than just inside it.

Put in the pair you measured in T24, then hold each end for a few seconds before you trust it. T35 and T36
leave the servo sitting on these numbers unattended.
*/
const int PULSE_MIN_US = 550;
const int PULSE_MAX_US = 2500;

/*
The pulse repeats every 20 ms and that never changes. Only its width carries the position.

The S3's LEDC timers stop at 14 bits, so asking for 16 makes ledcAttach() fail and the pin never pulses at all. At
14 bits, duty = pulse_us * 16384 / 20000, and one count is about 1.2 us.
*/
const int SERVO_FREQ = 50;
const int SERVO_RESOLUTION = 14;
const long PERIOD_US = 20000;

// The servo can slam end to end as fast as its gears allow, which is violent, draws the most current, and in T35
// turns a tracker into an oscillator. This caps how far the commanded position may move per update.
const int MAX_STEP_US_PER_UPDATE = 8;
const int UPDATE_INTERVAL_MS = 20;

int currentPulseUs = (PULSE_MIN_US + PULSE_MAX_US) / 2;
unsigned long lastUpdateMs = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);

  analogReadResolution(12);
  analogSetPinAttenuation(POT_PIN, ADC_11db);

  // Checking the return is the difference between a silent pin and knowing why it is silent.
  if (!ledcAttach(SERVO_PIN, SERVO_FREQ, SERVO_RESOLUTION)) {
    Serial.println("ledcAttach failed, the servo will not move");
  }

  writePulse(currentPulseUs);
}

// The pot picks a target and the limiter decides how fast the servo may get there. Turn the knob quickly: without
// the limiter the arm jumps, with it the arm sweeps. Commanded position is a separate thing from requested, the
// same idea as the relay lockout in T30.
void loop() {
  if (millis() - lastUpdateMs < UPDATE_INTERVAL_MS) {
    return;
  }
  lastUpdateMs = millis();

  int pot = analogRead(POT_PIN);
  int targetUs = map(pot, 0, 4095, PULSE_MIN_US, PULSE_MAX_US);

  int difference = targetUs - currentPulseUs;
  if (difference > MAX_STEP_US_PER_UPDATE) {
    difference = MAX_STEP_US_PER_UPDATE;
  } else if (difference < -MAX_STEP_US_PER_UPDATE) {
    difference = -MAX_STEP_US_PER_UPDATE;
  }
  currentPulseUs += difference;

  writePulse(currentPulseUs);

  Serial.print(targetUs);
  Serial.print(" ");
  Serial.println(currentPulseUs);
}

// The entire servo protocol: a pulse of a certain length, fifty times a second. No address, no acknowledgement, no
// feedback. The servo has a potentiometer on its output shaft and turns the motor until that matches the pulse.
void writePulse(int pulseUs) {
  uint32_t duty = (uint32_t)((float)pulseUs * (float)(1UL << SERVO_RESOLUTION) / (float)PERIOD_US);
  ledcWrite(SERVO_PIN, duty);
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
