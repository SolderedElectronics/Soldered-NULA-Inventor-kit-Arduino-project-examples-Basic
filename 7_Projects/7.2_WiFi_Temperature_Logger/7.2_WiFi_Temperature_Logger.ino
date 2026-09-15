/**
 **************************************************
 *
 * @file        7.2_WiFi_Temperature_Logger.ino
 * @brief       Sends a message every minute, the moment the temperature moves a full degree, or whenever the
 *              button is pressed. Between those it wakes every two seconds to ask the sensor one question, and
 *              sleeps. Times every phase so you can see where the energy goes.
 *              For details, connection diagram and more, check out the example documentation at: <link placeholder>
 * @author      GreenyHub
 ***************************************************
 */

#include <Wire.h>
#include <WiFi.h>
#include <HTTPClient.h>

// From the Library Manager, not this repository. Search for "Soldered TMP117" and
// "Soldered PCF85063A RTC Arduino Library".
#include "Soldered-TMP117.h"
#include "PCF85063A-SOLDERED.h"

// Fill these in before uploading. The board joins 2.4 GHz networks.
const char *ssid = "your ssid";
const char *password = "your password";

// Open https://webhook.site, copy the unique link it gives you, and paste it here. Anything you send shows up on
// that page as it arrives.
const char *uploadURL = "your unique url";

/*
The only thing on the breadboard is the button on IO21, and the only thing on the Qwiic cable is the sensor. The
RGB LED and the RTC are already on the board.

The button needs a 10 kOhm to 3V3 of its own, because the internal pull-ups switch off during deep sleep and the
pin would otherwise float and wake the board at random. Example 6.1 goes into this.
*/
const int BUTTON_PIN = 21;

// The onboard PCF85063A interrupt output, internal to the board and already pulled up there, so nothing to wire.
const int RTC_INT_PIN = 17;

const uint8_t TMP117_ADDRESS = 0x49;

/*
The two things that make the board send: a full degree of movement since the last message, and the clock.

CHANGE_C is written into the sensor as a band either side of the last reading sent, so the TMP117 watches for it
once a second while the processor is off, and latches the answer.

One minute is short for a real logger and is here so the example can be watched. A logger left running for a week
would use fifteen or sixty, and nothing else would change.
*/
const float CHANGE_C = 1.0;
const int LOG_INTERVAL_MINUTES = 1;

/*
How often the board wakes just to ask whether that degree has happened yet. This is what "immediately" costs: the
board cannot feel the temperature while asleep and the alert pin is not wired, so the only way to react inside a
minute is to wake and look. Example 6.2 measured one of these checks at about 29 ms of waking for 1.5 ms of asking.
*/
const uint64_t CHECK_SECONDS = 2;

/*
The ring buffer lives in RTC memory, so it survives deep sleep. A reading that fails to upload stays in it and goes
up on the next wake, and several readings travel in one connection. Twenty-four covers an hour at one a minute, and
when it fills the oldest is dropped.
*/
const int BUFFER_SIZE = 24;

RTC_DATA_ATTR float bufTemperature[BUFFER_SIZE];
RTC_DATA_ATTR uint32_t bufTimestamp[BUFFER_SIZE];
RTC_DATA_ATTR int bufCount = 0;
RTC_DATA_ATTR int wakeCount = 0;

// What "changed by a full degree" is measured from. It has to survive deep sleep, or every wake thinks it is first.
RTC_DATA_ATTR float lastSentTemperature = 0;
RTC_DATA_ATTR bool haveSent = false;

// 0x49 because JP2 on the breakout ties ADD0 high. A missing module reads 0.0000 rather than failing, so
// getDeviceID() == 0x117 is the only honest test.
Soldered_TMP117 tmp(TMP117_ADDRESS);

// The onboard RTC at 0x51, nothing to wire. readTime() before getHour() and friends, or they return the previous
// read's values.
PCF85063A rtc;

void setup() {
  Serial.begin(115200);

  bool firstBoot = (wakeCount == 0);

  // Only the first boot waits for the monitor. The CH340 stays connected to the computer while the processor
  // sleeps, so the monitor never drops.
  if (firstBoot) {
    delay(1000);
  }

  wakeCount++;

  Wire.begin();

  // Everything above this line is the cost of waking up. Everything below it is this wake doing its job.
  uint32_t startupUs = micros();

  if (firstBoot) {
    // The chip loader prints nine lines before this sketch gets an instruction, on every wake. This sets a flag the
    // ROM checks on its way out of deep sleep. The banner at the top of the log is the one boot it cannot affect.
    esp_deep_sleep_disable_rom_logging();

    Serial.println();
    Serial.println("7.2 wifi temperature logger");
  }

  rtc.begin();
  if (firstBoot) {
    rtc.setTime(12, 0, 0);
    rtc.setDate(1, 1, 1, 2026);
    configureSensor();
  }
  rtc.readTime();

  // Ask whether anything happened while the processor was off. The flags latch until the config register is read,
  // and reading it both asks and re-arms, so this must come before anything else touches the sensor.
  uint16_t config = tmp.readConfig();
  bool changed = ((config >> 15) & 1) || ((config >> 14) & 1);

  bool alarmWake = wokeOnPin(RTC_INT_PIN) || firstBoot;
  bool buttonWake = wokeOnPin(BUTTON_PIN);

  // Most wakes are the two-second check and find nothing. Those cost two register reads and go straight back to
  // sleep, with one dot printed. That is 6.2's quiet path, and it is what makes an immediate response affordable.
  if (!alarmWake && !buttonWake && !changed) {
    Serial.print(".");
    sleepAgain(false);
  }

  Serial.println();
  Serial.print("wake ");
  Serial.print(wakeCount);
  Serial.print(" by ");
  Serial.println(wakeSource());

  float latest = takeReading();

  bool full = (bufCount >= BUFFER_SIZE);

  if (bufCount > 0) {
    // Why this connection happened travels with the readings, so the log explains itself. A change is named ahead
    // of the schedule because when both are true, the change is the news.
    const char *reason = buttonWake ? "button pressed"
                         : changed  ? "temperature changed by a full degree"
                         : full     ? "buffer full"
                                    : "scheduled upload";

    Serial.print("connecting because of ");
    Serial.println(reason);

    // The RGB LED reports the phase, because a board that spends its life asleep usually has nobody watching serial.
    setLed(0, 0, 40);
    unsigned long uploaded = uploadBuffer(reason);
    setLed(uploaded > 0 ? 0 : 40, uploaded > 0 ? 40 : 0, 0);
    delay(300);
    setLed(0, 0, 0);

    // Move the band only when the message actually went. A failed upload leaves the sensor watching the old mark,
    // which is right: nothing has been reported, so nothing has moved as far as the log is concerned.
    if (uploaded > 0 && !isnan(latest)) {
      lastSentTemperature = latest;
      haveSent = true;
      setBandAround(latest);
    }
  } else {
    Serial.println("nothing in the buffer, so the radio stays off");
  }

  // The cost of being awake against the cost of the work. On a board that sleeps between readings the first number
  // is usually the larger one, and it is the same whether the radio ran or not.
  Serial.print("waking up cost ");
  Serial.print(startupUs / 1000.0, 1);
  Serial.print(" ms, this wake did ");
  Serial.print((micros() - startupUs) / 1000.0, 1);
  Serial.println(" ms of work on top");

  sleepAgain(alarmWake);
}

void loop() {
  // Never reached. Deep sleep restarts the sketch from setup() every time.
}

// Configure once, on the first boot: the TMP117 keeps this in its own EEPROM through sleep, reset and power cuts.
// setAlertMode(ALERT) is the line that matters, since init() leaves it in data-ready mode where the flag is raised
// after every conversion, reporting a change on every wake.
void configureSensor() {
  tmp.init(nullptr);
  tmp.setConvMode(TMP117_CMODE::CONTINUOUS);
  tmp.setConvTime(TMP117_CONVT::C1S);
  tmp.setAlertMode(TMP117_PMODE::ALERT);
}

// Returns the reading just taken, or NAN if there was no sensor to take it from.
float takeReading() {
  if (tmp.getDeviceID() != 0x117) {
    Serial.println("no TMP117 answering. Check the Qwiic cable. Nothing logged this wake.");
    return NAN;
  }

  float celsius = tmp.getTemperature();

  // From the RTC, not millis(), which restarts at zero on every wake and says nothing about when this happened.
  uint32_t stamp = (uint32_t)rtc.getHour() * 3600 + (uint32_t)rtc.getMinute() * 60 + rtc.getSecond();

  if (bufCount < BUFFER_SIZE) {
    bufTemperature[bufCount] = celsius;
    bufTimestamp[bufCount] = stamp;
    bufCount++;
  } else {
    // Full. Drop the oldest, which is the right thing to lose if a network has been down for two hours.
    for (int i = 1; i < BUFFER_SIZE; i++) {
      bufTemperature[i - 1] = bufTemperature[i];
      bufTimestamp[i - 1] = bufTimestamp[i];
    }
    bufTemperature[BUFFER_SIZE - 1] = celsius;
    bufTimestamp[BUFFER_SIZE - 1] = stamp;
  }

  Serial.print("logged ");
  Serial.print(celsius, 3);
  Serial.print(" C at ");
  Serial.print(clockString(stamp));
  Serial.print(", buffer holds ");
  Serial.println(bufCount);

  return celsius;
}

// The limits live in the sensor, so the comparison keeps happening at a few microamps with the processor off. They
// are ordinary registers, unlike the mode settings, so writing them costs no EEPROM.
void setBandAround(float celsius) {
  tmp.setAlertTemperature(celsius - CHANGE_C, celsius + CHANGE_C);

  // Clear anything the old band left latched, or the next wake reports a change that has already been handled.
  tmp.readConfig();
}

// Seconds since midnight as hh:mm:ss. The buffer stores one number per reading because a number is cheap to keep in
// RTC memory; turning it into something readable happens once, where a human will look at it.
String clockString(uint32_t stamp) {
  char buf[9];
  snprintf(buf, sizeof(buf), "%02u:%02u:%02u", (unsigned)(stamp / 3600), (unsigned)((stamp / 60) % 60), (unsigned)(stamp % 60));
  return String(buf);
}

/*
Time every phase separately, because these numbers are the point of the example. Association takes seconds; the
upload itself, once associated, takes a fraction of that. Uploading twenty readings costs barely more than one,
because the expensive part happened before the first byte of data moved.

On a battery board, connecting less often and sending more each time is worth far more than sending less. Wi-Fi is
a power decision before it is a feature, which is why this is the only example in the kit that uses it.
*/
unsigned long uploadBuffer(const char *reason) {
  unsigned long connectStart = millis();

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED && millis() - connectStart < 20000) {
    delay(100);
  }

  unsigned long connectMs = millis() - connectStart;

  if (WiFi.status() != WL_CONNECTED) {
    Serial.print("association failed after ");
    Serial.print(connectMs);
    Serial.println(" ms. Readings stay in the buffer for next time.");
    WiFi.mode(WIFI_OFF);
    return 0;
  }

  Serial.print("associated in ");
  Serial.print(connectMs);
  Serial.println(" ms");

  // Two lines saying who sent this and why, then one line per reading. The unit goes on every line, not in a
  // heading: whoever reads this log will be scrolling rather than starting from the top.
  String payload = "device: NULA DeepSleep ESP32-S3\n";
  payload += "reason: " + String(reason) + "\n";

  for (int i = 0; i < bufCount; i++) {
    payload += clockString(bufTimestamp[i]) + ", " + String(bufTemperature[i], 3) + " C\n";
  }

  unsigned long postStart = millis();

  HTTPClient http;
  http.begin(uploadURL);
  // Say what the body is, so the far end shows it as plain lines rather than guessing at a format.
  http.addHeader("Content-Type", "text/plain");
  int code = http.POST(payload);
  http.end();

  unsigned long postMs = millis() - postStart;

  Serial.print("POST took ");
  Serial.print(postMs);
  Serial.print(" ms, response ");
  Serial.println(code);

  WiFi.mode(WIFI_OFF);

  if (code <= 0) {
    Serial.println("upload failed. Readings stay in the buffer.");
    return 0;
  }

  unsigned long sent = bufCount;
  bufCount = 0;

  Serial.print("sent ");
  Serial.print(sent);
  Serial.print(sent == 1 ? " reading. Association was " : " readings. Association was ");
  Serial.print((float)connectMs / (float)(connectMs + postMs) * 100.0, 0);
  Serial.println(" % of the time the radio was on.");

  return sent;
}

/*
Three wake sources at once, none needing a wire. The timer is the fast one, asking the sensor every CHECK_SECONDS
whether the temperature moved, which is how a change gets reported in seconds rather than at the next minute. It is
the only way to do it here: the TMP117 breakout's ALERT output is on a header that ships unpopulated, and nothing in
this kit needs soldering. The RTC alarm is the heartbeat, and the button is there so you can ask for a message.
*/
void sleepAgain(bool rearmAlarm) {
  // Only touch the alarm on the wake it fired. Arming it is what clears the flag and releases IO17, and the target
  // is worked out from the time now, so re-arming on every check would push it further away forever.
  if (rearmAlarm) {
    rtc.readTime();

    int nextMinute = (rtc.getMinute() + LOG_INTERVAL_MINUTES) % 60;
    rtc.setAlarm(0, nextMinute, 99, 99, 99);

    Serial.print("next alarm at minute ");
    Serial.println(nextMinute);
  }

  Serial.flush();

  // The fast check.
  esp_sleep_enable_timer_wakeup(CHECK_SECONDS * 1000000ULL);

  // ext1 watches a group of pins rather than one, so both the RTC and the button can wake the board. Both lines
  // idle high through a pull-up: the RTC's on the board, the button's on the breadboard.
  uint64_t mask = (1ULL << RTC_INT_PIN) | (1ULL << BUTTON_PIN);
  esp_sleep_enable_ext1_wakeup(mask, ESP_EXT1_WAKEUP_ANY_LOW);

  esp_deep_sleep_start();
}

// Which pin in the ext1 mask pulled low. False on a reset or power-on, when no pin was involved at all.
bool wokeOnPin(int pin) {
  if (esp_sleep_get_wakeup_cause() != ESP_SLEEP_WAKEUP_EXT1) {
    return false;
  }
  return (esp_sleep_get_ext1_wakeup_status() >> pin) & 1;
}

const char *wakeSource() {
  if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_TIMER) {
    return "the check timer, with the sensor flagging a change";
  }
  if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT1) {
    if (wokeOnPin(BUTTON_PIN)) {
      return "the button";
    }
    if (wokeOnPin(RTC_INT_PIN)) {
      return "the RTC alarm";
    }
    return "a pin";
  }
  return "power on or reset";
}

void setLed(uint8_t r, uint8_t g, uint8_t b) {
  rgbLedWrite(RGB_BUILTIN, r, g, b);
}

/*
Every other example spends energy on the thing it is measuring. This one spends it on talking, and the difference in
scale is the lesson: a temperature reading is two milliseconds, joining a Wi-Fi network is two or three thousand.
The radio is a different order of magnitude, and treating it as just another feature flattens a battery in a day.

Both intervals here are set for watching rather than lasting, and moving them is the exercise.
LOG_INTERVAL_MINUTES at 60 sends one message an hour holding sixty readings, one association instead of sixty, and
the data that arrives is identical. CHECK_SECONDS at 30 turns "within two seconds" into "within thirty" and drops
the checking cost fifteen-fold. Neither changes a line of code. The only way to choose them is to decide how late a
report is still useful.
*/
