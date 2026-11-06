/**
 **************************************************
 *
 * @file        2.1_Internal_Pull_Up.ino
 * @brief       Reads a button using the pull-up resistor that is built into the chip.
 *              For details, connection diagram and more, check out the example documentation at: <link placeholder>
 * @author      Borna Zelenika @ soldered.com
 ***************************************************
 */

const int BUTTON_PIN = 21;
const int LED_PIN = 38;

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(LED_PIN, OUTPUT);

  // The same pull-up, moved inside the chip. It is around 45 kOhm, weak next to a 10 kOhm, so over a long
  // wire it is slow to bring the line back up and easy for nearby noise to disturb.
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  Serial.println("INTERNAL PULL-UP: no resistor on the board. Same readings as the external one.");
}

void loop() {
  // Read the pin, show the reading on the LED and print it, then wait a moment.
  bool reading = digitalRead(BUTTON_PIN);

  digitalWrite(LED_PIN, reading);
  Serial.println(reading);

  delay(20);
}
