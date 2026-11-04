/**
 **************************************************
 *
 * @file        08i_Melody_Player.ino
 * @brief       Plays a tune stored as two tables, one of note frequencies and one of note lengths, on the buzzer. The
 *              servo swings between 60 and 120 degrees on every beat, as if it were conducting. Nothing blocks, so
 *              the notes and the servo both run from the clock.
 *              For details, connection diagram and more, check out the example documentation at: <link placeholder>
 * @author      Borna Zelenika @ soldered.com
 ***************************************************
 */

// IO48 drives the transistor under the buzzer through the 10 kOhm resistor, and IO47 carries the servo's signal.
const int BUZZER_PIN = 48;
const int SERVO_PIN = 47;

// The tune, as data. Each note has a frequency in hertz, a length in beats and a name to print. The three lists
// line up, so entry 0 of each describes the first note. Frequencies near 2.7 kHz are where this buzzer is loudest,
// so the tune sits in the sixth octave.
const int NOTE_COUNT = 15;
const int MELODY_HZ[NOTE_COUNT] = {1319, 1319, 1397, 1568, 1568, 1397, 1319, 1175, 1047, 1047, 1175, 1319, 1319, 1175, 1175};
const float MELODY_BEATS[NOTE_COUNT] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1.5, 0.5, 2};
const char *MELODY_NAMES[NOTE_COUNT] = {"E6", "E6", "F6", "G6", "G6", "F6", "E6", "D6", "C6", "C6", "D6", "E6", "E6", "D6", "D6"};

// The speed of the tune. At 120 beats a minute one beat lasts 500 ms.
const int TEMPO_BPM = 120;
const unsigned long BEAT_MS = 60000UL / TEMPO_BPM;

// The buzzer goes quiet this long before each note ends, so that two equal notes in a row sound like two.
const unsigned long GAP_MS = 40;

// The two ends of the conductor's swing, in degrees.
const int ANGLE_LOW = 60;
const int ANGLE_HIGH = 120;

// The servo's pulse widths for 0 and 180 degrees. These are the limits you measured in T19.
const int PULSE_MIN_US = 550;
const int PULSE_MAX_US = 2500;

// The servo wants a pulse every 20 ms. The S3's PWM stops at 14 bits.
const int SERVO_FREQ = 50;
const int SERVO_RESOLUTION = 14;
const long PERIOD_US = 20000;

// PWM channels share their timers in pairs, and the buzzer changes its timer's frequency for every note. Channels 0
// and 2 sit on different timers, so a new note never disturbs the servo's 20 ms period.
const int BUZZER_CHANNEL = 0;
const int SERVO_CHANNEL = 2;

// Where the tune is, and when things last happened.
int noteIndex = 0;
unsigned long noteStartMs = 0;
unsigned long noteMs = 0;
bool noteSounding = false;
unsigned long lastBeatMs = 0;
bool servoHigh = false;

void setup() {
  Serial.begin(115200);
  delay(1000);

  // The 1000 Hz is only a starting value, and every note then sets its own frequency.
  ledcAttachChannel(BUZZER_PIN, 1000, 8, BUZZER_CHANNEL);

  // Checking the return is the difference between a silent pin and knowing why it is silent.
  if (!ledcAttachChannel(SERVO_PIN, SERVO_FREQ, SERVO_RESOLUTION, SERVO_CHANNEL)) {
    Serial.println("ledcAttach failed, the servo will not move");
  }

  // Start the servo at the low end of its swing.
  writeAngle(ANGLE_LOW);

  Serial.println("MELODY: Ode to Joy at 120 beats a minute.");

  // Start the clock for the first note and the first beat. The beat is set back by one beat length so that the
  // servo swings at once.
  unsigned long songStartMs = millis();
  noteStartMs = songStartMs;
  lastBeatMs = songStartMs - BEAT_MS;
  startNote(0);
}

void loop() {
  unsigned long now = millis();

  // On every beat, swing the servo to the other end.
  if (now - lastBeatMs >= BEAT_MS) {
    lastBeatMs += BEAT_MS;
    servoHigh = !servoHigh;
    writeAngle(servoHigh ? ANGLE_HIGH : ANGLE_LOW);
  }

  // Just before the note ends, silence the buzzer to leave the small gap between notes.
  if (noteSounding && now - noteStartMs >= noteMs - GAP_MS) {
    ledcWriteTone(BUZZER_PIN, 0);
    noteSounding = false;
  }

  // When the note is over, move on to the next one, and go back to the first one after the last.
  if (now - noteStartMs >= noteMs) {
    // Add the length instead of using the time now, so small delays do not add up over the tune.
    noteStartMs += noteMs;
    noteIndex++;

    if (noteIndex >= NOTE_COUNT) {
      Serial.println("End of the tune. Starting again.");
      noteIndex = 0;
    }

    startNote(noteIndex);
  }
}

// Starts sounding one note from the tables, works out how long it lasts, and prints its name.
void startNote(int index) {
  noteMs = (unsigned long)(MELODY_BEATS[index] * BEAT_MS);

  ledcWriteTone(BUZZER_PIN, MELODY_HZ[index]);
  noteSounding = true;

  Serial.print(MELODY_NAMES[index]);
  Serial.print("  ");
  Serial.print(MELODY_BEATS[index], 1);
  Serial.println(" beats");
}

// Turns the servo to an angle in degrees. The angle becomes a pulse width between your two limits, and the pulse
// width becomes a duty number, as in the servo examples.
void writeAngle(int degrees) {
  int pulseUs = map(degrees, 0, 180, PULSE_MIN_US, PULSE_MAX_US);
  uint32_t duty = (uint32_t)((float)pulseUs * (float)(1UL << SERVO_RESOLUTION) / (float)PERIOD_US);
  ledcWrite(SERVO_PIN, duty);
}
