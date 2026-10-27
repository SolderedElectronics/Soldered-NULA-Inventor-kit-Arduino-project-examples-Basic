/**
 **************************************************
 *
 * @file        0.1_First_Blink.ino
 * @brief       The first example of the kit. Blinks one LED on IO38 and prints one line to the Serial Monitor, so
 *              you can confirm the board, the driver and the toolchain all work before anything harder.
 *              For details, connection diagram and more, check out the example documentation at: <link placeholder>
 * @author      Borna Zelenika @ soldered.com
 ***************************************************
 */

// The LED needs a 330 Ohm resistor in series with it. Example 1.1 works out where that number comes from.
const int LED_PIN = 38;

// On and off time in milliseconds. 500 gives one full blink per second.
const int DELAY_MS = 500;

void setup() {
  // Set the Serial Monitor to 115200 as well, or you get nothing but rubbish. Every example uses this rate.
  Serial.begin(115200);

  // Time to open the Serial Monitor. The line below is printed once and you will miss it otherwise.
  delay(1000);

  Serial.println("NULA DeepSleep ESP32-S3 running. Blinking an LED on IO38.");

  pinMode(LED_PIN, OUTPUT);
}

void loop() {
  digitalWrite(LED_PIN, HIGH);
  delay(DELAY_MS);

  digitalWrite(LED_PIN, LOW);
  delay(DELAY_MS);
}
