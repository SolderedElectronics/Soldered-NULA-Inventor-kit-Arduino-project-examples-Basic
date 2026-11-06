/**
 **************************************************
 *
 * @file        4.3c_Buzzer_Held_Steady.ino
 * @brief       Drives the pin of a passive buzzer's transistor steadily high, to show that a steady voltage makes no sound.
 *              For details, connection diagram and more, check out the example documentation at: <link placeholder>
 * @author      Borna Zelenika @ soldered.com
 ***************************************************
 */

// IO48 drives the base of the transistor through a 10 kOhm resistor, as before.
const int DRIVE_PIN = 48;

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();

  // A passive buzzer is a coil and a metal disc with no oscillator inside, so it needs an alternating signal. A
  // steady high pushes the disc once and holds it: a faint click, then silence, with current flowing throughout.
  pinMode(DRIVE_PIN, OUTPUT);
  digitalWrite(DRIVE_PIN, HIGH);
  Serial.println("LEVEL: a steady HIGH. Silence, and current flowing. Do not leave this running.");
}

void loop() {
  delay(1000);
}
