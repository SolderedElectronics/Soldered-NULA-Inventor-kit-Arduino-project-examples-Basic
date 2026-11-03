/**
 **************************************************
 *
 * @file        03b_Toggle_Button.ino
 * @brief       Toggles an LED on every clean press of a button, ignoring the contact bounce.
 *              For details, connection diagram and more, check out the example documentation at: <link placeholder>
 * @author      Borna Zelenika @ soldered.com
 ***************************************************
 */

const int BUTTON_PIN = 21;
const int LED_PIN = 38;

// A reading must stay the same for this long before the sketch believes it. Your own T9 result, rounded
// generously upwards, is the right number here. 30 ms covers almost every button.
const unsigned long DEBOUNCE_MS = 30;

// The most recent raw reading, the last one we accepted as real, and the moment the raw reading last changed.
bool lastReading = HIGH;
bool stableState = HIGH;
unsigned long lastChangeMs = 0;

// Whether the LED is currently on.
bool ledOn = false;

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  Serial.println("TOGGLE: press the button to switch the LED on, press again to switch it off.");
}

void loop() {
  bool reading = digitalRead(BUTTON_PIN);

  // Any change, including every bounce, restarts the stopwatch.
  if (reading != lastReading) {
    lastReading = reading;
    lastChangeMs = millis();
  }

  // Once the reading has held still for DEBOUNCE_MS and differs from the state we accepted last,
  // it is a real change.
  if (millis() - lastChangeMs >= DEBOUNCE_MS && reading != stableState) {
    stableState = reading;

    // The pull-up makes a press read LOW. Only the moment of pressing toggles the LED, not the release.
    if (stableState == LOW) {
      ledOn = !ledOn;
      digitalWrite(LED_PIN, ledOn);
      Serial.println(ledOn ? "LED on" : "LED off");
    }
  }
}
