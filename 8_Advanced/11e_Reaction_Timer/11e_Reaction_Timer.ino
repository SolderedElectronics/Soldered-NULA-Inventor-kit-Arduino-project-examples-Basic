/**
 **************************************************
 *
 * @file        11e_Reaction_Timer.ino
 * @brief       A reaction timer game. Press the button to start a round. The LED stays dark for a random time
 *              between 1.5 and 4 seconds, then lights and the buzzer chirps, and you press the button as fast as
 *              you can. The sketch prints your reaction in milliseconds, your best time of the session and the
 *              average of your last five rounds. A press before the LED lights is a false start and voids the round.
 *              For details, connection diagram and more, check out the example documentation at: <link placeholder>
 * @author      Borna Zelenika @ soldered.com
 ***************************************************
 */

// The button is on IO21 with the internal pull-up, the LED on IO38 and the buzzer's transistor on IO48.
const int BUTTON_PIN = 21;
const int LED_PIN = 38;
const int BUZZER_PIN = 48;

// From T17. Yours will be a different number.
const int BUZZER_HZ = 2700;

// The LED stays dark for a random time between these two limits, in milliseconds.
const unsigned long MIN_WAIT_MS = 1500;
const unsigned long MAX_WAIT_MS = 4000;

// A round with no press this long after the LED lit is void.
const unsigned long TOO_SLOW_MS = 2000;

// How long the result stays on the screen before the next round can start, in milliseconds.
const unsigned long RESULT_HOLD_MS = 1500;

// After the first edge of a press, further edges for this long are contact bounce and are ignored.
const unsigned long BOUNCE_MS = 30;

// The ring buffer holds the last five reaction times.
const int HISTORY_SIZE = 5;

// The four states of a round. Only one is active at a time.
enum State {
  STATE_IDLE,     // waiting for a press to start a round
  STATE_WAITING,  // LED dark, the random delay is running
  STATE_GO,       // LED lit, the timer is running
  STATE_RESULT    // the round is over, a short pause before the next one
};

State state = STATE_IDLE;

// The moment the current state began, from millis().
unsigned long stateStartMs = 0;

// The random delay chosen for this round, in milliseconds.
unsigned long waitMs = 0;

// The ring buffer of recent reactions, where the next one goes, and how many slots hold a real value so far.
unsigned long recent[HISTORY_SIZE];
int nextSlot = 0;
int recentCount = 0;

// The best reaction of the session. Zero means there is none yet.
unsigned long bestMs = 0;
int roundsDone = 0;

// State for the button and for the buzzer's beep sequence.
bool lastReading = HIGH;
unsigned long lastPressMs = 0;

int beepsLeft = 0;
bool beepSounding = false;
unsigned long beepChangeMs = 0;
unsigned long beepOnMs = 0;
unsigned long beepGapMs = 0;

// Returns true once for each press, at the first edge. The reaction is timed from that edge, so the sketch does not
// wait for the contact to settle before it takes the time.
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

// Starts a series of beeps without waiting. updateBeeps() plays it out on later passes of loop().
void startBeeps(int count, unsigned long onMs, unsigned long gapMs) {
  beepsLeft = count;
  beepOnMs = onMs;
  beepGapMs = gapMs;
  beepSounding = true;
  beepChangeMs = millis();
  ledcWriteTone(BUZZER_PIN, BUZZER_HZ);
}

// Switches the buzzer on and off in step with the series that startBeeps() began.
void updateBeeps() {
  if (beepsLeft == 0) {
    return;
  }

  unsigned long now = millis();

  if (beepSounding) {
    if (now - beepChangeMs >= beepOnMs) {
      ledcWrite(BUZZER_PIN, 0);
      beepSounding = false;
      beepChangeMs = now;
      beepsLeft--;
    }
  } else if (now - beepChangeMs >= beepGapMs) {
    ledcWriteTone(BUZZER_PIN, BUZZER_HZ);
    beepSounding = true;
    beepChangeMs = now;
  }
}

// Moves to a new state and does what that state needs once, on the way in. The time of the move is kept in
// stateStartMs, and for STATE_GO that time is the start of the reaction.
void enterState(State next) {
  state = next;
  stateStartMs = millis();

  switch (next) {
    case STATE_IDLE:
      digitalWrite(LED_PIN, LOW);
      break;

    case STATE_WAITING:
      digitalWrite(LED_PIN, LOW);
      waitMs = random(MIN_WAIT_MS, MAX_WAIT_MS + 1);
      Serial.println("get ready, press the button the moment the LED lights");
      break;

    case STATE_GO:
      digitalWrite(LED_PIN, HIGH);
      startBeeps(1, 150, 0);
      break;

    case STATE_RESULT:
      digitalWrite(LED_PIN, LOW);
      break;
  }
}

// Stores one reaction in the ring buffer, updates the best time and prints the round.
void recordReaction(unsigned long reactionMs) {
  // The new value goes over the oldest one once the buffer is full. The % wraps the slot number back to 0.
  recent[nextSlot] = reactionMs;
  nextSlot = (nextSlot + 1) % HISTORY_SIZE;
  if (recentCount < HISTORY_SIZE) {
    recentCount++;
  }

  roundsDone++;
  if (bestMs == 0 || reactionMs < bestMs) {
    bestMs = reactionMs;
  }

  // The average covers only the slots that hold a real value.
  unsigned long total = 0;
  for (int i = 0; i < recentCount; i++) {
    total += recent[i];
  }
  unsigned long average = total / recentCount;

  Serial.printf("round %d: %lu ms   best %lu ms   average of last %d: %lu ms\n", roundsDone, reactionMs, bestMs,
                recentCount, average);
}

// setup() runs once, right after the board powers on or resets.
void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // The buzzer pin is set up once. Frequencies are set later with ledcWriteTone().
  ledcAttach(BUZZER_PIN, BUZZER_HZ, 8);
  ledcWrite(BUZZER_PIN, 0);

  Serial.println();
  Serial.println("reaction timer: press the button to start a round");
  enterState(STATE_IDLE);
}

void loop() {
  bool pressed = buttonPressed();
  updateBeeps();

  switch (state) {
    case STATE_IDLE:
      if (pressed) {
        enterState(STATE_WAITING);
      }
      break;

    case STATE_WAITING:
      if (pressed) {
        // A press while the LED is still dark is a false start.
        Serial.println("false start, round void");
        startBeeps(2, 80, 80);
        enterState(STATE_RESULT);
      } else if (millis() - stateStartMs >= waitMs) {
        enterState(STATE_GO);
      }
      break;

    case STATE_GO:
      if (pressed) {
        // stateStartMs holds the moment the LED lit, so the difference is the reaction.
        recordReaction(millis() - stateStartMs);
        enterState(STATE_RESULT);
      } else if (millis() - stateStartMs >= TOO_SLOW_MS) {
        Serial.println("too slow, round void");
        enterState(STATE_RESULT);
      }
      break;

    case STATE_RESULT:
      // Presses are ignored here, so a button held down cannot start the next round by accident.
      if (millis() - stateStartMs >= RESULT_HOLD_MS) {
        Serial.println("press the button to start a round");
        enterState(STATE_IDLE);
      }
      break;
  }
}
