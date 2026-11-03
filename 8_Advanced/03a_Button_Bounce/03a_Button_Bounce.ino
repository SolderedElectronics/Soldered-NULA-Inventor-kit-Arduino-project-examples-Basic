/**
 **************************************************
 *
 * @file        03a_Button_Bounce.ino
 * @brief       Samples a button every 50 us after a press and prints how long it rattles for.
 *              For details, connection diagram and more, check out the example documentation at: <link placeholder>
 * @author      Borna Zelenika @ soldered.com
 ***************************************************
 */

const int BUTTON_PIN = 21;

// 50 us over 20 ms gives 400 readings, fine enough to see the individual make-and-break events of a contact.
const int BOUNCE_SAMPLES = 400;
const int BOUNCE_INTERVAL_US = 50;
char bounceTrace[BOUNCE_SAMPLES + 1];

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

  // Take 400 readings, one every 50 us. Nothing else may happen in here, or the timing stops being true.
  unsigned long nextUs = micros();
  for (int i = 0; i < BOUNCE_SAMPLES; i++) {
    while ((long)(micros() - nextUs) < 0) {
    }
    bounceTrace[i] = digitalRead(BUTTON_PIN) ? '1' : '0';
    nextUs += BOUNCE_INTERVAL_US;
  }
  bounceTrace[BOUNCE_SAMPLES] = 0;

  // The last sample that differs from the settled state is where the bounce ended.
  char settled = bounceTrace[BOUNCE_SAMPLES - 1];
  int lastDifferent = -1;
  for (int i = 0; i < BOUNCE_SAMPLES; i++) {
    if (bounceTrace[i] != settled) {
      lastDifferent = i;
    }
  }

  float bounceMs = (lastDifferent + 1) * BOUNCE_INTERVAL_US / 1000.0;

  Serial.println();
  Serial.println(bounceTrace);
  Serial.print("Bounce lasted about ");
  Serial.print(bounceMs, 2);
  Serial.println(" ms");

  // Wait for release before arming again.
  while (digitalRead(BUTTON_PIN) == LOW) {
    delay(10);
  }
  delay(100);
}
