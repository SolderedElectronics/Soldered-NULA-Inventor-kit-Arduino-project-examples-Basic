/**
 **************************************************
 *
 * @file        09f_Temperature_Cross_Check.ino
 * @brief       Reads the room temperature from the TMP117 and the temperature of the ESP32-S3 chip itself every two
 *              seconds, prints both with their difference, and keeps a running mean of that difference.
 *              For details, connection diagram and more, check out the example documentation at: <link placeholder>
 * @author      Borna Zelenika @ soldered.com
 ***************************************************
 */

#include <Wire.h>

// From the Library Manager. Search for "Soldered TMP117", by Soldered.
#include "Soldered-TMP117.h"

// The TMP117 is on the Qwiic cable at the address from T22. The chip's own sensor needs no pin and no wire.
const uint8_t TMP117_ADDRESS = 0x49;

// How long to wait between two readings, in milliseconds.
const unsigned long READ_INTERVAL_MS = 2000;

Soldered_TMP117 tmp(TMP117_ADDRESS);

// The running mean of the difference is kept as a sum and a count, so no list of old readings is needed.
float differenceSum = 0.0;
int differenceCount = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);

  Wire.begin();
  tmp.init(nullptr);

  Serial.println();
  Serial.println("room is the TMP117, chip is the ESP32-S3. Difference is chip minus room.");
}

// Adds one difference to the running total and returns the mean of every difference so far.
float addToMean(float difference) {
  differenceSum = differenceSum + difference;
  differenceCount = differenceCount + 1;
  return differenceSum / differenceCount;
}

void loop() {
  // Read both sources. temperatureRead() is the sensor inside the chip and needs nothing connected.
  float roomCelsius = tmp.getTemperature();
  uint16_t id = tmp.getDeviceID();
  float chipCelsius = temperatureRead();

  // The check from T22. A missing TMP117 still gives a number, so a wrong ID means the room value is not a
  // measurement. Such a reading is left out of the mean, or it would drag the mean towards nonsense.
  if (id != 0x117) {
    Serial.println("device ID reads wrong, so the room value is not trusted. Check the Qwiic cable.");
    delay(READ_INTERVAL_MS);
    return;
  }

  // The difference is data. It tells you how far the chip sits above the room, not how wrong either source is.
  float difference = chipCelsius - roomCelsius;
  float meanDifference = addToMean(difference);

  // %+.2f always prints the sign, so a positive difference shows a plus.
  Serial.printf("room %.2f C  chip %.2f C  difference %+.2f C  mean %+.2f C\n", roomCelsius, chipCelsius, difference,
                meanDifference);

  delay(READ_INTERVAL_MS);
}
