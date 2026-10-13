/**
 **************************************************
 *
 * @file        7.4_Bench_Controller.ino
 * @brief       Five jobs sharing one loop, none of them waiting for the others. A pot aims the servo, the TMP117
 *              colours the onboard RGB LED, and the buzzer chirps, until the relay button locks the controls and
 *              the LED turns green. Two idle minutes put the board to sleep.
 *              For details, connection diagram and more, check out the example documentation at: <link placeholder>
 * @author      Soldered Electronics
 ***************************************************
 */

#include <Wire.h>

// From the Library Manager, not this repository. Search for "Soldered TMP117" and "Soldered Library for Relays".
#include "Soldered-TMP117.h"
#include "Relays-SOLDERED.h"

/*
Everything before this page worked on its own. This one is about making four of them work at the same time, so the
parts list is short on purpose: one pot, two buttons, the servo, the buzzer, and the two Qwiic modules. The readout
is the WS2812B already on the board.

IO21 is the button that also has to wake the board, so it is the one that needs a pull-up of its own on the
breadboard. The internal pull-ups switch off during deep sleep, which example 6.1 covers.
*/
const int BUTTON_PINS[2] = {21, 39};
const int BUTTON_COUNT = 2;

#define BTN_RELAY  0
#define BTN_BUZZER 1

const int POT_PIN = 4;

// The servo takes its power from VCC, never from 3V3. The 3V3 regulator is there to run the chip.
const int SERVO_PIN = 47;
const int BUZZER_PIN = 48;

const uint8_t TMP117_ADDRESS = 0x49;
const uint8_t RELAY_ADDRESS = 0x30;

// From example 4.3. Yours will be a different number.
const int BUZZER_HZ = 2700;

// From example 4.2. Yours will be different numbers.
const int PULSE_MIN_US = 550;
const int PULSE_MAX_US = 2500;
const int SERVO_FREQ = 50;

// 14, not 16. The S3 LEDC peripheral caps there and ledcAttach() simply fails above it, leaving the pin silent and
// saying nothing, which is why both calls below are checked.
const int SERVO_RESOLUTION = 14;
const long PERIOD_US = 20000;

// Your measured bounce duration from example 2.1, rounded up.
const unsigned long DEBOUNCE_MS = 25;

const unsigned long IDLE_SLEEP_MS = 120000;

// 0x49 because JP2 on the breakout ties ADD0 high. A missing module reads 0.0000 rather than failing, so
// getDeviceID() == 0x117 is the only honest test.
Soldered_TMP117 tmp(TMP117_ADDRESS);

// No address on this line: this library takes it in begin() instead.
CH_Relay Relay;

// Debounce state, one set per button, from example 2.1.
bool buttonState[BUTTON_COUNT];
bool lastReading[BUTTON_COUNT];
unsigned long lastChangeMs[BUTTON_COUNT];
bool buttonPressed[BUTTON_COUNT];

float temperatureC = 0;
bool sensorOk = false;
bool relayOn = false;
bool buzzerOn = false;
bool chirping = false;

unsigned long lastInputMs = 0;
unsigned long lastSensorMs = 0;
unsigned long lastDisplayMs = 0;
unsigned long lastServoMs = 0;
unsigned long lastChirpMs = 0;

// Three different rates, none allowed to hold up the others. The buttons are faster than all of them because they
// are read on every single pass.
const unsigned long SENSOR_INTERVAL_MS = 400;
const unsigned long DISPLAY_INTERVAL_MS = 80;
const unsigned long SERVO_INTERVAL_MS = 30;

// At 20 us a step the needle crosses its whole range in about three seconds. This is the number to change first if
// the sweep feels wrong on your bench.
const int SERVO_STEP_US = 20;

// The colour runs blue to red across this span. Pick a range your room actually sits inside.
const float COLOUR_MIN_C = 20.0;
const float COLOUR_MAX_C = 30.0;

// The WS2812B is far brighter than it needs to be. This is an eighth of full scale.
const uint8_t LED_BRIGHTNESS = 32;

const unsigned long CHIRP_MS = 120;
const unsigned long CHIRP_PERIOD_MS = 2000;

int currentPulseUs = (PULSE_MIN_US + PULSE_MAX_US) / 2;

void setup() {
  bool firstBoot = (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_UNDEFINED);

  Serial.begin(115200);
  // Only the first boot waits for a monitor: serial survives deep sleep here because the CH340 stays enumerated.
  if (firstBoot) {
    delay(1000);
    // Deep sleep reprints the nine-line boot ROM banner on every wake. This silences it from the next one onwards.
    esp_deep_sleep_disable_rom_logging();
  }

  Wire.begin();
  // Nothing else is configured: every setConvMode, setConvTime, setAveraging and setAlertMode call writes the
  // sensor's EEPROM, and doing that on each boot of a board that sleeps all day is a lot of writes for no gain.
  tmp.init(nullptr);

  Relay.begin(RELAY_ADDRESS);
  Relay.relayControl(CHANNEL1, LOW);

  for (int i = 0; i < BUTTON_COUNT; i++) {
    pinMode(BUTTON_PINS[i], INPUT_PULLUP);
    buttonState[i] = HIGH;
    lastReading[i] = HIGH;
    lastChangeMs[i] = 0;
    buttonPressed[i] = false;
  }

  analogReadResolution(12);
  analogSetPinAttenuation(POT_PIN, ADC_11db);

  // Checked, because a resolution the chip refuses leaves the pin silent and says nothing about it.
  if (!ledcAttach(SERVO_PIN, SERVO_FREQ, SERVO_RESOLUTION)) {
    Serial.println("servo channel failed to attach. The needle will not move.");
  }
  if (!ledcAttach(BUZZER_PIN, BUZZER_HZ, 8)) {
    Serial.println("buzzer channel failed to attach.");
  }
  ledcWrite(BUZZER_PIN, 0);

  lastInputMs = millis();

  Serial.println();
  Serial.println("bench controller. pot aims the needle, IO39 toggles the buzzer, IO21 closes the relay and locks both.");
  if (!firstBoot) {
    Serial.println("woken by a button press.");
  }
}

/*
Five jobs, none of which waits for any of the others. Everything on the pages before this one worked in isolation,
and the reason all of it composes here is that no part of it blocks. Architecture is the one thing that cannot be
taught on a page about a single component, and it comes down to this.
*/
void loop() {
  serviceButtons();
  serviceSensor();
  serviceControls();
  serviceDisplay();
  serviceOutputs();
  serviceSleep();
}

// Debouncing from 2.1, in the form you actually want it: a function that reports a press once and then forgets it,
// so the code reacting to a button never has to think about timing.
void serviceButtons() {
  unsigned long now = millis();

  for (int i = 0; i < BUTTON_COUNT; i++) {
    bool reading = digitalRead(BUTTON_PINS[i]);

    if (reading != lastReading[i]) {
      lastReading[i] = reading;
      lastChangeMs[i] = now;
    }

    if (now - lastChangeMs[i] > DEBOUNCE_MS && reading != buttonState[i]) {
      buttonState[i] = reading;

      // Pull-ups mean LOW is pressed.
      if (reading == LOW) {
        buttonPressed[i] = true;
        lastInputMs = now;
      }
    }
  }
}

bool takePress(int index) {
  if (!buttonPressed[index]) {
    return false;
  }
  buttonPressed[index] = false;
  return true;
}

void serviceSensor() {
  if (millis() - lastSensorMs < SENSOR_INTERVAL_MS) {
    return;
  }
  lastSensorMs = millis();

  // Asked every time, so pulling the Qwiic cable out mid-run is caught on the next read rather than never.
  sensorOk = (tmp.getDeviceID() == 0x117);
  if (sensorOk) {
    temperatureC = tmp.getTemperature();
  }
}

/*
Closing the relay locks the controls. The ignored press is the point of this function: it is still scanned, still
debounced, still taken off the queue, and then dropped with a line saying so. A program that blocks elsewhere never
sees the press at all, and from the bench the two look identical. One is a decision and the other is a bug, and
what tells them apart is that a program which chose to ignore you can say so.
*/
void serviceControls() {
  if (takePress(BTN_RELAY)) {
    relayOn = !relayOn;
    Relay.relayControl(CHANNEL1, relayOn ? HIGH : LOW);
    Serial.print("relay ");
    Serial.print(relayOn ? "on" : "off");
    Serial.println(relayOn ? ", controls locked" : ", controls unlocked");
  }

  if (takePress(BTN_BUZZER)) {
    if (relayOn) {
      Serial.println("locked. Press the button on IO21 to unlock.");
    } else {
      buzzerOn = !buzzerOn;
      Serial.print("buzzer ");
      Serial.println(buzzerOn ? "on" : "off");
    }
  }
}

void serviceDisplay() {
  if (millis() - lastDisplayMs < DISPLAY_INTERVAL_MS) {
    return;
  }
  lastDisplayMs = millis();

  if (!sensorOk) {
    // Flashing white, so a missing sensor can never be mistaken for a reading at one end of the colour range.
    bool on = (millis() / 300) % 2;
    rgbLedWrite(RGB_BUILTIN, on ? LED_BRIGHTNESS : 0, on ? LED_BRIGHTNESS : 0, on ? LED_BRIGHTNESS : 0);
  } else if (relayOn) {
    // Green appears nowhere in the gradient below, so it means one thing: the controls are locked. A state that
    // changes what the buttons do has to be visible without the serial monitor.
    rgbLedWrite(RGB_BUILTIN, 0, LED_BRIGHTNESS, 0);
  } else {
    float span = (temperatureC - COLOUR_MIN_C) / (COLOUR_MAX_C - COLOUR_MIN_C);
    span = constrain(span, 0.0, 1.0);

    // rgbLedWrite() takes plain red, green, blue and sorts out the GRB byte order the WS2812B actually wants.
    rgbLedWrite(RGB_BUILTIN, (uint8_t)(span * LED_BRIGHTNESS), 0, (uint8_t)((1.0 - span) * LED_BRIGHTNESS));
  }
}

void serviceOutputs() {
  serviceBuzzer();

  if (millis() - lastServoMs < SERVO_INTERVAL_MS) {
    return;
  }
  lastServoMs = millis();

  // Locked, the pot is not read and the needle holds still, but this job keeps running on its 30 ms timer either
  // way. Stopping a job and stopping what a job does to the outside world are different things.
  if (relayOn) {
    return;
  }

  int target = map(analogRead(POT_PIN), 0, 4095, PULSE_MIN_US, PULSE_MAX_US);

  int difference = target - currentPulseUs;
  difference = constrain(difference, -SERVO_STEP_US, SERVO_STEP_US);
  currentPulseUs += difference;

  uint32_t duty = (uint32_t)((float)currentPulseUs * (float)(1UL << SERVO_RESOLUTION) / (float)PERIOD_US);
  ledcWrite(SERVO_PIN, duty);
}

// A chirp every two seconds, written without a single delay. Two timestamps and a flag do what delay(120) would
// have done, and the needle keeps moving and the buttons keep answering while it sounds.
void serviceBuzzer() {
  if (!buzzerOn) {
    if (chirping) {
      chirping = false;
      ledcWrite(BUZZER_PIN, 0);
    }
    return;
  }

  unsigned long now = millis();

  if (!chirping && now - lastChirpMs >= CHIRP_PERIOD_MS) {
    chirping = true;
    lastChirpMs = now;
    // Half of full scale. Example 4.3 measured loudness peaking at 50 % duty and falling again above it.
    ledcWrite(BUZZER_PIN, 128);
  }

  if (chirping && now - lastChirpMs >= CHIRP_MS) {
    chirping = false;
    ledcWrite(BUZZER_PIN, 0);
  }
}

// Two minutes without a press and the whole thing sleeps, waking on IO21. Everything it was doing is lost, because
// deep sleep is not pausing, and setup() rebuilds it on the way back. The relay is dropped first: a latched coil
// would draw more asleep than the chip does.
void serviceSleep() {
  if (millis() - lastInputMs < IDLE_SLEEP_MS) {
    return;
  }

  Serial.println("idle for two minutes. Sleeping. Press the button on IO21 to wake.");
  Serial.flush();

  ledcWrite(BUZZER_PIN, 0);
  Relay.relayControl(CHANNEL1, LOW);
  rgbLedWrite(RGB_BUILTIN, 0, 0, 0);

  esp_sleep_enable_ext0_wakeup((gpio_num_t)BUTTON_PINS[BTN_RELAY], 0);
  esp_deep_sleep_start();
}
