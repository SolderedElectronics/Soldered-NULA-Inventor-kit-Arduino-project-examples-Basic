/**
 **************************************************
 *
 * @file        2.1_Button_Bounce.ino
 * @brief       Samples a button every 50 us after a press and prints how long it rattles for.
 *              For details, connection diagram and more, check out the example documentation at: <link placeholder>
 * @author      Borna Zelenika @ soldered.com
 ***************************************************
 */

const int BUTTON_PIN = 21;

// 400 readings, one every 50 us, make 20 ms. That is fine enough to see each make-and-break of the contacts.
// The extra slot at the end holds the 0 that marks the end of the text.
char trace[401];

void setup() {
  Serial.begin(115200);
  delay(1000);

  // The pin is held HIGH by the pull-up inside the chip, so a press pulls it LOW.
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  Serial.println("BOUNCE: press the button. The trace below is your button, sampled every 50 us.");
}

void loop() {
  // Wait for the line to go low, which is the moment the contacts first touch.
  if (digitalRead(BUTTON_PIN) != LOW) {
    return;
  }

  // Take 400 readings, one every 50 us, and store each one as the character '1' or '0'.
  // Nothing else may happen in here, not even a print, or the timing stops being true.
  for (int i = 0; i < 400; i++) {
    if (digitalRead(BUTTON_PIN) == HIGH) {
      trace[i] = '1';
    } else {
      trace[i] = '0';
    }
    delayMicroseconds(50);
  }
  trace[400] = 0;

  // The last reading is where the button settled. Walk through the trace and remember the last place
  // that disagreed with it. That is where the bounce ended.
  char settled = trace[399];
  int lastDifferent = -1;
  for (int i = 0; i < 400; i++) {
    if (trace[i] != settled) {
      lastDifferent = i;
    }
  }

  // Each reading is 50 us, which is 0.05 ms.
  float bounceMs = (lastDifferent + 1) * 0.05;

  Serial.println();
  Serial.println(trace);
  Serial.print("Bounce lasted about ");
  Serial.print(bounceMs, 2);
  Serial.println(" ms");

  // Wait for release before arming again.
  while (digitalRead(BUTTON_PIN) == LOW) {
    delay(10);
  }
  delay(100);
}
