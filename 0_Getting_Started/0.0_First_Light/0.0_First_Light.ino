/**
 **************************************************
 *
 * @file        0.0_First_Light.ino
 * @brief       The smallest thing the board can do: light the RGB LED that is already on it. Nothing is wired, so
 *              this proves the cable, the driver, the port and the upload all work before any circuit exists.
 *              For details, connection diagram and more, check out the example documentation at: <link placeholder>
 * @author      Soldered Electronics
 ***************************************************
 */

/*
There is an RGB LED on the board itself, on IO2. You do not wire it and you cannot see it on the header, because it
is part of the board. It is the one thing you can make happen with the USB-C cable and nothing else.
*/

// How long each colour stays on, in milliseconds.
const int HOLD_MS = 800;

// A quarter brightness. This LED is uncomfortable to look at wide open.
const uint8_t LEVEL = 64;

// setup() runs once, right after the board powers on or resets
void setup() {
  // Nothing to set up. rgbLedWrite() takes care of the LED on its own.
}

// loop() runs over and over, as long as the board is powered
void loop() {
  // rgbLedWrite() takes a brightness for red, green and blue, from 0 to 255.
  // RGB_BUILTIN is the board's own LED, so you do not need to know which pin it is on.
  rgbLedWrite(RGB_BUILTIN, LEVEL, 0, 0);  // red
  delay(HOLD_MS);

  rgbLedWrite(RGB_BUILTIN, 0, LEVEL, 0);  // green
  delay(HOLD_MS);

  rgbLedWrite(RGB_BUILTIN, 0, 0, LEVEL);  // blue
  delay(HOLD_MS);

  // All three channels off, so there is a clear gap before the next red.
  rgbLedWrite(RGB_BUILTIN, 0, 0, 0);
  delay(HOLD_MS);
}
