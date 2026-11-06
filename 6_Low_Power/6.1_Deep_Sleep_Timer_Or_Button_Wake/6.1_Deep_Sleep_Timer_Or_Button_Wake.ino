/**
 **************************************************
 *
 * @file        6.1_Deep_Sleep_Timer_Or_Button_Wake.ino
 * @brief       Two counters show what survives deep sleep and what does not. The board blinks, then sleeps until either ten
 *              seconds have passed or the button is pressed, and reports which one woke it.
 *              For details, connection diagram and more, check out the example documentation at: <link placeholder>
 * @author      Borna Zelenika @ soldered.com
 ***************************************************
 */

// The button is on IO21 and the LED on IO38.
//
// IO21 needs a pull-up resistor of its own here, which it did not in T7: the internal pull-up switches off
// while the chip is asleep, so a wake pin left to float drifts down, looks like a press, and wakes the board over and
// over. It is also one of this chip's RTC-capable pins, IO0 to IO21, and only those can wake it from deep sleep.
const int BUTTON_PIN = 21;
const int LED_PIN = 38;

// How long the board sleeps if nobody presses the button.
const int SLEEP_SECONDS = 10;

// The two deliberate pauses in setup(), named so the awake time can say how much of itself was work and how much
// was waiting. The serial wait gives the USB port time to come back; without it the first lines of a boot are lost.
const int SERIAL_WAIT_MS = 1000;
const int BLINK_MS = 200;

// RTC_DATA_ATTR puts a variable in the small block of memory that stays powered while the rest of the chip is off.
RTC_DATA_ATTR int rtcBootCount = 0;

// An ordinary global, for comparison.
int normalBootCount = 0;

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

  // Ask the chip what woke it and keep the answer as words. The starting text is for the case where none of the
  // wake sources below fired, which means the board was just powered on or reset.
  String wakeReason = "power on or reset, not a wake";
  if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_TIMER) {
    wakeReason = "timer";
  }
  if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT0) {
    wakeReason = "a pin (button or RTC alarm)";
  }

  Serial.print("woken by: ");
  Serial.println(wakeReason);

  // Blink so there is something to see from across the room.
  digitalWrite(LED_PIN, HIGH);
  delay(BLINK_MS);
  digitalWrite(LED_PIN, LOW);

  Serial.print("sleeping for up to ");
  Serial.print(SLEEP_SECONDS);
  Serial.println(" seconds, or until the button is pressed");

  unsigned long awakeMs = millis();
  unsigned long overheadMs = SERIAL_WAIT_MS + BLINK_MS;
  unsigned long workMs = awakeMs - overheadMs;
  unsigned long cycleMs = SLEEP_SECONDS * 1000;
  float percent = 100.0 * awakeMs / cycleMs;

  Serial.print("awake for ");
  Serial.print(awakeMs);
  Serial.print(" ms of every ");
  Serial.print(cycleMs);
  Serial.print(" ms  (");
  Serial.print(percent, 1);
  Serial.println(" %)");

  Serial.print("  of which ");
  Serial.print(overheadMs);
  Serial.print(" ms is the serial wait and the blink, so the work took ");
  Serial.print(workMs);
  Serial.println(" ms");
  Serial.flush();

  // Both sources are armed, and whichever comes first wakes the board.
  esp_sleep_enable_timer_wakeup(SLEEP_SECONDS * 1000000ULL);
  esp_sleep_enable_ext0_wakeup((gpio_num_t)BUTTON_PIN, 0);
  esp_deep_sleep_start();
}

void loop() {
  // Never reached. The board sleeps at the end of setup() and starts again from the top when it wakes.
}
