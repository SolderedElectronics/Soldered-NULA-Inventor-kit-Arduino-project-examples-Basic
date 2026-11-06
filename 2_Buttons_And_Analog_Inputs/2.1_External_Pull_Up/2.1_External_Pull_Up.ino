/**
 **************************************************
 *
 * @file        2.1_External_Pull_Up.ino
 * @brief       Reads a button on a pin held HIGH by a 10 kOhm pull-up resistor to 3V3.
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

  // The 10 kOhm to 3V3 holds the pin HIGH; pressing connects it straight to GND, which wins. The resistor is
  // what stops that being a short circuit between 3V3 and GND while the button is held.
  pinMode(BUTTON_PIN, INPUT);
  Serial.println("EXTERNAL PULL-UP: HIGH at rest, LOW while pressed.");
}

void loop() {
  // Read the pin, show the reading on the LED and print it, then wait a moment.
  bool reading = digitalRead(BUTTON_PIN);

  digitalWrite(LED_PIN, reading);
  Serial.println(reading);

  delay(20);
}
