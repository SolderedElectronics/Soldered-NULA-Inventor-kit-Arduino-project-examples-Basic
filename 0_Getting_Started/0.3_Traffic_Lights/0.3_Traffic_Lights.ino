/**
 **************************************************
 *
 * @file        0.3_Traffic_Lights.ino
 * @brief       A lit spot walking along five LEDs on IO38 to IO42. Each LED lights for half a second in turn, left to
 *              right, and then the row starts again.
 *              For details, connection diagram and more, check out the example documentation at: <link placeholder>
 * @author      Borna Zelenika @ soldered.com
 ***************************************************
 */

// The five LEDs, left to right as they sit on the breadboard, each with its own 330 Ohm resistor.
const int LED_1_PIN = 38;
const int LED_2_PIN = 39;
const int LED_3_PIN = 40;
const int LED_4_PIN = 41;
const int LED_5_PIN = 42;

// How long each LED stays in its state, in milliseconds.
const int STEP_MS = 500;

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("RUNNING: a lit spot walks along the row and starts again.");

  // Every one of the five pins is an output.
  pinMode(LED_1_PIN, OUTPUT);
  pinMode(LED_2_PIN, OUTPUT);
  pinMode(LED_3_PIN, OUTPUT);
  pinMode(LED_4_PIN, OUTPUT);
  pinMode(LED_5_PIN, OUTPUT);
}

void loop() {
  digitalWrite(LED_1_PIN, HIGH);
  delay(STEP_MS);
  digitalWrite(LED_1_PIN, LOW);

  digitalWrite(LED_2_PIN, HIGH);
  delay(STEP_MS);
  digitalWrite(LED_2_PIN, LOW);

  digitalWrite(LED_3_PIN, HIGH);
  delay(STEP_MS);
  digitalWrite(LED_3_PIN, LOW);

  digitalWrite(LED_4_PIN, HIGH);
  delay(STEP_MS);
  digitalWrite(LED_4_PIN, LOW);

  digitalWrite(LED_5_PIN, HIGH);
  delay(STEP_MS);
  digitalWrite(LED_5_PIN, LOW);
}
