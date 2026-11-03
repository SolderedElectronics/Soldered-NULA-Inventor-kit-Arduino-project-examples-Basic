/**
 **************************************************
 *
 * @file        08f_Two_Tone_Alarm.ino
 * @brief       Alternates a passive buzzer between two pitches like a siren, with a pot setting the loudness
 *              through the duty cycle.
 *              For details, connection diagram and more, check out the example documentation at: <link placeholder>
 * @author      Borna Zelenika @ soldered.com
 ***************************************************
 */

// IO48 drives the base of the transistor through the 10 kOhm resistor, and IO4 reads the pot, the volume knob.
const int DRIVE_PIN = 48;
const int POT_PIN = 4;

// The high pitch is the resonant frequency you measured in T22, where the buzzer is loudest. The low pitch is a
// third lower, which gives the familiar two-tone sound.
const int HIGH_HZ = 2700;
const int LOW_HZ = 1800;

// How long each pitch lasts before it swaps, in milliseconds.
const int TONE_MS = 400;

const int PWM_RESOLUTION = 8;

bool highTone = true;
unsigned long lastSwitchMs = 0;

void setup() {
  analogReadResolution(12);
  analogSetPinAttenuation(POT_PIN, ADC_11db);

  ledcAttach(DRIVE_PIN, 1000, PWM_RESOLUTION);
  ledcWriteTone(DRIVE_PIN, HIGH_HZ);
  ledcWrite(DRIVE_PIN, 0);
}

void loop() {
  // Swap pitch when the time is up. ledcWriteTone() also forces the duty back to 50 %, which the next lines undo.
  if (millis() - lastSwitchMs >= TONE_MS) {
    lastSwitchMs = millis();

    highTone = !highTone;
    ledcWriteTone(DRIVE_PIN, highTone ? HIGH_HZ : LOW_HZ);
  }

  // The knob sets the duty from 0 to 50 %, the range in which a bigger duty is a louder sound. Half of 255 is 127.
  int raw = analogRead(POT_PIN);
  int duty = (raw * 127L) / 4095;
  ledcWrite(DRIVE_PIN, duty);

  delay(5);
}
