/**
 **************************************************
 *
 * @file        10d_Sensor_And_Button.ino
 * @brief       The TMP117 watches a temperature band while the board sleeps. The board wakes every five seconds to
 *              read the sensor's alert flag, or at once when the button is pressed to print the temperature now.
 *              For details, connection diagram and more, check out the example documentation at: <link placeholder>
 * @author      Borna Zelenika @ soldered.com
 ***************************************************
 */

#include <Wire.h>

// From the Library Manager. Search for "Soldered TMP117", by Soldered.
#include "Soldered-TMP117.h"

// The button is on IO21 (with its own pull-up to 3V3) and the LED on IO38. The TMP117 is on the Qwiic cable.
const int BUTTON_PIN = 21;
const int LED_PIN = 38;
const uint8_t TMP117_ADDRESS = 0x49;

// How often the board wakes to read the flag if nobody presses the button.
const uint64_t SLEEP_SECONDS = 5;

// The band is this many degrees Celsius either side of the temperature when it was last set.
const float BAND_C = 1.0;

// Whether the sensor has been configured yet, and how many alerts it has raised so far. Both stay in RTC memory.
RTC_DATA_ATTR bool configured = false;
RTC_DATA_ATTR int alertCount = 0;

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

// Puts a band around the given temperature. The limits live in the sensor, so they keep working while the board
// sleeps. Reading the config register afterwards clears any flag the old band left behind.
void setBandAround(float celsius) {
  tmp.setAlertTemperature(celsius - BAND_C, celsius + BAND_C);
  tmp.readConfig();
}

void sleepAgain() {
  // If the button is still held down, wait for it to be let go, or the board would wake again at once.
  while (digitalRead(BUTTON_PIN) == LOW) {
    delay(10);
  }

  Serial.flush();

  // Both sources are armed, and whichever comes first wakes the board.
  esp_sleep_enable_timer_wakeup(SLEEP_SECONDS * 1000000ULL);
  esp_sleep_enable_ext0_wakeup((gpio_num_t)BUTTON_PIN, 0);
  esp_deep_sleep_start();
}

void setup() {
  Serial.begin(115200);

  bool firstBoot = !configured;
  if (firstBoot) {
    delay(1000);
  }

  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT);
  Wire.begin();

  // The sensor keeps its settings through sleep, so it is configured only once.
  if (firstBoot) {
    tmp.init(nullptr);
    tmp.setConvMode(TMP117_CMODE::CONTINUOUS);
    tmp.setConvTime(TMP117_CONVT::C1S);
    tmp.setAlertMode(TMP117_PMODE::ALERT);
  }

  // A missing sensor still gives a number, so ask it who it is.
  if (tmp.getDeviceID() != 0x117) {
    Serial.println();
    Serial.println("no TMP117 on the bus. Check the Qwiic cable.");
    blink(5);
    sleepAgain();
  }

  if (firstBoot) {
    setBandAround(tmp.getTemperature());
    configured = true;
    Serial.println();
    Serial.println("sensor set up. Put a finger on it, or press the button.");
    sleepAgain();
  }

  // Reading the config register tells us about the flags and clears them in the same step.
  uint16_t config = tmp.readConfig();
  bool alert = ((config >> 15) & 1) || ((config >> 14) & 1);
  float celsius = tmp.getTemperature();

  if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT0) {
    // The button was pressed: report the temperature now, whatever the flag says.
    Serial.println();
    Serial.print("button: ");
    Serial.print(celsius, 2);
    Serial.print(" C, alerts so far: ");
    Serial.println(alertCount);
    blink(2);
  } else if (alert) {
    // The timer woke the board and the sensor had raised its flag.
    alertCount++;
    Serial.println();
    Serial.print("alert ");
    Serial.print(alertCount);
    Serial.print(": the temperature left the band, now ");
    Serial.print(celsius, 2);
    Serial.println(" C");
    blink(1);
  } else {
    // Nothing to report. One dot so you can watch the checks go by.
    Serial.print(".");
  }

  // After an alert, put a new band around where the temperature is now.
  if (alert) {
    setBandAround(celsius);
  }

  sleepAgain();
}

void loop() {
  // Never reached. Deep sleep restarts the sketch from setup() every time.
}
