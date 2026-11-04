/**
 **************************************************
 *
 * @file        08j_Level_Meter.ino
 * @brief       An eight-LED level meter on a 74HC595. The number of lit LEDs follows the pot, and one more LED marks
 *              the highest level reached, falling one step every second. The register is written only when the
 *              pattern changes.
 *              For details, connection diagram and more, check out the example documentation at: <link placeholder>
 * @author      Borna Zelenika @ soldered.com
 ***************************************************
 */

/*
Find pin 1 first. It is at the end with the half-moon notch, left-hand pin with the notch at the top. Some packages
also carry a round dimple that is not next to the notch and is not pin 1 on this part.

SRCLR must be held at 3V3 or the register clears itself at random, and the 100 nF must sit as close to the chip's
supply pins as the breadboard allows.
*/
const int DATA_PIN = 11;
const int CLOCK_PIN = 12;
const int LATCH_PIN = 10;
const int OE_PIN = 13;
const int POT_PIN = 4;

// Smooths the pot reading. It is faster than T13's 0.05, because a meter has to keep up with your hand.
const float EMA_ALPHA = 0.2;
float potEma = -1.0;

// The peak stays for this long before it falls one step.
const unsigned long PEAK_FALL_MS = 1000;
unsigned long lastPeakMoveMs = 0;

// The current level and the peak, both from 0 (nothing lit) to 8 (every LED lit).
int level = 0;
int peak = 0;

// What was last shown and printed. -1 is a value nothing can have, so the first pass always writes and prints.
int lastPattern = -1;
int lastLevel = -1;
int lastPeak = -1;

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(LATCH_PIN, OUTPUT);
  digitalWrite(LATCH_PIN, LOW);

  pinMode(DATA_PIN, OUTPUT);
  pinMode(CLOCK_PIN, OUTPUT);

  // Output enable is active low, so holding it LOW is what makes the LEDs visible.
  pinMode(OE_PIN, OUTPUT);
  digitalWrite(OE_PIN, LOW);

  analogReadResolution(12);
  analogSetPinAttenuation(POT_PIN, ADC_11db);

  Serial.println("LEVEL: the bar follows the knob and the peak falls one step a second.");
}

void loop() {
  int raw = analogRead(POT_PIN);

  // Seed the average on the first pass, or the bar crawls up from zero every time the board resets.
  if (potEma < 0) {
    potEma = raw;
  }
  potEma += EMA_ALPHA * (raw - potEma);

  // Nine states, from 0 to 8 LEDs lit. The largest reading, 4095, gives 8.
  level = constrain((int)(potEma * 9 / 4096), 0, 8);

  updatePeak();

  // Write the register only when the pattern is not the one already showing.
  int pattern = buildPattern(level, peak);
  if (pattern != lastPattern) {
    writeManual(pattern);
    lastPattern = pattern;
  }

  // Print only when the level or the peak changed.
  if (level != lastLevel || peak != lastPeak) {
    lastLevel = level;
    lastPeak = peak;

    Serial.print("level ");
    Serial.print(level);
    Serial.print(" peak ");
    Serial.println(peak);
  }

  delay(20);
}

// Raises the peak at once when the level reaches it, and lets it fall one step for every PEAK_FALL_MS the level has
// stayed below it.
void updatePeak() {
  if (level >= peak) {
    peak = level;
    lastPeakMoveMs = millis();
  } else if (millis() - lastPeakMoveMs >= PEAK_FALL_MS) {
    peak--;
    lastPeakMoveMs = millis();
  }
}

// Builds the byte for the register: a run of lit LEDs for the level, plus one more LED for the peak.
int buildPattern(int barLevel, int peakLevel) {
  // One shift left, minus one, turns a count into a run of lit bits: 3 becomes 0b00000111.
  int bar = (1 << barLevel) - 1;

  // The peak is a single lit bit, at the position of the highest level reached. No peak means no bit.
  int peakBit = 0;
  if (peakLevel > 0) {
    peakBit = 1 << (peakLevel - 1);
  }

  // OR keeps every bit that is set in either, so the bar and the peak LED both stay lit.
  return bar | peakBit;
}

// Sends one byte to the register, one pin write at a time. Nothing appears on the outputs until the latch is pulsed.
void writeManual(uint8_t value) {
  for (int bit = 7; bit >= 0; bit--) {
    digitalWrite(DATA_PIN, (value >> bit) & 1);

    digitalWrite(CLOCK_PIN, HIGH);
    digitalWrite(CLOCK_PIN, LOW);
  }

  digitalWrite(LATCH_PIN, HIGH);
  digitalWrite(LATCH_PIN, LOW);
}
