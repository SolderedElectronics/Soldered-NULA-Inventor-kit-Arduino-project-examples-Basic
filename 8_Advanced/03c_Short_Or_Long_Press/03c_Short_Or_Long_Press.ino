/**
 **************************************************
 *
 * @file        03c_Short_Or_Long_Press.ino
 * @brief       Measures how long a button is held and blinks the LED once or three times to match.
 *              For details, connection diagram and more, check out the example documentation at: <link placeholder>
 * @author      Borna Zelenika @ soldered.com
 ***************************************************
 */

const int BUTTON_PIN = 21;
const int LED_PIN = 38;

// A press held for at least this long counts as a long press.
const unsigned long LONG_PRESS_MS = 500;

// Ignore changes that come sooner than this after the last one, so the contact bounce is not counted.
const unsigned long DEBOUNCE_MS = 30;

bool wasPressed = false;
unsigned long pressedAtMs = 0;
unsigned long lastEdgeMs = 0;

// Blinks the LED a number of times, 150 ms on and 150 ms off each.
void blinkLed(int times) {
  for (int i = 0; i < times; i++) {
    digitalWrite(LED_PIN, HIGH);
    delay(150);
    digitalWrite(LED_PIN, LOW);
    delay(150);
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  Serial.println("PRESS LENGTH: tap the button for one blink, hold it for three.");
}

void loop() {
  // The pull-up makes a press read LOW, so a pressed button gives true here.
  bool pressed = (digitalRead(BUTTON_PIN) == LOW);

  if (pressed != wasPressed && millis() - lastEdgeMs >= DEBOUNCE_MS) {
    wasPressed = pressed;
    lastEdgeMs = millis();

    if (pressed) {
      // The button has just gone down: note the time and light the LED while it stays down.
      pressedAtMs = millis();
      digitalWrite(LED_PIN, HIGH);
    } else {
      // The button has just come up: work out how long it was held.
      unsigned long heldMs = millis() - pressedAtMs;
      digitalWrite(LED_PIN, LOW);

      Serial.print("Held for ");
      Serial.print(heldMs);
      Serial.println(" ms");

      blinkLed(heldMs >= LONG_PRESS_MS ? 3 : 1);
    }
  }
}
