/**
 **************************************************
 *
 * @file        7.3_Two_Sensor_Sun_Tracker.ino
 * @brief       Two photoresistors either side of a paper strip turn a servo needle towards the brighter one, with a
 *              dead band and a rate limit to keep it from shuffling on its own noise.
 *              For details, connection diagram and more, check out the example documentation at: <link placeholder>
 * @author      Borna Zelenika @ soldered.com
 ***************************************************
 */

/*
The two photoresistors are on IO4 and IO5, the servo on IO47, and the 74HC595 LED bar on the four pins named below.

The sensors need a strip of paper standing upright in the channel between them, so light from one side falls on one
and is shadowed from the other. Without it they read almost the same value and the sketch has nothing to steer by.

Nothing is mounted on the servo. It is a needle: the horn turns to face whichever side is brighter, and you move a
light around the sensor head to lead it. A real tracker carries its sensors on the moving part, which needs a
bracket this kit does not have.

The servo takes its power from VCC, as in T19.
*/
const int LDR_LEFT_PIN = 4;
const int LDR_RIGHT_PIN = 5;

const int SERVO_PIN = 47;

const int DATA_PIN = 11;
const int CLOCK_PIN = 12;
const int LATCH_PIN = 10;
const int OE_PIN = 13;

// Your servo's real limits, measured in T19.
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

// The servo signal is 50 pulses a second, each one 20000 microseconds long, and the board describes a pulse's width
// as a number of steps out of 16384, which is what 14 bits give you. The S3 stops at 14 bits.
const int SERVO_FREQ = 50;
const int SERVO_RESOLUTION = 14;
const long SERVO_STEPS = 16384;
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

// Calibration, filled in at startup. Each sensor gets its own darkest and brightest reading. They start at the
// opposite ends so that the first reading replaces both.
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

  // Start with the needle in the centre. The pulse width is turned into steps out of 16384 for the board.
  long duty = (long)currentPulseUs * SERVO_STEPS / PERIOD_US;
  ledcWrite(SERVO_PIN, duty);

  // Calibration: ten seconds of moving a light around the sensor head. Each sensor records its own darkest and
  // brightest reading, and everything afterwards is a percentage of that sensor's own range.
  Serial.println();
  Serial.println("CALIBRATING for 10 seconds. Move a light around both sensors, and shade them.");

  unsigned long start = millis();
  while (millis() - start < 10000) {
    int leftReading = analogRead(LDR_LEFT_PIN);
    int rightReading = analogRead(LDR_RIGHT_PIN);

    if (leftReading < leftMin) {
      leftMin = leftReading;
    }
    if (leftReading > leftMax) {
      leftMax = leftReading;
    }
    if (rightReading < rightMin) {
      rightMin = rightReading;
    }
    if (rightReading > rightMax) {
      rightMax = rightReading;
    }

    delay(5);
  }

  // If a sensor barely changed, give it a range of at least 10 so the percentages below never divide by zero.
  if (leftMax - leftMin < 10) {
    leftMax = leftMin + 10;
  }
  if (rightMax - rightMin < 10) {
    rightMax = rightMin + 10;
  }

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

void loop() {
  // One update every 30 ms. The rest of the time the loop just comes round again.
  if (millis() - lastUpdateMs >= UPDATE_INTERVAL_MS) {
    lastUpdateMs = millis();

    // Calibrate each sensor to its own range before subtracting, not after. Two photoresistors from the same bag are
    // not matched, and subtracting raw readings turns that mismatch into a permanent error the needle cannot tell
    // from a real one: it sits off-centre and looks exactly like a bug in the code. Each reading becomes a percentage
    // of that sensor's own range, from 0 to 100.
    int left = constrain(map(analogRead(LDR_LEFT_PIN), leftMin, leftMax, 0, 100), 0, 100);
    int right = constrain(map(analogRead(LDR_RIGHT_PIN), rightMin, rightMax, 0, 100), 0, 100);

    // Gained here, once, so the needle, the LED bar and the printed error all mean the same thing.
    int error = constrain((left - right) * ERROR_GAIN, -100, 100);

    // The LED bar, lit from the middle outwards, so it shows both the size of the error and which way it points. It
    // moves while the needle is still gliding, which is the quickest way to tell a too-wide dead band from a sensor
    // that is dead. Every 12 points of error lights one more LED, up to four on each side. Each pattern is the eight
    // LEDs written as eight 1s and 0s, left to right.
    int magnitude = constrain(abs(error) / 12, 0, 4);
    uint8_t pattern = 0b00011000;

    if (error > 0 && magnitude == 1) {
      pattern = 0b00001000;
    } else if (error > 0 && magnitude == 2) {
      pattern = 0b00001100;
    } else if (error > 0 && magnitude == 3) {
      pattern = 0b00001110;
    } else if (error > 0 && magnitude == 4) {
      pattern = 0b00001111;
    } else if (error < 0 && magnitude == 1) {
      pattern = 0b00010000;
    } else if (error < 0 && magnitude == 2) {
      pattern = 0b00110000;
    } else if (error < 0 && magnitude == 3) {
      pattern = 0b01110000;
    } else if (error < 0 && magnitude == 4) {
      pattern = 0b11110000;
    }

    shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, pattern);
    digitalWrite(LATCH_PIN, HIGH);
    digitalWrite(LATCH_PIN, LOW);

    // Hard over at full error, on the trimmed centre when the sensors agree. The two halves of the sweep are mapped
    // separately, so the centre trim moves only the centre.
    int targetUs;
    if (error >= 0) {
      targetUs = map(error, 0, 100, PULSE_CENTRE_US, PULSE_MAX_US);
    } else {
      targetUs = map(error, -100, 0, PULSE_MIN_US, PULSE_CENTRE_US);
    }

    // Where the needle points now, on the same -100 to 100 scale, so the two can be compared directly. This is the
    // same mapping run backwards.
    int shownError;
    if (currentPulseUs >= PULSE_CENTRE_US) {
      shownError = map(currentPulseUs, PULSE_CENTRE_US, PULSE_MAX_US, 0, 100);
    } else {
      shownError = map(currentPulseUs, PULSE_MIN_US, PULSE_CENTRE_US, -100, 0);
    }

    if (abs(error - shownError) > DEAD_BAND) {
      // Move towards the target rather than jumping, so a sudden change in the light becomes a glide.
      int gap = targetUs - currentPulseUs;
      currentPulseUs += constrain(gap, -STEP_US, STEP_US);
    }

    currentPulseUs = constrain(currentPulseUs, PULSE_MIN_US, PULSE_MAX_US);

    long duty = (long)currentPulseUs * SERVO_STEPS / PERIOD_US;
    ledcWrite(SERVO_PIN, duty);

    // Printed whether or not the needle moved, so a still needle is visibly a decision rather than a crash.
    updateCount++;
    if (updateCount >= PRINT_EVERY) {
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
}

// The LED bar is bright, close to the sensors, and the photoresistors can see it. If the needle drifts towards the
// breadboard whenever the bar lights up, that is the LEDs, not the control loop.
