/**
 **************************************************
 *
 * @file        11f_Light_Beam_Alarm.ino
 * @brief       A light-beam alarm with arming. The LED on IO38 shines on the photoresistor on IO4 and the pair is
 *              the beam. A short press of the button arms the alarm, with a 10 second countdown to leave. When the
 *              countdown ends the sketch measures the beam and from then on sounds a siren if the light on the sensor
 *              stays below 60 percent of that level for three reads in a row. Holding the button for one second
 *              disarms it. The onboard LED shows the state: green is disarmed, yellow is arming, red is alarm.
 *              For details, connection diagram and more, check out the example documentation at: <link placeholder>
 * @author      Borna Zelenika @ soldered.com
 ***************************************************
 */

// The button is on IO21 with the internal pull-up, the photoresistor divider on IO4, the beam LED on IO38 and the
// buzzer's transistor on IO48. The status LED is the one built into the board.
const int BUTTON_PIN = 21;
const int LIGHT_PIN = 4;
const int BEAM_LED_PIN = 38;
const int BUZZER_PIN = 48;

// From T17. Yours will be a different number. The siren warbles between this pitch and a lower one.
const int BUZZER_HZ = 2700;
const int BUZZER_LOW_HZ = 2200;

// How bright the status LED is, from 0 to 255. It is bright at full value, so it stays low.
const uint8_t STATUS_BRIGHTNESS = 32;

// The time to leave the room after arming, and how long the button must be held to count as a long press.
const unsigned long ARMING_MS = 10000;
const unsigned long LONG_PRESS_MS = 1000;

// After the first edge of a press or a release, further edges for this long are contact bounce and are ignored.
const unsigned long BOUNCE_MS = 30;

// The beam is read every 20 ms. Three low reads in a row, 60 ms, trigger the alarm, so a flicker does not.
const unsigned long SAMPLE_MS = 20;
const int LOW_READS_TO_TRIGGER = 3;

// The alarm triggers below this percentage of the calibrated beam level.
const int TRIGGER_PERCENT = 60;

// The calibration averages this many reads. A beam below MIN_BEAM_COUNTS means the LED is not aimed at the sensor.
const int CALIBRATION_READS = 16;
const int MIN_BEAM_COUNTS = 300;

// The siren changes pitch this often, in milliseconds.
const unsigned long WARBLE_MS = 200;

// The four states of the alarm. Only one is active at a time.
enum State {
  STATE_DISARMED,
  STATE_ARMING,
  STATE_ARMED,
  STATE_ALARM
};

// What the button just did. A short press is reported on release, a long press as soon as the second is up.
enum Press {
  NO_PRESS,
  SHORT_PRESS,
  LONG_PRESS
};

State state = STATE_DISARMED;
unsigned long stateStartMs = 0;

// The calibrated beam level, the level below which the alarm triggers, and the latest read.
int beamLevel = 0;
int triggerLevel = 0;
int lastLevel = 0;

unsigned long lastSampleMs = 0;
int lowReads = 0;

// State for the button, the arming chirp and the siren.
bool buttonDown = false;
bool longReported = false;
unsigned long buttonDownMs = 0;
unsigned long buttonUpMs = 0;

int lastChirpSecond = -1;
bool chirping = false;
unsigned long chirpEndMs = 0;
int sirenPitchHz = 0;

// Reads the button and reports a short or a long press. A press is accepted at its first edge, and edges that follow
// within BOUNCE_MS are ignored.
Press readButton() {
  bool isDown = (digitalRead(BUTTON_PIN) == LOW);
  unsigned long now = millis();

  if (isDown && !buttonDown) {
    // The button has just gone down, unless this is bounce right after a release.
    if (now - buttonUpMs > BOUNCE_MS) {
      buttonDown = true;
      buttonDownMs = now;
      longReported = false;
    }
  } else if (isDown && buttonDown) {
    // Still held. Report the long press once, the moment the second is up.
    if (!longReported && now - buttonDownMs >= LONG_PRESS_MS) {
      longReported = true;
      return LONG_PRESS;
    }
  } else if (!isDown && buttonDown) {
    // Released, unless this is bounce right after the press.
    if (now - buttonDownMs > BOUNCE_MS) {
      buttonDown = false;
      buttonUpMs = now;
      if (!longReported) {
        return SHORT_PRESS;
      }
    }
  }

  return NO_PRESS;
}

// Plays one short chirp without waiting. updateSound() ends it.
void startChirp(unsigned long durationMs) {
  ledcWriteTone(BUZZER_PIN, BUZZER_HZ);
  chirping = true;
  chirpEndMs = millis() + durationMs;
}

// Ends a chirp when its time is up, and warbles the siren while the alarm is on.
void updateSound() {
  unsigned long now = millis();

  if (state == STATE_ALARM) {
    // The pitch flips every WARBLE_MS. It is set only when it changes, so the tone is not restarted on every pass.
    int pitch = (((now / WARBLE_MS) % 2) == 0) ? BUZZER_HZ : BUZZER_LOW_HZ;
    if (pitch != sirenPitchHz) {
      sirenPitchHz = pitch;
      ledcWriteTone(BUZZER_PIN, pitch);
    }
    return;
  }

  if (chirping && (long)(now - chirpEndMs) >= 0) {
    ledcWrite(BUZZER_PIN, 0);
    chirping = false;
  }
}

// Shows the state on the board's own LED. Armed is dark on purpose, so nothing tells an intruder the beam is live.
void showStatus(State shown) {
  switch (shown) {
    case STATE_DISARMED:
      rgbLedWrite(RGB_BUILTIN, 0, STATUS_BRIGHTNESS, 0);
      break;
    case STATE_ARMING:
      rgbLedWrite(RGB_BUILTIN, STATUS_BRIGHTNESS, STATUS_BRIGHTNESS, 0);
      break;
    case STATE_ARMED:
      rgbLedWrite(RGB_BUILTIN, 0, 0, 0);
      break;
    case STATE_ALARM:
      rgbLedWrite(RGB_BUILTIN, STATUS_BRIGHTNESS, 0, 0);
      break;
  }
}

// Measures the beam by averaging a few reads and works out the trigger level. Returns false when the beam is too
// weak to use, which usually means the LED is not pointing at the sensor.
bool calibrateBeam() {
  long total = 0;
  for (int i = 0; i < CALIBRATION_READS; i++) {
    total += analogRead(LIGHT_PIN);
    delay(2);
  }

  beamLevel = total / CALIBRATION_READS;
  triggerLevel = beamLevel * TRIGGER_PERCENT / 100;

  if (beamLevel < MIN_BEAM_COUNTS) {
    Serial.printf("cannot arm: the beam reads only %d, aim the LED at the photoresistor\n", beamLevel);
    return false;
  }

  return true;
}

// Moves to a new state and does what that state needs once, on the way in.
void enterState(State next) {
  state = next;
  stateStartMs = millis();
  showStatus(next);

  switch (next) {
    case STATE_DISARMED:
      digitalWrite(BEAM_LED_PIN, LOW);
      ledcWrite(BUZZER_PIN, 0);
      chirping = false;
      Serial.println("state: DISARMED");
      break;

    case STATE_ARMING:
      // The beam goes on now, so it is steady by the time it is measured.
      digitalWrite(BEAM_LED_PIN, HIGH);
      lastChirpSecond = -1;
      Serial.println("state: ARMING, leave now, 10 s to go");
      break;

    case STATE_ARMED:
      lowReads = 0;
      Serial.printf("state: ARMED, beam level %d, alarm below %d\n", beamLevel, triggerLevel);
      break;

    case STATE_ALARM:
      sirenPitchHz = 0;
      chirping = false;
      Serial.printf("state: ALARM, beam broken, level %d\n", lastLevel);
      break;
  }
}

// setup() runs once, right after the board powers on or resets.
void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(BEAM_LED_PIN, OUTPUT);

  analogReadResolution(12);
  analogSetPinAttenuation(LIGHT_PIN, ADC_11db);

  ledcAttach(BUZZER_PIN, BUZZER_HZ, 8);
  ledcWrite(BUZZER_PIN, 0);

  Serial.println();
  Serial.println("beam alarm ready: press the button to arm it, hold it for 1 s to disarm");
  enterState(STATE_DISARMED);
}

void loop() {
  Press press = readButton();
  unsigned long now = millis();
  unsigned long elapsed = now - stateStartMs;

  switch (state) {
    case STATE_DISARMED:
      if (press == SHORT_PRESS) {
        enterState(STATE_ARMING);
      }
      break;

    case STATE_ARMING: {
      // One slow chirp each second, counted by how many whole seconds have passed.
      int second = elapsed / 1000;
      if (second != lastChirpSecond) {
        lastChirpSecond = second;
        startChirp(60);
      }

      if (press == SHORT_PRESS) {
        // A press during the countdown cancels it.
        enterState(STATE_DISARMED);
      } else if (elapsed >= ARMING_MS) {
        // The countdown is over, so measure the beam. If it is too weak, go back to disarmed.
        if (calibrateBeam()) {
          enterState(STATE_ARMED);
        } else {
          enterState(STATE_DISARMED);
          startChirp(400);
        }
      }
      break;
    }

    case STATE_ARMED:
      if (press == LONG_PRESS) {
        enterState(STATE_DISARMED);
        break;
      }

      // Read the beam on a fixed beat. A low read adds one to the count and a good read sets it back to zero.
      if (now - lastSampleMs >= SAMPLE_MS) {
        lastSampleMs = now;
        lastLevel = analogRead(LIGHT_PIN);

        if (lastLevel < triggerLevel) {
          lowReads++;
        } else {
          lowReads = 0;
        }

        if (lowReads >= LOW_READS_TO_TRIGGER) {
          enterState(STATE_ALARM);
        }
      }
      break;

    case STATE_ALARM:
      // Only a long press silences the siren. The beam coming back does nothing.
      if (press == LONG_PRESS) {
        enterState(STATE_DISARMED);
      }
      break;
  }

  updateSound();
}
