/**
 **************************************************
 *
 * @file        05f_Heartbeat.ino
 * @brief       Makes the LED on IO38 beat like a heart: two quick pulses, then a rest. The potentiometer on IO4 sets
 *              the rate from 40 to 140 beats per minute, and the beat is drawn from a table of brightness values.
 *              For details, connection diagram and more, check out the example documentation at: <link placeholder>
 * @author      Borna Zelenika @ soldered.com
 ***************************************************
 */

// The LED on IO38 needs a 330 Ohm resistor in series or it will be damaged. The pot's wiper is on IO4, as in T9.
const int LED_PIN = 38;
const int POT_PIN = 4;

// 5 kHz is far above what the eye can follow, and 8 bits gives 256 brightness steps (0 to 255).
const int PWM_FREQ = 5000;
const int PWM_RESOLUTION = 8;

// The slowest and fastest heartbeat the knob can ask for, in beats per minute.
const int MIN_BPM = 40;
const int MAX_BPM = 140;

// The BPM is printed only when it has moved at least this far from the last printed one.
const int PRINT_STEP_BPM = 5;

// One whole beat drawn as 32 brightness values, from the start of the beat to the start of the next.
// Entries 0 to 7 are the first pulse (lub), 8 to 16 the second, smaller one (dub), and the rest is silence.
// The shape is the same at every speed. Only the time each entry lasts changes.
const int BEAT_STEPS = 32;
const uint8_t BEAT_SHAPE[BEAT_STEPS] = {
  60, 160, 255, 200, 120, 50, 15, 5,
  0, 0, 40, 130, 190, 140, 70, 20,
  4, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0
};

// How long the beat in progress lasts, and when it started. Both are in milliseconds.
unsigned long beatMs = 1000;
unsigned long beatStartMs = 0;

// The BPM last printed.
int printedBpm = 0;

// Reads the knob and turns it into beats per minute, from MIN_BPM at the left end to MAX_BPM at the right end.
int readBpm() {
  int raw = analogRead(POT_PIN);
  return MIN_BPM + (raw * (MAX_BPM - MIN_BPM)) / 4095;
}

// Starts a new beat: reads the knob, works out how long one beat lasts at that rate, and prints the BPM if it
// has changed enough.
void startBeat(unsigned long now) {
  int bpm = readBpm();

  // 60000 ms in a minute, divided by the beats in that minute, is the length of one beat.
  beatMs = 60000UL / bpm;
  beatStartMs = now;

  if (abs(bpm - printedBpm) >= PRINT_STEP_BPM) {
    printedBpm = bpm;
    Serial.print("BPM ");
    Serial.println(bpm);
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  // The same 12-bit, 3.3 V range as the other analog examples.
  analogReadResolution(12);
  analogSetPinAttenuation(POT_PIN, ADC_11db);

  ledcAttach(LED_PIN, PWM_FREQ, PWM_RESOLUTION);

  Serial.println("HEARTBEAT: the knob on IO4 sets 40 to 140 beats per minute.");

  // The first beat starts now. printedBpm is far from any real value, so the first BPM is always printed.
  printedBpm = -100;
  startBeat(millis());
}

void loop() {
  unsigned long now = millis();
  unsigned long elapsedMs = now - beatStartMs;

  // When the beat is over, read the knob again and start the next one. The rate can change only here, once a beat.
  if (elapsedMs >= beatMs) {
    startBeat(now);
    elapsedMs = 0;
  }

  // How far through the beat we are, turned into a position in the table. The result is 0 to 31.
  int step = (elapsedMs * BEAT_STEPS) / beatMs;

  // Show that entry of the table. There is no long delay here: the loop comes round every few milliseconds, so
  // the clock decides which entry is showing, not a pause.
  ledcWrite(LED_PIN, BEAT_SHAPE[step]);

  delay(5);
}
