/**
 **************************************************
 *
 * @file        10c_Wake_Tally.ino
 * @brief       The board sleeps for ten seconds or until the button is pressed. Every wake is counted by its
 *              source in RTC memory, and the LED blinks once for the timer and twice for the button.
 *              For details, connection diagram and more, check out the example documentation at: <link placeholder>
 * @author      Borna Zelenika @ soldered.com
 ***************************************************
 */

// The button is on IO21 and the LED on IO38. IO21 has its own 10 kOhm pull-up to 3V3 on the breadboard, because the
// internal one switches off while the chip sleeps.
const int BUTTON_PIN = 21;
const int LED_PIN = 38;

// How long the board sleeps if nobody presses the button.
const int SLEEP_SECONDS = 10;

// One counter for each wake source. RTC_DATA_ATTR keeps them alive through deep sleep. They start at 0 when the
// board is powered on.
RTC_DATA_ATTR int timerWakes = 0;
RTC_DATA_ATTR int buttonWakes = 0;

// Blinks the LED the given number of times.
void blink(int times) {
  for (int i = 0; i < times; i++) {
    digitalWrite(LED_PIN, HIGH);
    delay(150);
    digitalWrite(LED_PIN, LOW);
    delay(150);
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT);

  // Ask the chip what woke it, then count and blink according to the answer.
  esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();

  if (cause == ESP_SLEEP_WAKEUP_TIMER) {
    timerWakes++;
    blink(1);
  } else if (cause == ESP_SLEEP_WAKEUP_EXT0) {
    buttonWakes++;
    blink(2);
  } else {
    // Power on or reset: three blinks so you can tell it apart from a real wake.
    blink(3);
  }

  Serial.println();
  Serial.print("timer wakes:  ");
  Serial.println(timerWakes);
  Serial.print("button wakes: ");
  Serial.println(buttonWakes);

  // If the button is still held down, the pin is still LOW and the board would wake again the moment it sleeps.
  // Wait here until the button is let go.
  while (digitalRead(BUTTON_PIN) == LOW) {
    delay(10);
  }

  Serial.flush();

  // Arm both sources. Whichever comes first wakes the board.
  esp_sleep_enable_timer_wakeup((uint64_t)SLEEP_SECONDS * 1000000ULL);
  esp_sleep_enable_ext0_wakeup((gpio_num_t)BUTTON_PIN, 0);
  esp_deep_sleep_start();
}

void loop() {
  // Never reached. The board sleeps at the end of setup() and starts again from the top when it wakes.
}
