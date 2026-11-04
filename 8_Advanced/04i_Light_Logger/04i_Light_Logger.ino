/**
 **************************************************
 *
 * @file        04i_Light_Logger.ino
 * @brief       Reads the photoresistor on IO4 twice a second into a ring buffer that holds the last minute, and once
 *              a second prints the latest reading with the minimum, maximum and average over the whole buffer.
 *              For details, connection diagram and more, check out the example documentation at: <link placeholder>
 * @author      Borna Zelenika @ soldered.com
 ***************************************************
 */

// The middle of the photoresistor divider, same pin as T9 and T10 used.
const int LDR_PIN = 4;

// One reading every 500 ms, and one line printed every 1000 ms.
const unsigned long READ_INTERVAL_MS = 500;
const unsigned long PRINT_INTERVAL_MS = 1000;

// The buffer holds 120 readings, which at two a second is exactly one minute.
const int BUFFER_SIZE = 120;

// The ring buffer: a fixed row of slots that the newest reading keeps overwriting the oldest one in.
int readings[BUFFER_SIZE];

// The slot the next reading goes into, how many slots hold a reading so far, and the sum of all those readings.
int nextSlot = 0;
int readingCount = 0;
long runningSum = 0;

// The most recent reading, and the times of the last read and the last print.
int latestReading = 0;
unsigned long lastReadMs = 0;
unsigned long lastPrintMs = 0;

// Puts a new reading into the buffer. Once the buffer is full, the slot being overwritten holds the oldest reading,
// so that value is taken off the running sum before the new one is added. The average never needs a loop.
void addReading(int value) {
  if (readingCount == BUFFER_SIZE) {
    runningSum = runningSum - readings[nextSlot];
  }

  readings[nextSlot] = value;
  runningSum = runningSum + value;
  latestReading = value;

  // Move on to the next slot, and go back to the first one after the last.
  nextSlot = (nextSlot + 1) % BUFFER_SIZE;

  // Count the slots in use, until they are all in use.
  if (readingCount < BUFFER_SIZE) {
    readingCount++;
  }
}

// Prints the latest reading with the minimum, maximum and average of everything in the buffer.
void printSummary() {
  // The minimum and maximum have to look at every reading in use. Slots are filled from 0 upwards, so
  // until the buffer is full the readings are in slots 0 to readingCount - 1.
  int lowest = readings[0];
  int highest = readings[0];

  for (int i = 1; i < readingCount; i++) {
    if (readings[i] < lowest) {
      lowest = readings[i];
    }
    if (readings[i] > highest) {
      highest = readings[i];
    }
  }

  // The average comes straight from the running sum.
  float average = (float)runningSum / readingCount;

  Serial.print("Now ");
  Serial.print(latestReading);
  Serial.print(", min ");
  Serial.print(lowest);
  Serial.print(", max ");
  Serial.print(highest);
  Serial.print(", average ");
  Serial.print(average, 1);
  Serial.print(" (");
  Serial.print(readingCount);
  Serial.println(" readings)");
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  // The same 12-bit, 3.3 V range as the other analog examples.
  analogReadResolution(12);
  analogSetPinAttenuation(LDR_PIN, ADC_11db);

  Serial.println("LIGHT LOGGER: one reading every 500 ms, the last 120 (one minute) kept.");

  // Take the first reading straight away, so the buffer is never empty when the first line is printed.
  addReading(analogRead(LDR_PIN));
  lastReadMs = millis();
  lastPrintMs = millis();
}

void loop() {
  unsigned long now = millis();

  // Every 500 ms, take a reading and put it in the buffer.
  if (now - lastReadMs >= READ_INTERVAL_MS) {
    lastReadMs = now;
    addReading(analogRead(LDR_PIN));
  }

  // Every second, print the summary.
  if (now - lastPrintMs >= PRINT_INTERVAL_MS) {
    lastPrintMs = now;
    printSummary();
  }
}
