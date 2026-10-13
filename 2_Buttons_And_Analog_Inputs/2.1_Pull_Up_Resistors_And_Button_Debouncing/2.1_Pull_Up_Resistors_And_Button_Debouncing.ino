/**
 **************************************************
 *
 * @file        2.1_Pull_Up_Resistors_And_Button_Debouncing.ino
 * @brief       One button read five ways: floating, external pull-up, external pull-down, internal pull-up, and a
 *              capture of the bounce itself. The code barely changes; the circuit does.
 *              For details, connection diagram and more, check out the example documentation at: <link placeholder>
 * @author      Soldered Electronics
 ***************************************************
 */

/*
Work through the modes in order. The code barely changes between them. What changes is the circuit, and that is the
point of the page, so each mode needs its own wiring before you upload it.

  MODE_FLOATING     no resistor holding the pin at all. Reads whatever is nearby.
  MODE_PULLUP_EXT   a resistor to 3V3 holds the pin HIGH at rest, so a press reads LOW.
  MODE_PULLDOWN_EXT a resistor to GND holds it LOW at rest, so the readings come out inverted.
  MODE_PULLUP_INT   the same pull-up, provided inside the chip instead of on the breadboard.
  MODE_BOUNCE       samples IO21 every 50 us and prints how long your button rattles for.
*/
#define MODE_FLOATING     0
#define MODE_PULLUP_EXT   1
#define MODE_PULLDOWN_EXT 2
#define MODE_PULLUP_INT   3
#define MODE_BOUNCE       4

const int MODE = MODE_FLOATING;

const int BUTTON_PIN = 21;
const int LED_PIN = 38;

// 50 us over 20 ms gives 400 readings, fine enough to see the individual make-and-break events of a contact.
const int BOUNCE_SAMPLES = 400;
const int BOUNCE_INTERVAL_US = 50;
char bounceTrace[BOUNCE_SAMPLES + 1];

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(LED_PIN, OUTPUT);

  switch (MODE) {
    case MODE_FLOATING:
      // Nothing is holding this pin anywhere, so it drifts and follows your hand. A pin with nothing attached
      // does not read LOW, it reads whatever it feels like.
      pinMode(BUTTON_PIN, INPUT);
      Serial.println("FLOATING: nothing is holding the pin. Move your hand near the wire.");
      break;

    case MODE_PULLUP_EXT:
      // The 10 kOhm to 3V3 holds the pin HIGH; pressing connects it straight to GND, which wins. The resistor is
      // what stops that being a short circuit between 3V3 and GND while the button is held.
      pinMode(BUTTON_PIN, INPUT);
      Serial.println("EXTERNAL PULL-UP: HIGH at rest, LOW while pressed.");
      break;

    case MODE_PULLDOWN_EXT:
      // Same resistor, other end of the circuit. Nothing about the button changed: the resistor decides the
      // resting state, not the button.
      pinMode(BUTTON_PIN, INPUT);
      Serial.println("EXTERNAL PULL-DOWN: LOW at rest, HIGH while pressed. The logic is inverted.");
      break;

    case MODE_PULLUP_INT:
    case MODE_BOUNCE:
      // The same pull-up, moved inside the chip. It is around 45 kOhm, weak next to a 10 kOhm, so over a long
      // wire it is slow to bring the line back up and easy for nearby noise to disturb.
      pinMode(BUTTON_PIN, INPUT_PULLUP);
      if (MODE == MODE_PULLUP_INT) {
        Serial.println("INTERNAL PULL-UP: no resistor on the board. Same readings as the external one.");
      } else {
        Serial.println("BOUNCE: press the button. The trace below is your button, sampled every 50 us.");
      }
      break;
  }
}

void loop() {
  switch (MODE) {
    case MODE_BOUNCE:
      captureBounce();
      break;
    default:
      showRawReading();
      break;
  }
}

void showRawReading() {
  bool reading = digitalRead(BUTTON_PIN);

  digitalWrite(LED_PIN, reading);
  Serial.println(reading);

  delay(20);
}

/*
A pushbutton is two pieces of springy metal, and they meet and part several times in the first few milliseconds
while the spring settles. The pin reads every one of those as a separate press. This waits for the first edge and
then samples flat out, so the trace is your button rather than somebody else's.
*/
void captureBounce() {
  // Wait for the line to go low, which is the moment the contacts first touch.
  if (digitalRead(BUTTON_PIN) != LOW) {
    return;
  }

  unsigned long nextUs = micros();
  for (int i = 0; i < BOUNCE_SAMPLES; i++) {
    // Busy-wait until this sample is due. Nothing else may happen in here, or the timing stops being true.
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

  // Round that up generously and it is your debounce interval. Then ignore further changes for that long rather
  // than calling delay(), which stops everything else in the program too.

  // Wait for release before arming again.
  while (digitalRead(BUTTON_PIN) == LOW) {
    delay(10);
  }
  delay(100);
}
