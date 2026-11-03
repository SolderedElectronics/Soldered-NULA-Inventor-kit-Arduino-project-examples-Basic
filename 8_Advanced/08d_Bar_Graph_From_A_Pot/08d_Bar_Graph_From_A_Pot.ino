/**
 **************************************************
 *
 * @file        08d_Bar_Graph_From_A_Pot.ino
 * @brief       Fills the row of eight LEDs on a 74HC595 one at a time as the pot is turned, like a bar graph.
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

/*
A bar graph. The whole display is still one byte, so lighting five LEDs costs what lighting one costs.

Two habits from earlier are doing the work, and without them the LED at the boundary flickers: the EMA from T11
smooths the reading, and the hysteresis from T15 means a step only changes once the smoothed value has moved clear
of the edge by BAR_MARGIN counts.
*/
const int BAR_STEP = 4096 / 9;  // nine states, from no LEDs lit to all eight
const int BAR_MARGIN = 40;      // how far past an edge the pot has to go before the bar changes

float potEma = -1.0;
int barLevel = 0;

void setup() {
  pinMode(LATCH_PIN, OUTPUT);
  digitalWrite(LATCH_PIN, LOW);

  pinMode(DATA_PIN, OUTPUT);
  pinMode(CLOCK_PIN, OUTPUT);

  // Output enable is active low, so holding it LOW is what makes the LEDs visible.
  pinMode(OE_PIN, OUTPUT);
  digitalWrite(OE_PIN, LOW);

  analogReadResolution(12);
  analogSetPinAttenuation(POT_PIN, ADC_11db);
}

void loop() {
  int raw = analogRead(POT_PIN);

  // Seed the average on the first pass, or the bar crawls up from zero every time the board resets.
  if (potEma < 0) {
    potEma = raw;
  }
  // Faster than T11's 0.05, because a display the reader is turning has to keep up with their hand.
  potEma += 0.2 * (raw - potEma);

  if (barLevel < 8 && potEma > (long)(barLevel + 1) * BAR_STEP + BAR_MARGIN) {
    barLevel++;
  } else if (barLevel > 0 && potEma < (long)barLevel * BAR_STEP - BAR_MARGIN) {
    barLevel--;
  }

  // One shift left, minus one, turns a count into a run of lit bits: 3 becomes 0b00000111.
  writeManual((1 << barLevel) - 1);

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
