/**
 **************************************************
 *
 * @file        0.1_Two_LEDs_Taking_Turns.ino
 * @brief       Two LEDs on IO38 and IO39 taking turns. One is always lit and the other is always dark, and they
 *              swap every half second.
 *              For details, connection diagram and more, check out the example documentation at: <link placeholder>
 * @author      Borna Zelenika @ soldered.com
 ***************************************************
 */

// The two LEDs, each with its own 330 Ohm resistor in series.
const int LED_A_PIN = 38;
const int LED_B_PIN = 39;

// How long one turn lasts, in milliseconds.
const int STEP_MS = 500;

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("ALTERNATE: two LEDs take turns. One is always lit.");

  pinMode(LED_A_PIN, OUTPUT);
  pinMode(LED_B_PIN, OUTPUT);
}

void loop() {
  // The second LED is always given the opposite of the first.
  digitalWrite(LED_A_PIN, HIGH);
  digitalWrite(LED_B_PIN, LOW);
  delay(STEP_MS);

  digitalWrite(LED_A_PIN, LOW);
  digitalWrite(LED_B_PIN, HIGH);
  delay(STEP_MS);
}
