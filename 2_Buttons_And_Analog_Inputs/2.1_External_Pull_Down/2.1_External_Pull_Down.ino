/**
 **************************************************
 *
 * @file        2.1_External_Pull_Down.ino
 * @brief       Reads a button on a pin held LOW by a 10 kOhm pull-down resistor to GND.
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

  // Same resistor, other end of the circuit. Nothing about the button changed: the resistor decides the
  // resting state, not the button.
  pinMode(BUTTON_PIN, INPUT);
  Serial.println("EXTERNAL PULL-DOWN: LOW at rest, HIGH while pressed. The logic is inverted.");
}

void loop() {
  // Read the pin, show the reading on the LED and print it, then wait a moment.
  bool reading = digitalRead(BUTTON_PIN);

  digitalWrite(LED_PIN, reading);
  Serial.println(reading);

  delay(20);
}
