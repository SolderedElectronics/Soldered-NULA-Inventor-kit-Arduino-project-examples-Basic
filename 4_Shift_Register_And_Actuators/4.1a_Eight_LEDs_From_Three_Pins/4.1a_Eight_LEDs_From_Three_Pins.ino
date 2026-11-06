/**
 **************************************************
 *
 * @file        4.1a_Eight_LEDs_From_Three_Pins.ino
 * @brief       Drives eight LEDs from a 74HC595, times three different ways of getting a byte into it, and
 *              then walks a single lit bit across the row.
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

// How many times each of the three ways sends the byte while it is being timed.
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

  // Same circuit, same result, three ways of sending the byte, and the gap between slowest and fastest is large.
  // How you talk to a part is a design decision with a number attached. Every way below sends the same byte,
  // 0b10101010, and each one is repeated TIMING_UPDATES times while the clock runs.
  Serial.println();

  // Way 1, by hand. A shift register is a row of eight memory cells: put a bit on the data pin, pulse the clock,
  // and every cell hands its contents to the next while the first takes the new bit. Eight bits means eight of
  // those steps, written out here from the first bit to the last: HIGH, LOW, HIGH, LOW and so on.
  //
  // Nothing appears on the outputs while this happens. They are fed from a second set of latches that only copy
  // the row when the latch pin is pulsed, which is the last two lines.
  unsigned long start = micros();
  for (long i = 0; i < TIMING_UPDATES; i++) {
    digitalWrite(DATA_PIN, HIGH);
    digitalWrite(CLOCK_PIN, HIGH);
    digitalWrite(CLOCK_PIN, LOW);

    digitalWrite(DATA_PIN, LOW);
    digitalWrite(CLOCK_PIN, HIGH);
    digitalWrite(CLOCK_PIN, LOW);

    digitalWrite(DATA_PIN, HIGH);
    digitalWrite(CLOCK_PIN, HIGH);
    digitalWrite(CLOCK_PIN, LOW);

    digitalWrite(DATA_PIN, LOW);
    digitalWrite(CLOCK_PIN, HIGH);
    digitalWrite(CLOCK_PIN, LOW);

    digitalWrite(DATA_PIN, HIGH);
    digitalWrite(CLOCK_PIN, HIGH);
    digitalWrite(CLOCK_PIN, LOW);

    digitalWrite(DATA_PIN, LOW);
    digitalWrite(CLOCK_PIN, HIGH);
    digitalWrite(CLOCK_PIN, LOW);

    digitalWrite(DATA_PIN, HIGH);
    digitalWrite(CLOCK_PIN, HIGH);
    digitalWrite(CLOCK_PIN, LOW);

    digitalWrite(DATA_PIN, LOW);
    digitalWrite(CLOCK_PIN, HIGH);
    digitalWrite(CLOCK_PIN, LOW);

    digitalWrite(LATCH_PIN, HIGH);
    digitalWrite(LATCH_PIN, LOW);
  }
  unsigned long elapsed = micros() - start;
  long updatesPerSecond = (long)(TIMING_UPDATES * 1000000.0 / elapsed);

  Serial.print("manual pin writes: ");
  Serial.print(updatesPerSecond);
  Serial.println(" updates per second");

  // Way 2, with shiftOut(). The same eight steps, written for you, and it calls digitalWrite() exactly as the
  // lines above do. Convenience, not speed, so expect its number to land close.
  start = micros();
  for (long i = 0; i < TIMING_UPDATES; i++) {
    shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, 0b10101010);

    digitalWrite(LATCH_PIN, HIGH);
    digitalWrite(LATCH_PIN, LOW);
  }
  elapsed = micros() - start;
  updatesPerSecond = (long)(TIMING_UPDATES * 1000000.0 / elapsed);

  Serial.print("shiftOut():        ");
  Serial.print(updatesPerSecond);
  Serial.println(" updates per second");

  // Way 3, with hardware SPI. The chip's hardware generates the clock, so the processor hands over one byte and
  // is free until it is done.
  //
  // This has to come last. SPI.begin() takes the data and clock pins away from the GPIO driver, and after that a
  // digitalWrite() to either does nothing. There is no way back inside one run.
  SPI.begin(CLOCK_PIN, -1, DATA_PIN, -1);
  SPI.setFrequency(8000000);

  start = micros();
  for (long i = 0; i < TIMING_UPDATES; i++) {
    SPI.transfer(0b10101010);

    digitalWrite(LATCH_PIN, HIGH);
    digitalWrite(LATCH_PIN, LOW);
  }
  elapsed = micros() - start;
  updatesPerSecond = (long)(TIMING_UPDATES * 1000000.0 / elapsed);

  Serial.print("hardware SPI:      ");
  Serial.print(updatesPerSecond);
  Serial.println(" updates per second");

  Serial.println();
}

// Clock one lit bit along the row slowly enough to follow with your eye. The timing run handed the data and clock
// pins to the SPI hardware, so the walk sends its bytes with SPI from here on. The LEDs cannot tell the difference.
//
// Each byte has a single bit set: 1 is 0b00000001, 2 is 0b00000010, 4 is 0b00000100 and so on up to 128. The last
// bit sent lands on QA, so QA is the least significant bit and QH the most significant.
void loop() {
  SPI.transfer(1);
  digitalWrite(LATCH_PIN, HIGH);
  digitalWrite(LATCH_PIN, LOW);
  delay(150);

  SPI.transfer(2);
  digitalWrite(LATCH_PIN, HIGH);
  digitalWrite(LATCH_PIN, LOW);
  delay(150);

  SPI.transfer(4);
  digitalWrite(LATCH_PIN, HIGH);
  digitalWrite(LATCH_PIN, LOW);
  delay(150);

  SPI.transfer(8);
  digitalWrite(LATCH_PIN, HIGH);
  digitalWrite(LATCH_PIN, LOW);
  delay(150);

  SPI.transfer(16);
  digitalWrite(LATCH_PIN, HIGH);
  digitalWrite(LATCH_PIN, LOW);
  delay(150);

  SPI.transfer(32);
  digitalWrite(LATCH_PIN, HIGH);
  digitalWrite(LATCH_PIN, LOW);
  delay(150);

  SPI.transfer(64);
  digitalWrite(LATCH_PIN, HIGH);
  digitalWrite(LATCH_PIN, LOW);
  delay(150);

  SPI.transfer(128);
  digitalWrite(LATCH_PIN, HIGH);
  digitalWrite(LATCH_PIN, LOW);
  delay(150);
}
