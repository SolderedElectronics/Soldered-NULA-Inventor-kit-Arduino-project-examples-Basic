/**
 **************************************************
 *
 * @file        6.1_Deep_Sleep_Wake_Sources_And_RTC_Memory.ino
 * @brief       Two counters show what survives deep sleep and what does not. The board sleeps on a timer, on a
 *              button, on either, and finally on the onboard RTC's alarm at the start of the next minute.
 *              For details, connection diagram and more, check out the example documentation at: <link placeholder>
 * @author      Soldered Electronics
 ***************************************************
 */

#include <Wire.h>

// From the Library Manager, not this repository. Search for "Soldered PCF85063A RTC Arduino Library", by Soldered.
#include "PCF85063A-SOLDERED.h"

/*
The button is on IO21 and the LED on IO38.

IO21 needs a pull-up resistor of its own here, which it did not in example 2.1: the internal pull-up switches off
while the chip is asleep, so a wake pin left to float drifts down, looks like a press, and wakes the board over and
over. It is also one of this chip's RTC-capable pins, IO0 to IO21, and only those can wake it from deep sleep.
*/
const int BUTTON_PIN = 21;
const int LED_PIN = 38;

/*
  MODE_TIMER      sleep for a fixed number of seconds
  MODE_BUTTON     sleep until the button is pressed, with no timer at all
  MODE_EITHER     both, and the sketch reports which one woke it
  MODE_RTC_ALARM  wake at the start of the next minute, from the RTC on the board
*/
#define MODE_TIMER     0
#define MODE_BUTTON    1
#define MODE_EITHER    2
#define MODE_RTC_ALARM 3

const int MODE = MODE_RTC_ALARM;

const int SLEEP_SECONDS = 10;

// The two deliberate pauses in setup(), named so the awake time can say how much of itself was work and how much
// was waiting. The serial wait gives the USB port time to come back; without it the first lines of a boot are lost.
// A logger nobody is watching sets both to 0.
const int SERIAL_WAIT_MS = 1000;
const int BLINK_MS = 200;

/*
The onboard PCF85063A sits on the same I2C bus as the Qwiic connector at 0x51. Its interrupt output goes to IO17,
inside the board and not on a header, already pulled up to 3V3 there, so MODE_RTC_ALARM needs nothing wired.

That mode also arms a timer as a safety net: if the alarm is set to a time that cannot arrive, the board still comes
back and says the timer woke it.
*/
const int RTC_INT_PIN = 17;
const int RTC_SAFETY_TIMEOUT_S = 90;

// RTC_DATA_ATTR puts a variable in the small block of memory that stays powered while the rest of the chip is off.
RTC_DATA_ATTR int rtcBootCount = 0;

// An ordinary global, for comparison.
int normalBootCount = 0;

// Nothing this object hands back is current until readTime() has been called, so a forgotten readTime() looks
// exactly like a clock that has stopped.
PCF85063A rtc;

void setup() {
  Serial.begin(115200);
  delay(SERIAL_WAIT_MS);

  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT);

  rtcBootCount++;
  normalBootCount++;

  Serial.println();
  Serial.print("RTC memory counter:  ");
  Serial.println(rtcBootCount);
  Serial.print("ordinary counter:    ");
  Serial.println(normalBootCount);

  // The ordinary counter reads 1 every time. Deep sleep is not pausing: the chip stops and starts again from the
  // top of setup() with every global back at its initial value, as if you had pressed reset.

  Serial.print("woken by: ");
  Serial.println(wakeupReasonName());

  // Blink so there is something to see from across the room.
  digitalWrite(LED_PIN, HIGH);
  delay(BLINK_MS);
  digitalWrite(LED_PIN, LOW);

  // Peripherals do not survive either. I2C, PWM, Wi-Fi and anything a library set up have to be initialised again
  // on every wake. Forgetting this gives a sketch that works once and then quietly stops.
  Wire.begin();

  switch (MODE) {
    case MODE_TIMER:
      sleepOnTimer();
      break;
    case MODE_BUTTON:
      sleepOnButton();
      break;
    case MODE_EITHER:
      sleepOnEither();
      break;
    case MODE_RTC_ALARM:
      sleepUntilNextMinute();
      break;
  }
}

void loop() {
  // Never reached. The board sleeps at the end of setup() and starts again from the top when it wakes.
}

// The argument is in microseconds, which is why every example using it multiplies.
void sleepOnTimer() {
  Serial.print("sleeping for ");
  Serial.print(SLEEP_SECONDS);
  Serial.println(" seconds");

  printAwakeTime((unsigned long)SLEEP_SECONDS * 1000UL);
  Serial.flush();

  esp_sleep_enable_timer_wakeup((uint64_t)SLEEP_SECONDS * 1000000ULL);
  esp_deep_sleep_start();
}

// No timer at all: the board stays asleep indefinitely until the button pulls IO21 to ground. ext0 watches one
// RTC-capable pin for one level, and 0 means LOW. Sleeping is only useful if something can wake you.
void sleepOnButton() {
  Serial.println("sleeping until the button is pressed");
  printAwakeTime(0);
  Serial.flush();

  esp_sleep_enable_ext0_wakeup((gpio_num_t)BUTTON_PIN, 0);
  esp_deep_sleep_start();
}

void sleepOnEither() {
  Serial.print("sleeping for up to ");
  Serial.print(SLEEP_SECONDS);
  Serial.println(" seconds, or until the button is pressed");
  printAwakeTime((unsigned long)SLEEP_SECONDS * 1000UL);
  Serial.flush();

  esp_sleep_enable_timer_wakeup((uint64_t)SLEEP_SECONDS * 1000000ULL);
  esp_sleep_enable_ext0_wakeup((gpio_num_t)BUTTON_PIN, 0);
  esp_deep_sleep_start();
}

/*
An interval and a schedule are different things. A timer wake sleeps ten seconds from whenever the last boot
finished, so every wake adds that boot's length to the gap and they drift steadily later. The RTC is not counting
from now, it is holding the time, so an alarm on the zeroth second fires at the start of every minute forever.
Leave this running for ten minutes and check the timestamps.
*/
void sleepUntilNextMinute() {
  rtc.begin();
  rtc.readTime();

  // Set the clock once so the printed times mean something. After that it runs on its own, through deep sleep and
  // through a power cut if a battery is fitted.
  if (rtcBootCount == 1) {
    rtc.setTime(12, 0, 0);
    rtc.setDate(1, 1, 1, 2026);
    rtc.readTime();
  }

  Serial.print("RTC says ");
  printTwoDigits(rtc.getHour());
  Serial.print(":");
  printTwoDigits(rtc.getMinute());
  Serial.print(":");
  printTwoDigits(rtc.getSecond());
  Serial.println();

  // 99 in any field means "do not care about this one".
  rtc.setAlarm(0, 99, 99, 99, 99);

  Serial.println("sleeping until the RTC alarm at the start of the next minute");
  printAwakeTime(60000UL);
  Serial.flush();

  esp_sleep_enable_ext0_wakeup((gpio_num_t)RTC_INT_PIN, 0);

  // The safety net described at the top of this file.
  esp_sleep_enable_timer_wakeup((uint64_t)RTC_SAFETY_TIMEOUT_S * 1000000ULL);

  esp_deep_sleep_start();
}

/*
The fraction of each cycle spent awake is the number that decides battery life, and unlike current it needs no
meter: the chip draws milliamps awake and microamps asleep, so multiplying that fraction by the two datasheet
figures gives an estimate you measured yourself.

Watch how much of it is the serial wait and the blink rather than work, which is why 7.2 times each phase
separately instead of the boot as a whole.
*/
void printAwakeTime(unsigned long cycleMs) {
  unsigned long awakeMs = millis();
  unsigned long overheadMs = (unsigned long)SERIAL_WAIT_MS + (unsigned long)BLINK_MS;

  Serial.print("awake for ");
  Serial.print(awakeMs);
  Serial.print(" ms");

  if (cycleMs > 0) {
    Serial.print(" of every ");
    Serial.print(cycleMs);
    Serial.print(" ms  (");
    Serial.print(100.0 * awakeMs / cycleMs, 1);
    Serial.print(" %)");
  }
  Serial.println();

  Serial.print("  of which ");
  Serial.print(overheadMs);
  Serial.print(" ms is the serial wait and the blink, so the work took ");
  Serial.print(awakeMs > overheadMs ? awakeMs - overheadMs : 0);
  Serial.println(" ms");
}

void printTwoDigits(int value) {
  if (value < 10) {
    Serial.print("0");
  }
  Serial.print(value);
}

const char *wakeupReasonName() {
  switch (esp_sleep_get_wakeup_cause()) {
    case ESP_SLEEP_WAKEUP_TIMER: return "timer";
    case ESP_SLEEP_WAKEUP_EXT0:  return "a pin (button or RTC alarm)";
    case ESP_SLEEP_WAKEUP_EXT1:  return "a pin group";
    case ESP_SLEEP_WAKEUP_GPIO:  return "GPIO";
    default:                     return "power on or reset, not a wake";
  }
}

/*
Measuring this board's current: reading it over USB tells you nothing useful, because the CH340 stays enumerated and
draws far more than the sleeping processor. Battery current is the number that matters. The onboard WS2812B also
leaks a little even when dark, and opening JP1 is part of getting down to single-digit microamps.
*/
