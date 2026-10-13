/**
 **************************************************
 *
 * @file        6.2_Checking_A_Sensor_Alert_Flag.ino
 * @brief       The TMP117 keeps converting on its own and decides for itself when the temperature has left a band,
 *              then latches a flag. The board sleeps, wakes just long enough to read that one flag, and goes
 *              straight back to sleep unless the sensor has something to say.
 *              For details, connection diagram and more, check out the example documentation at: <link placeholder>
 * @author      Soldered Electronics
 ***************************************************
 */

#include <Wire.h>

// From the Library Manager, not this repository. Search for "Soldered TMP117", by Soldered.
#include "Soldered-TMP117.h"

/*
The LED on IO38 blinks once whenever the sensor actually had something to report. Nothing else is wired, because
everything else travels down the Qwiic cable.

The TMP117 also has a hardware ALERT output, and on a breakout with the header fitted it can pull a wire low and
wake the processor with no code running at all. This example reaches the same decision over the bus instead: the
alert condition is a flag inside the sensor, and a flag can be read.
*/
const int LED_PIN = 38;

const uint8_t TMP117_ADDRESS = 0x49;

// Raising SLEEP_SECONDS does not make the board miss anything. The flag latches, so the sensor holds the fact until
// it is read. Checking less often makes the response slower, it does not lose the event, and that is the difference
// between a flag and a pulse.
const uint64_t SLEEP_SECONDS = 2;
const float BAND_C = 1.0;

RTC_DATA_ATTR int wakeCount = 0;
RTC_DATA_ATTR int quietCount = 0;
RTC_DATA_ATTR float lastTemperature = 0;
RTC_DATA_ATTR bool haveLast = false;

// How long the last quiet check spent working, kept across sleep so the next alert can print the two side by side.
// micros() restarts from setup() on every wake, so a reading after the hardware is up and another before sleeping
// splits a wake into the part you pay for being awake and the part that does the work.
RTC_DATA_ATTR uint32_t quietWorkUs = 0;
RTC_DATA_ATTR bool haveQuiet = false;

// 0x49 because JP2 on the breakout ties ADD0 high. A missing module reads 0.0000 rather than failing, so
// getDeviceID() == 0x117 is the only honest test. Example 5.1 covers it.
Soldered_TMP117 tmp(TMP117_ADDRESS);

void setup() {
  Serial.begin(115200);

  bool firstBoot = (wakeCount == 0);

  // Only the first boot waits for the monitor. The CH340 is a separate chip that stays connected to the computer
  // while the processor sleeps, so the monitor never drops. A board with native USB would disconnect every sleep.
  if (firstBoot) {
    delay(1000);
  }

  wakeCount++;

  pinMode(LED_PIN, OUTPUT);
  Wire.begin();

  // Everything above this line is the cost of waking up. Everything below it is this wake doing its job.
  uint32_t startupUs = micros();

  if (firstBoot) {
    // The boot ROM prints nine lines before the sketch gets an instruction, and a board waking every two seconds
    // prints them every two seconds. This sets a flag the ROM checks on the way out of deep sleep, so every wake
    // after this one is quiet. It cannot silence the first boot, which is why one banner still appears.
    esp_deep_sleep_disable_rom_logging();

    Serial.println();
    Serial.println("6.2 checking a sensor alert flag");
    configureSensor();
  }

  // In alert mode the flags at the top of the config register latch until the register is read, and reading it is
  // what clears them, so this one read both asks the question and arms the sensor for the next.
  //
  // Forgetting it is the classic fault on this sensor. On the wired version the pin stays low, the board wakes the
  // instant it sleeps, and it loops forever at full power.
  uint16_t config = tmp.readConfig();
  bool highAlert = (config >> 15) & 1;
  bool lowAlert = (config >> 14) & 1;

  // The check from example 5.1. A reading from nothing is still a number, so ask the sensor who it is.
  if (tmp.getDeviceID() != 0x117) {
    Serial.println();
    Serial.println("no TMP117 on the bus. Check the Qwiic cable.");
    blink(5);
    sleepAgain();
  }

  // Nothing has been measured yet, so put a band around wherever the room is now and start from there.
  if (firstBoot) {
    setBandAround(tmp.getTemperature());
    sleepAgain();
  }

  // The quiet path, and the point of the example: two registers read and already on the way back to sleep. One dot
  // is printed so you can watch the checks go by.
  if (!highAlert && !lowAlert) {
    quietCount++;
    Serial.print(".");
    quietWorkUs = micros() - startupUs;
    haveQuiet = true;
    sleepAgain();
  }

  // Something happened. This is the only path that costs anything.
  float celsius = tmp.getTemperature();

  Serial.println();
  Serial.print("wake ");
  Serial.print(wakeCount);
  Serial.print(": ");
  Serial.print(highAlert ? "rose past the top of the band" : "fell past the bottom of the band");
  Serial.print(", now ");
  Serial.print(celsius, 3);
  Serial.println(" C");

  if (quietCount > 0) {
    Serial.print(quietCount);
    Serial.print(quietCount == 1 ? " check" : " checks");
    Serial.println(" before this one found nothing");
  }

  if (haveLast) {
    Serial.print("change since the last alert: ");
    Serial.print(celsius - lastTemperature, 3);
    Serial.println(" C");
  }

  lastTemperature = celsius;
  haveLast = true;
  quietCount = 0;

  setBandAround(celsius);

  // Three numbers: what a wake costs before your code runs, what an alert needs on top, and what a check that found
  // nothing needed. On this board the fixed cost dominates, so doing less while awake saves less than not waking at
  // all. Try SLEEP_SECONDS at 30 and watch what happens to the run.
  uint32_t alertWorkUs = micros() - startupUs;

  Serial.print("waking up cost ");
  Serial.print(startupUs / 1000.0, 1);
  Serial.print(" ms, this alert ");
  Serial.print(alertWorkUs / 1000.0, 1);
  Serial.print(" ms on top");
  if (haveQuiet) {
    Serial.print(", a check that found nothing ");
    Serial.print(quietWorkUs / 1000.0, 1);
    Serial.print(" ms");
  }
  Serial.println();

  blink(1);
  sleepAgain();
}

void loop() {
  // Never reached. Deep sleep restarts the sketch from setup() every time.
}

/*
Configure once, on the first boot. The TMP117 stores its configuration in its own EEPROM, so it keeps these settings
through deep sleep, a reset and a power cut, and rewriting them every wake would burn that write budget for nothing.

setAlertMode(ALERT) is the line that matters most: init() leaves the sensor in data-ready mode, where the flag is
raised after every conversion, which would report an alert on every check.
*/
void configureSensor() {
  tmp.init(nullptr);

  tmp.setConvMode(TMP117_CMODE::CONTINUOUS);
  tmp.setConvTime(TMP117_CONVT::C1S);
  tmp.setAlertMode(TMP117_PMODE::ALERT);

  Serial.println("sensor set to alert mode, one conversion per second");
}

// The limits live in the sensor, not here, so once written the processor can be switched off entirely and the
// comparison still happens once a second.
void setBandAround(float celsius) {
  tmp.setAlertTemperature(celsius - BAND_C, celsius + BAND_C);

  Serial.print("band is now ");
  Serial.print(celsius - BAND_C, 2);
  Serial.print(" to ");
  Serial.print(celsius + BAND_C, 2);
  Serial.println(" C. Put a finger on the sensor.");

  // Clear anything the old band left latched, or the next check reports an alert that has already been handled.
  tmp.readConfig();
}

void sleepAgain() {
  Serial.flush();
  esp_sleep_enable_timer_wakeup(SLEEP_SECONDS * 1000000ULL);
  esp_deep_sleep_start();
}

void blink(int times) {
  for (int i = 0; i < times; i++) {
    digitalWrite(LED_PIN, HIGH);
    delay(120);
    digitalWrite(LED_PIN, LOW);
    delay(120);
  }
}

/*
In 6.1 a timer woke the board and the board did all the thinking, running a conversion and some arithmetic to learn
nothing. Here the threshold lives in the sensor, which converts once a second at a few microamps against limits the
processor wrote once and forgot.

The timings say where that pays. Trimming the work to two register reads saves less than you would hope, because
most of a wake is spent waking. What the sensor bought is permission to wake far less often, and a board with the
alert pin on a wire would not need to check at all.

The processor is no longer in charge of the system: it is a resource the sensor calls on.
*/
