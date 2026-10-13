/**
 **************************************************
 *
 * @file        7.3_Two_Sensor_Sun_Tracker.ino
 * @brief       Two photoresistors either side of a paper strip turn a servo needle towards the brighter one, with a
 *              dead band and a rate limit to keep it from shuffling on its own noise.
 *              For details, connection diagram and more, check out the example documentation at: <link placeholder>
 * @author      Soldered Electronics
 ***************************************************
 */

/*
The two photoresistors are on IO4 and IO5, the servo on IO47, and the 74HC595 LED bar on the four pins named below.

The sensors need a strip of paper standing upright in the channel between them, so light from one side falls on one
and is shadowed from the other. Without it they read almost the same value and the sketch has nothing to steer by.

Nothing is mounted on the servo. It is a needle: the horn turns to face whichever side is brighter, and you move a
light around the sensor head to lead it. A real tracker carries its sensors on the moving part, which needs a
bracket this kit does not have.

The servo takes its power from VCC, as in example 4.2.
*/
const int LDR_LEFT_PIN = 4;
const int LDR_RIGHT_PIN = 5;

const int SERVO_PIN = 47;

const int DATA_PIN = 11;
const int CLOCK_PIN = 12;
const int LATCH_PIN = 10;
const int OE_PIN = 13;

// Your servo's real limits, measured in example 4.2.
const int PULSE_MIN_US = 550;
const int PULSE_MAX_US = 2500;

// Where the needle sits when the sensors agree. Halfway is only halfway by arithmetic: the horn refits no finer
// than one spline, 18 degrees on an SG90. Each half of the sweep is mapped from this separately, so you can trim
// the centre without disturbing either end stop.
const int PULSE_CENTRE_US = 1525;

/*
How hard the needle reacts to a difference between the sensors. In room light the two readings land within a few
points of each other, and five points out of a hundred barely moves the horn, so the difference is multiplied to
make the needle commit to a side rather than hover.

Raise it until the needle takes a side in ordinary light. Too far and it slams between the ends, because gain
multiplies the sensors' noise along with the signal, which is what DEAD_BAND and the rate limit are for.
*/
const int ERROR_GAIN = 5;

const int SERVO_FREQ = 50;
const int SERVO_RESOLUTION = 14;
const long PERIOD_US = 20000;

// How far the needle may travel per update and how often one happens: together, its top speed, about one and a half
// seconds across the full sweep. Raise STEP_US to 2000 and it jumps instead, which is worth seeing once.
const int STEP_US = 40;
const int UPDATE_INTERVAL_MS = 30;

/*
The dead band, on the same 0 to 100 scale as the error, measured against where the needle already points rather than
against zero. The question is not whether the light is off centre but whether it has moved far enough since last
time to be worth chasing.

Set it to 0 and the needle never settles. Raise it if yours twitches with the light held still, lower it if it
ignores a light you have genuinely moved.
*/
const int DEAD_BAND = 8;

// The serial line would be unreadable at one line every 30 ms, so only every fifth update prints.
const int PRINT_EVERY = 5;

// Calibration, filled in at startup. Each sensor gets its own.
int leftMin = 4095, leftMax = 0;
int rightMin = 4095, rightMax = 0;

int currentPulseUs = PULSE_CENTRE_US;
unsigned long lastUpdateMs = 0;
int updateCount = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);

  analogReadResolution(12);
  analogSetPinAttenuation(LDR_LEFT_PIN, ADC_11db);
  analogSetPinAttenuation(LDR_RIGHT_PIN, ADC_11db);

  pinMode(DATA_PIN, OUTPUT);
  pinMode(CLOCK_PIN, OUTPUT);
  pinMode(LATCH_PIN, OUTPUT);
  pinMode(OE_PIN, OUTPUT);
  digitalWrite(OE_PIN, LOW);

  // The S3 stops at 14 bits. Ask for 16 and this returns false, the pin stays silent, and nothing else complains,
  // so the failure looks like a dead servo.
  if (!ledcAttach(SERVO_PIN, SERVO_FREQ, SERVO_RESOLUTION)) {
    Serial.println("ledcAttach failed. The servo pin will not pulse.");
  }
  writePulse(currentPulseUs);

  calibrate();
}

void loop() {
  if (millis() - lastUpdateMs < UPDATE_INTERVAL_MS) {
    return;
  }
  lastUpdateMs = millis();

  // Calibrate each sensor to its own range before subtracting, not after. Two photoresistors from the same bag are
  // not matched, and subtracting raw readings turns that mismatch into a permanent error the needle cannot tell
  // from a real one: it sits off-centre and looks exactly like a bug in the code.
  int left = scaled(analogRead(LDR_LEFT_PIN), leftMin, leftMax);
  int right = scaled(analogRead(LDR_RIGHT_PIN), rightMin, rightMax);

  // Gained here, once, so the needle, the LED bar and the printed error all mean the same thing.
  int error = constrain((left - right) * ERROR_GAIN, -100, 100);

  showError(error);

  // Hard over at full error, on the trimmed centre when the sensors agree.
  int targetUs = pulseFor(error);

  // Where the needle points now, in the same units, so the two can be compared directly.
  int shownError = errorFor(currentPulseUs);

  if (abs(error - shownError) > DEAD_BAND) {
    // Move towards the target rather than jumping, so a sudden change in the light becomes a glide.
    int gap = targetUs - currentPulseUs;
    currentPulseUs += constrain(gap, -STEP_US, STEP_US);
  }

  currentPulseUs = constrain(currentPulseUs, PULSE_MIN_US, PULSE_MAX_US);
  writePulse(currentPulseUs);

  // Printed whether or not the needle moved, so a still needle is visibly a decision rather than a crash.
  if (++updateCount >= PRINT_EVERY) {
    updateCount = 0;
    Serial.print(left);
    Serial.print(" ");
    Serial.print(right);
    Serial.print(" ");
    Serial.print(error);
    Serial.print(" ");
    Serial.println(currentPulseUs);
  }
}

// Ten seconds of moving a light around the sensor head. Each sensor records its own darkest and brightest reading,
// and everything afterwards is a percentage of that sensor's own range.
void calibrate() {
  Serial.println();
  Serial.println("CALIBRATING for 10 seconds. Move a light around both sensors, and shade them.");

  unsigned long start = millis();
  while (millis() - start < 10000) {
    int l = analogRead(LDR_LEFT_PIN);
    int r = analogRead(LDR_RIGHT_PIN);

    if (l < leftMin) leftMin = l;
    if (l > leftMax) leftMax = l;
    if (r < rightMin) rightMin = r;
    if (r > rightMax) rightMax = r;

    delay(5);
  }

  if (leftMax - leftMin < 10) leftMax = leftMin + 10;
  if (rightMax - rightMin < 10) rightMax = rightMin + 10;

  Serial.print("left  ");
  Serial.print(leftMin);
  Serial.print(" to ");
  Serial.println(leftMax);
  Serial.print("right ");
  Serial.print(rightMin);
  Serial.print(" to ");
  Serial.println(rightMax);
  Serial.println();
  Serial.println("left right error pulse_us");
}

int scaled(int raw, int minimum, int maximum) {
  return constrain(map(raw, minimum, maximum, 0, 100), 0, 100);
}

// An error of -100 to 100 turned into a pulse width, one half of the sweep at a time so the centre trim moves only
// the centre.
int pulseFor(int error) {
  if (error >= 0) {
    return map(error, 0, 100, PULSE_CENTRE_US, PULSE_MAX_US);
  }
  return map(error, -100, 0, PULSE_MIN_US, PULSE_CENTRE_US);
}

// The reverse, used by the dead band to ask where the needle is currently pointing.
int errorFor(int pulseUs) {
  if (pulseUs >= PULSE_CENTRE_US) {
    return map(pulseUs, PULSE_CENTRE_US, PULSE_MAX_US, 0, 100);
  }
  return map(pulseUs, PULSE_MIN_US, PULSE_CENTRE_US, -100, 0);
}

// Lit from the middle outwards, so the bar shows both the size of the error and which way it points. It moves while
// the needle is still gliding, which is the quickest way to tell a too-wide dead band from a sensor that is dead.
void showError(int error) {
  uint8_t pattern = 0;
  int magnitude = constrain(abs(error) / 12, 0, 4);

  for (int i = 0; i < magnitude; i++) {
    if (error > 0) {
      pattern |= (1 << (3 - i));
    } else {
      pattern |= (1 << (4 + i));
    }
  }

  if (magnitude == 0) {
    pattern = 0b00011000;
  }

  shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, pattern);
  digitalWrite(LATCH_PIN, HIGH);
  digitalWrite(LATCH_PIN, LOW);
}

void writePulse(int pulseUs) {
  uint32_t duty = (uint32_t)((float)pulseUs * (float)(1UL << SERVO_RESOLUTION) / (float)PERIOD_US);
  ledcWrite(SERVO_PIN, duty);
}

// The LED bar is bright, close to the sensors, and the photoresistors can see it. If the needle drifts towards the
// breadboard whenever the bar lights up, that is the LEDs, not the control loop.
