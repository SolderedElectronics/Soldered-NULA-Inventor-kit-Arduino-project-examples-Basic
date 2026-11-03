/**
 **************************************************
 *
 * @file        08c_Dim_Eight_LEDs_From_One_Pin.ino
 * @brief       Lights all eight LEDs on a 74HC595 and dims them together with a PWM signal on output enable,
 *              with a pot setting the level.
 *              For details, connection diagram and more, check out the example documentation at: <link placeholder>
 * @author      Borna Zelenika @ soldered.com
 ***************************************************
 */

/*
Find pin 1 first. It is at the end with the half-moon notch, left-hand pin with the notch at the top. Some packages
also carry a round dimple that is not next to the notch and is not pin 1 on this part.

Two things here are not optional and neither announces itself when missing. SRCLR must be held at 3V3 or the
register clears itself at random. And the 100 nF must sit as close to the chip's supply pins as the breadboard
allows, because eight outputs switching at once is a current spike the supply cannot follow.

Each of the eight outputs needs its own 330 Ohm resistor in series with its LED.
*/
const int DATA_PIN = 11;
const int CLOCK_PIN = 12;
const int LATCH_PIN = 10;
const int OE_PIN = 13;
const int POT_PIN = 4;

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(LATCH_PIN, OUTPUT);
  digitalWrite(LATCH_PIN, LOW);

  pinMode(DATA_PIN, OUTPUT);
  pinMode(CLOCK_PIN, OUTPUT);

  analogReadResolution(12);
  analogSetPinAttenuation(POT_PIN, ADC_11db);

  // Output enable gets a PWM signal instead of a steady level.
  ledcAttach(OE_PIN, 5000, 8);

  Serial.println();
  Serial.println("74HC595 on IO11 (data), IO12 (clock), IO10 (latch), IO13 (output enable)");
}

// Output enable does not change what is in the register, it decides whether the outputs are connected at all. Feed
// it PWM and all eight dim together for no processor time at all. The duty is inverted because the pin is
// active low: a duty of 255 would hold the outputs off permanently.
void loop() {
  // All eight on, so the dimming happens to the whole row and not to a pattern.
  writeManual(0b11111111);

  int pot = analogRead(POT_PIN);
  int brightness = map(pot, 0, 4095, 0, 255);

  ledcWrite(OE_PIN, 255 - brightness);

  delay(20);
}

/*
The manual version, one pin write at a time. A shift register is a row of eight memory cells: put a bit on the data
pin, pulse the clock, and every cell hands its contents to the next while the first takes the new bit.

Nothing appears on the outputs while this happens. They are fed from a second set of latches that only copy the row
when the latch pin is pulsed.
*/
void writeManual(uint8_t value) {
  for (int bit = 7; bit >= 0; bit--) {
    digitalWrite(DATA_PIN, (value >> bit) & 1);

    digitalWrite(CLOCK_PIN, HIGH);
    digitalWrite(CLOCK_PIN, LOW);
  }

  digitalWrite(LATCH_PIN, HIGH);
  digitalWrite(LATCH_PIN, LOW);
}
