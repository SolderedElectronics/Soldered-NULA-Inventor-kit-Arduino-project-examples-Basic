/**
 **************************************************
 *
 * @file        2.1_Floating_Button.ino
 * @brief       Reads a button on a pin that nothing is holding, so the reading drifts.
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

  // Nothing is holding this pin anywhere, so it drifts and follows your hand. A pin with nothing attached
  // does not read LOW, it reads whatever it feels like.
  pinMode(BUTTON_PIN, INPUT);
  Serial.println("FLOATING: nothing is holding the pin. Move your hand near the wire.");
}

void loop() {
  // Read the pin, show the reading on the LED and print it, then wait a moment.
  bool reading = digitalRead(BUTTON_PIN);

  digitalWrite(LED_PIN, reading);
  Serial.println(reading);

  delay(20);
}
