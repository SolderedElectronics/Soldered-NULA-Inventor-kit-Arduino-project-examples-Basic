/**
 **************************************************
 *
 * @file        4.1a_Eight_LEDs_From_Three_Pins.ino
 * @brief       Drives eight LEDs from a 74HC595, times three different ways of getting a byte into it, and
 *              then breaks the latch on purpose.
 *              For details, connection diagram and more, check out the example documentation at: <link placeholder>
 * @author      Borna Zelenika @ soldered.com
 ***************************************************
 */

#include <SPI.h>

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

/*
  MODE_SPEED   times all three ways of sending a byte, then walks a single bit across the LEDs
  MODE_BROKEN  latching while shifting, so you can see what the latch is for
*/
#define MODE_SPEED  0
#define MODE_BROKEN 1

const int MODE = MODE_SPEED;

const long TIMING_UPDATES = 10000;

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(LATCH_PIN, OUTPUT);
  digitalWrite(LATCH_PIN, LOW);

  pinMode(DATA_PIN, OUTPUT);
  pinMode(CLOCK_PIN, OUTPUT);

  // Output enable is active low, so holding it LOW is what makes the LEDs visible.
  pinMode(OE_PIN, OUTPUT);
  digitalWrite(OE_PIN, LOW);

  Serial.println();
  Serial.println("74HC595 on IO11 (data), IO12 (clock), IO10 (latch), IO13 (output enable)");

  if (MODE == MODE_SPEED) {
    timeAllThree();
  }
}

void loop() {
  switch (MODE) {
    case MODE_SPEED:
      // The timing run handed the data and clock pins to the SPI peripheral, so the walk has to use the SPI writer
      // from here on. The LEDs cannot tell the difference.
      walkOneBit(writeSpi);
      break;
    case MODE_BROKEN:
      brokenMode();
      break;
  }
}

/*
The manual version, one pin write at a time, which is what every other method here does underneath. A shift
register is a row of eight memory cells: put a bit on the data pin, pulse the clock, and every cell hands its
contents to the next while the first takes the new bit.

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

// The same loop, written for you, and it calls digitalWrite() exactly as the manual version does. Convenience, not
// speed, so expect its number to land close.
void writeShiftOut(uint8_t value) {
  shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, value);

  digitalWrite(LATCH_PIN, HIGH);
  digitalWrite(LATCH_PIN, LOW);
}

// The same signalling again, except the chip's hardware generates the clock. The processor hands over one byte and
// is free until it is done.
void writeSpi(uint8_t value) {
  SPI.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE0));
  SPI.transfer(value);
  SPI.endTransaction();

  digitalWrite(LATCH_PIN, HIGH);
  digitalWrite(LATCH_PIN, LOW);
}

// Same circuit, same result, three ways of getting there, and the gap between slowest and fastest is large. How you
// talk to a part is a design decision with a number attached.
void timeAllThree() {
  Serial.println();

  Serial.print("manual pin writes: ");
  Serial.print(measure(writeManual));
  Serial.println(" updates per second");

  Serial.print("shiftOut():        ");
  Serial.print(measure(writeShiftOut));
  Serial.println(" updates per second");

  // SPI must come last. SPI.begin() takes the data and clock pins away from the GPIO driver, and after that a
  // digitalWrite() to either does nothing. There is no way back inside one run.
  SPI.begin(CLOCK_PIN, -1, DATA_PIN, -1);

  Serial.print("hardware SPI:      ");
  Serial.print(measure(writeSpi));
  Serial.println(" updates per second");

  Serial.println();
}

long measure(void (*writer)(uint8_t)) {
  unsigned long start = micros();
  for (long i = 0; i < TIMING_UPDATES; i++) {
    writer((uint8_t)i);
  }
  unsigned long elapsed = micros() - start;

  return (long)(TIMING_UPDATES * 1000000.0 / elapsed);
}

// Clock one lit bit along the row slowly enough to follow with your eye. With MSBFIRST the last bit shifted lands
// on QA, so QA is the least significant bit and QH the most significant.
void walkOneBit(void (*writer)(uint8_t)) {
  for (int i = 0; i < 8; i++) {
    writer(1 << i);
    delay(150);
  }
}

/*
The latch pulsed after every single bit instead of once at the end, so the outputs show the register mid-shift eight
times per byte. The LEDs blur and the pattern you asked for is never actually displayed.
*/
void brokenMode() {
  uint8_t value = 0b11110000;

  for (int bit = 7; bit >= 0; bit--) {
    digitalWrite(DATA_PIN, (value >> bit) & 1);
    digitalWrite(CLOCK_PIN, HIGH);
    digitalWrite(CLOCK_PIN, LOW);

    // The mistake, on purpose.
    digitalWrite(LATCH_PIN, HIGH);
    digitalWrite(LATCH_PIN, LOW);

    delay(120);
  }

  delay(400);
}
