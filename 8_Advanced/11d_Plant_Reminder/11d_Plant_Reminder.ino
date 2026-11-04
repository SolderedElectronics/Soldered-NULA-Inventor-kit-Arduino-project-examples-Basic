/**
 **************************************************
 *
 * @file        11d_Plant_Reminder.ino
 * @brief       Sleeps for 30 seconds at a time. On each wake it reads the light and the temperature. If it has been
 *              dark for three wakes in a row, or the temperature is outside 18 to 28 C, it blinks the LED and chirps
 *              the buzzer for two seconds. Otherwise it goes straight back to sleep with the LED dark.
 *              For details, connection diagram and more, check out the example documentation at: <link placeholder>
 * @author      Borna Zelenika @ soldered.com
 ***************************************************
 */

#include <Wire.h>

// From the Library Manager. Search for "Soldered TMP117", by Soldered.
#include "Soldered-TMP117.h"

// The photoresistor divider is on IO4, the LED on IO38 and the buzzer's transistor on IO48. The TMP117 is on the
// Qwiic cable.
const int LIGHT_PIN = 4;
const int LED_PIN = 38;
const int BUZZER_PIN = 48;
const uint8_t TMP117_ADDRESS = 0x49;

// How long the board sleeps between two checks, in seconds.
const uint64_t SLEEP_SECONDS = 30;

// A reading below this many counts means dark. This one is a starting value: take yours from the calibration in T11,
// with your finger over the photoresistor.
const int DARK_BELOW_COUNTS = 600;

// The plant is happy between these two temperatures, in degrees Celsius.
const float COLDEST_OK_C = 18.0;
const float WARMEST_OK_C = 28.0;

// The reminder fires after this many dark wakes in a row.
const int DARK_WAKES_LIMIT = 3;

// The reminder is five beeps of 200 ms with a 200 ms gap, which is two seconds. The buzzer pitch is the resonant
// frequency from T17, so use your own, and the duty of 25 % is the one T26 uses.
const int BEEP_COUNT = 5;
const int BEEP_MS = 200;
const int GAP_MS = 200;
const int BUZZER_HZ = 2700;
const int BUZZER_DUTY = 64;

// Both stay in RTC memory through deep sleep. After a power-on or a reset they start from zero.
RTC_DATA_ATTR int wakeCount = 0;
RTC_DATA_ATTR int darkWakes = 0;

Soldered_TMP117 tmp(TMP117_ADDRESS);

// Blinks the LED the given number of times.
void blink(int times) {
  for (int i = 0; i < times; i++) {
    digitalWrite(LED_PIN, HIGH);
    delay(120);
    digitalWrite(LED_PIN, LOW);
    delay(120);
  }
}

// Blinks the LED and chirps the buzzer together for two seconds, then leaves both off.
void remind() {
  ledcAttach(BUZZER_PIN, BUZZER_HZ, 8);

  for (int i = 0; i < BEEP_COUNT; i++) {
    digitalWrite(LED_PIN, HIGH);
    ledcWrite(BUZZER_PIN, BUZZER_DUTY);
    delay(BEEP_MS);

    digitalWrite(LED_PIN, LOW);
    ledcWrite(BUZZER_PIN, 0);
    delay(GAP_MS);
  }

  // Hand the pin back, so nothing keeps driving the transistor while the board sleeps.
  ledcDetach(BUZZER_PIN);
}

// Returns the reason for a reminder, or an empty text when the plant is fine. The temperature comes first because it
// needs no history, and the light needs the dark counter.
const char *reasonToRemind(float celsius) {
  if (celsius < COLDEST_OK_C) {
    return "too cold";
  }
  if (celsius > WARMEST_OK_C) {
    return "too warm";
  }
  if (darkWakes >= DARK_WAKES_LIMIT) {
    return "too dark";
  }
  return "";
}

// Stops the board for SLEEP_SECONDS. The sketch starts again from setup() when the timer runs out.
void sleepAgain() {
  Serial.flush();

  esp_sleep_enable_timer_wakeup(SLEEP_SECONDS * 1000000ULL);
  esp_deep_sleep_start();
}

void setup() {
  Serial.begin(115200);

  bool firstBoot = (wakeCount == 0);
  if (firstBoot) {
    delay(1000);
  }

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  Wire.begin();

  analogReadResolution(12);
  analogSetPinAttenuation(LIGHT_PIN, ADC_11db);

  // The sensor keeps its settings through sleep, so it is configured only once. The wait gives it time to finish
  // its first one-second conversion.
  if (firstBoot) {
    tmp.init(nullptr);
    tmp.setConvMode(TMP117_CMODE::CONTINUOUS);
    tmp.setConvTime(TMP117_CONVT::C1S);
    delay(1100);

    Serial.println();
    Serial.println("plant reminder: a check every 30 s, a reminder when it is too dark, too cold or too warm");
  }

  // A missing sensor still gives a number, so ask it who it is. A failed wake is not counted and the dark counter is
  // left alone.
  if (tmp.getDeviceID() != 0x117) {
    Serial.println("no TMP117 on the bus. Check the Qwiic cable.");
    blink(5);
    sleepAgain();
  }

  int light = analogRead(LIGHT_PIN);
  float celsius = tmp.getTemperature();
  wakeCount++;

  // Count the dark wakes in a row. One bright wake sets the count back to zero.
  if (light < DARK_BELOW_COUNTS) {
    darkWakes++;
  } else {
    darkWakes = 0;
  }

  const char *reason = reasonToRemind(celsius);

  Serial.printf("wake %d: light %d, %.2f C, dark %d in a row, ", wakeCount, light, celsius, darkWakes);
  if (reason[0] == '\0') {
    Serial.println("ok");
  } else {
    Serial.printf("reminder: %s\n", reason);
    remind();
  }

  sleepAgain();
}

void loop() {
  // Never reached. Deep sleep restarts the sketch from setup() every time.
}
