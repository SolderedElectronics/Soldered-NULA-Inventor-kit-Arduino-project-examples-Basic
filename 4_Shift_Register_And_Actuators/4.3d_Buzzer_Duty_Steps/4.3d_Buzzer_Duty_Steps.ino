/**
 **************************************************
 *
 * @file        4.3d_Buzzer_Duty_Steps.ino
 * @brief       Holds a passive buzzer at its resonant frequency and steps the duty cycle through 5, 10, 25 and 50 %.
 *              For details, connection diagram and more, check out the example documentation at: <link placeholder>
 * @author      Borna Zelenika @ soldered.com
 ***************************************************
 */

// IO48 drives the base of the transistor through a 10 kOhm resistor, as before.
const int DRIVE_PIN = 48;

// The resonant frequency you measured in T17.
const int RESONANT_HZ = 2700;

const int PWM_RESOLUTION = 8;

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();

  ledcAttach(DRIVE_PIN, 1000, PWM_RESOLUTION);

  // ledcWriteTone() also forces the duty back to 50 %, so it comes first and the duty is set after it.
  ledcWriteTone(DRIVE_PIN, RESONANT_HZ);
  ledcWrite(DRIVE_PIN, 0);
}

// Frequency decides the pitch; duty decides how long the coil is energised each cycle, so roughly how hard the disc
// is pushed. It gets louder as the duty rises, and the transistor gets warmer with it.
//
// The duty number is the percentage out of 255: 5 % is 12, 10 % is 25, 25 % is 63 and 50 % is 127.
void loop() {
  Serial.print("duty ");
  Serial.print(5);
  Serial.println(" %");
  ledcWrite(DRIVE_PIN, 12);
  delay(1500);

  Serial.print("duty ");
  Serial.print(10);
  Serial.println(" %");
  ledcWrite(DRIVE_PIN, 25);
  delay(1500);

  Serial.print("duty ");
  Serial.print(25);
  Serial.println(" %");
  ledcWrite(DRIVE_PIN, 63);
  delay(1500);

  Serial.print("duty ");
  Serial.print(50);
  Serial.println(" %");
  ledcWrite(DRIVE_PIN, 127);
  delay(1500);

  // Stop it with a duty of zero, not a level on the pin. A steady level is the silent case from the first sketch,
  // and it draws current through the coil until something gets warm.
  ledcWrite(DRIVE_PIN, 0);
  delay(1500);
}
