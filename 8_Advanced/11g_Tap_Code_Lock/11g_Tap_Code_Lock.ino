/**
 **************************************************
 *
 * @file        11g_Tap_Code_Lock.ino
 * @brief       A tap code lock with a servo latch. The secret is three taps on the button with a short gap and then a
 *              long gap between them. The sketch records the taps, waits 1.5 seconds after the last one and compares
 *              the pattern with the code. A right code turns the servo to the open position for 5 seconds. A wrong
 *              one gives a long low tone, and three wrong tries in a row lock the button for 10 seconds while the LED
 *              blinks fast.
 *              For details, connection diagram and more, check out the example documentation at: <link placeholder>
 * @author      Borna Zelenika @ soldered.com
 ***************************************************
 */

// The button is on IO21 with the internal pull-up, the LED on IO38, the servo on IO47 and the buzzer's transistor on
// IO48.
const int BUTTON_PIN = 21;
const int LED_PIN = 38;
const int SERVO_PIN = 47;
const int BUZZER_PIN = 48;

// From T17. Yours will be a different number. A wrong code plays the lower pitch.
const int BUZZER_HZ = 2700;
const int BUZZER_LOW_HZ = 800;

// From T19. Yours will be different numbers. The servo only turns, nothing is mounted on it, so the arm position is
// what you watch.
const int LOCKED_PULSE_US = 1000;
const int OPEN_PULSE_US = 2000;
const int SERVO_FREQ = 50;
const int SERVO_RESOLUTION = 14;
const long PERIOD_US = 20000;

// A gap shorter than SHORT_GAP_MS counts as short and one longer than LONG_GAP_MS counts as long. A gap in between is
// unclear and never matches the code.
const unsigned long SHORT_GAP_MS = 400;
const unsigned long LONG_GAP_MS = 700;

// This long after the last tap, the attempt is over and gets judged.
const unsigned long TAP_TIMEOUT_MS = 1500;

const unsigned long OPEN_MS = 5000;
const unsigned long LOCKOUT_MS = 10000;
const int MAX_WRONG_IN_A_ROW = 3;

// After the first edge of a press, further edges for this long are contact bounce and are ignored.
const unsigned long BOUNCE_MS = 30;

// The kinds of gap between two taps.
enum Gap {
  GAP_SHORT,
  GAP_LONG,
  GAP_UNCLEAR
};

// The secret code as a list of gaps. Three taps make two gaps: short, then long. Change the list to change the code.
const Gap SECRET_CODE[] = { GAP_SHORT, GAP_LONG };
const int CODE_GAPS = 2;

// The attempt records up to this many taps. Anything beyond that still counts as a tap but is not stored.
const int MAX_TAPS = 8;

// The three states of the lock.
enum State {
  STATE_LOCKED,
  STATE_OPEN,
  STATE_LOCKOUT
};

State state = STATE_LOCKED;
unsigned long stateStartMs = 0;

// The attempt in progress: the gap after each tap, how many taps so far, and when the last one was.
Gap gaps[MAX_TAPS - 1];
int tapCount = 0;
unsigned long lastTapMs = 0;

int attemptNumber = 0;
int wrongInARow = 0;

// State for the button and for the buzzer's tone.
bool lastReading = HIGH;
unsigned long lastPressMs = 0;
bool toneSounding = false;
unsigned long toneEndMs = 0;

// Returns true once for each press, at the first edge.
bool buttonPressed() {
  bool reading = digitalRead(BUTTON_PIN);
  unsigned long now = millis();

  bool pressed = (lastReading == HIGH && reading == LOW && now - lastPressMs > BOUNCE_MS);
  if (pressed) {
    lastPressMs = now;
  }

  lastReading = reading;
  return pressed;
}

// Plays a tone without waiting. updateTone() ends it.
void playTone(int hz, unsigned long durationMs) {
  ledcWriteTone(BUZZER_PIN, hz);
  toneSounding = true;
  toneEndMs = millis() + durationMs;
}

// Silences the buzzer when the tone's time is up.
void updateTone() {
  if (toneSounding && (long)(millis() - toneEndMs) >= 0) {
    ledcWrite(BUZZER_PIN, 0);
    toneSounding = false;
  }
}

// Turns the servo arm to a pulse width. The duty is the fraction of one 20 ms period that the pulse is high, scaled
// to the 14 bit range.
void moveServo(int pulseUs) {
  uint32_t duty = (uint32_t)((float)pulseUs * (float)(1UL << SERVO_RESOLUTION) / (float)PERIOD_US);
  ledcWrite(SERVO_PIN, duty);
}

// Sorts the time between two taps into short, long or unclear.
Gap classifyGap(unsigned long gapMs) {
  if (gapMs < SHORT_GAP_MS) {
    return GAP_SHORT;
  }
  if (gapMs > LONG_GAP_MS) {
    return GAP_LONG;
  }
  return GAP_UNCLEAR;
}

// Returns the word for a gap, for printing.
const char *gapName(Gap gap) {
  if (gap == GAP_SHORT) {
    return "short";
  }
  if (gap == GAP_LONG) {
    return "long";
  }
  return "unclear";
}

// Records one tap. From the second tap on, the time since the previous tap is sorted into a gap and stored.
void recordTap() {
  unsigned long now = millis();

  if (tapCount > 0 && tapCount <= MAX_TAPS - 1) {
    gaps[tapCount - 1] = classifyGap(now - lastTapMs);
  }

  tapCount++;
  lastTapMs = now;

  // A very short tick tells the player that the tap was heard.
  playTone(BUZZER_HZ, 25);
}

// Returns true when the attempt has the right number of taps and every gap matches the code in order.
bool matchesCode() {
  if (tapCount != CODE_GAPS + 1) {
    return false;
  }

  for (int i = 0; i < CODE_GAPS; i++) {
    if (gaps[i] != SECRET_CODE[i]) {
      return false;
    }
  }

  return true;
}

// Moves to a new state and does what that state needs once, on the way in.
void enterState(State next) {
  state = next;
  stateStartMs = millis();

  switch (next) {
    case STATE_LOCKED:
      moveServo(LOCKED_PULSE_US);
      digitalWrite(LED_PIN, LOW);
      break;

    case STATE_OPEN:
      moveServo(OPEN_PULSE_US);
      digitalWrite(LED_PIN, HIGH);
      break;

    case STATE_LOCKOUT:
      moveServo(LOCKED_PULSE_US);
      wrongInARow = 0;
      break;
  }
}

// Judges the attempt that just ended: prints it, then opens the lock, counts a wrong try or starts the lockout.
void judgeAttempt() {
  attemptNumber++;
  bool correct = matchesCode();

  // Print how many taps were heard and the gaps between them.
  Serial.printf("attempt %d: taps %d, gaps ", attemptNumber, tapCount);
  int storedGaps = min(tapCount - 1, MAX_TAPS - 1);
  if (storedGaps < 1) {
    Serial.print("none");
  }
  for (int i = 0; i < storedGaps; i++) {
    Serial.print(i > 0 ? " " : "");
    Serial.print(gapName(gaps[i]));
  }

  // The attempt is over, so the next tap starts a new one.
  tapCount = 0;

  if (correct) {
    wrongInARow = 0;
    Serial.println(", correct, open for 5 s");
    playTone(BUZZER_HZ, 300);
    enterState(STATE_OPEN);
    return;
  }

  wrongInARow++;
  Serial.printf(", wrong code (%d wrong in a row)\n", wrongInARow);
  playTone(BUZZER_LOW_HZ, 600);

  if (wrongInARow >= MAX_WRONG_IN_A_ROW) {
    Serial.println("lockout: 3 wrong tries in a row, locked for 10 s");
    enterState(STATE_LOCKOUT);
  }
}

// setup() runs once, right after the board powers on or resets.
void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);

  ledcAttach(SERVO_PIN, SERVO_FREQ, SERVO_RESOLUTION);
  ledcAttach(BUZZER_PIN, BUZZER_HZ, 8);
  ledcWrite(BUZZER_PIN, 0);

  Serial.println();
  Serial.println("tap code lock ready: tap short, then long, on the button to open it");
  enterState(STATE_LOCKED);
}

void loop() {
  bool pressed = buttonPressed();
  unsigned long now = millis();

  switch (state) {
    case STATE_LOCKED:
      if (pressed) {
        recordTap();
      }

      // Once 1.5 s pass with no new tap, the attempt is judged.
      if (tapCount > 0 && now - lastTapMs >= TAP_TIMEOUT_MS) {
        judgeAttempt();
      }
      break;

    case STATE_OPEN:
      if (now - stateStartMs >= OPEN_MS) {
        Serial.println("locked again");
        enterState(STATE_LOCKED);
      }
      break;

    case STATE_LOCKOUT:
      // The LED blinks fast and the button is ignored until the time is up.
      digitalWrite(LED_PIN, ((now / 100) % 2) == 0 ? HIGH : LOW);

      if (now - stateStartMs >= LOCKOUT_MS) {
        Serial.println("lockout over, ready for a new attempt");
        enterState(STATE_LOCKED);
      }
      break;
  }

  updateTone();
}
