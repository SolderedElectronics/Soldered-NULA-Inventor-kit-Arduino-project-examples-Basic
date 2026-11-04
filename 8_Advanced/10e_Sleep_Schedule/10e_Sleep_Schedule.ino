/**
 **************************************************
 *
 * @file        10e_Sleep_Schedule.ino
 * @brief       Wakes every ten seconds, reads the TMP117, and stores the reading in a ring of ten slots in RTC
 *              memory. On every tenth wake it prints the whole log, one line per slot, and then goes back to sleep.
 *              For details, connection diagram and more, check out the example documentation at: <link placeholder>
 * @author      Borna Zelenika @ soldered.com
 ***************************************************
 */

#include <Wire.h>

// From the Library Manager. Search for "Soldered TMP117", by Soldered.
#include "Soldered-TMP117.h"

// The LED is on IO38 and the TMP117 is on the Qwiic cable.
const int LED_PIN = 38;
const uint8_t TMP117_ADDRESS = 0x49;

// How long the board sleeps between two readings, in seconds.
const uint64_t SLEEP_SECONDS = 10;

// The log holds this many readings. The program prints it every time this many wakes have gone by.
const int RING_SIZE = 10;

// All of these stay in RTC memory through deep sleep. After a power-on or a reset they start from zero.
RTC_DATA_ATTR int wakeCount = 0;
RTC_DATA_ATTR float ring[RING_SIZE];

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

// Prints the whole ring, oldest reading first. The slot after the newest one holds the oldest reading, and the
// remainder (%) makes the count wrap round from the last slot back to the first.
void printLog() {
  Serial.printf("log of the last %d wakes:\n", RING_SIZE);

  for (int i = 0; i < RING_SIZE; i++) {
    int slot = (wakeCount + i) % RING_SIZE;
    int wakeNumber = wakeCount - RING_SIZE + 1 + i;
    Serial.printf("  wake %d  %.2f C\n", wakeNumber, ring[slot]);
  }
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
  Wire.begin();

  // The sensor keeps its settings through sleep, so it is configured only once. The wait gives it time to finish
  // its first one-second conversion.
  if (firstBoot) {
    tmp.init(nullptr);
    tmp.setConvMode(TMP117_CMODE::CONTINUOUS);
    tmp.setConvTime(TMP117_CONVT::C1S);
    delay(1100);

    Serial.println();
    Serial.println("sleep schedule: one reading every 10 s, the log prints on every tenth wake");
  }

  // A missing sensor still gives a number, so ask it who it is. A failed wake is not counted and nothing is stored.
  if (tmp.getDeviceID() != 0x117) {
    Serial.println("no TMP117 on the bus. Check the Qwiic cable.");
    blink(5);
    sleepAgain();
  }

  float celsius = tmp.getTemperature();

  // Wake number n goes into slot n - 1, and the remainder wraps the slot number round after the last one.
  int slot = wakeCount % RING_SIZE;
  ring[slot] = celsius;
  wakeCount++;

  Serial.printf("wake %d: %.2f C, stored in slot %d\n", wakeCount, celsius, slot + 1);

  // Every tenth wake, print the log and blink twice. Every other wake blinks once.
  if (wakeCount % RING_SIZE == 0) {
    printLog();
    blink(2);
  } else {
    blink(1);
  }

  sleepAgain();
}

void loop() {
  // Never reached. Deep sleep restarts the sketch from setup() every time.
}
