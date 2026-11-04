/**
 **************************************************
 *
 * @file        03d_Morse_Keyer.ino
 * @brief       Turns the button into a Morse key. A short press is a dot, a long press is a dash, a pause ends the
 *              letter and the sketch prints it. A longer silence adds a space. The LED on IO38 follows the button.
 *              For details, connection diagram and more, check out the example documentation at: <link placeholder>
 * @author      Borna Zelenika @ soldered.com
 ***************************************************
 */

const int BUTTON_PIN = 21;
const int LED_PIN = 38;

// A press shorter than this is a dot, and a press at least this long is a dash.
const unsigned long DASH_MS = 250;

// Silence after a release that ends the letter, and silence that adds a space between words.
const unsigned long LETTER_GAP_MS = 700;
const unsigned long WORD_GAP_MS = 2000;

// Ignore changes that come sooner than this after the last one, so the contact bounce is not counted.
const unsigned long DEBOUNCE_MS = 30;

// The longest pattern in the table is five symbols, so five is all the sketch keeps for one letter.
const int MAX_SYMBOLS = 5;

// One row of the Morse table: the pattern of dots and dashes, and the character it stands for.
struct MorseCode {
  const char *pattern;
  char character;
};

// The Morse table: every letter and digit. Looking a pattern up here is all the decoding there is.
const MorseCode MORSE_TABLE[] = {
  {".-", 'A'},    {"-...", 'B'},  {"-.-.", 'C'},  {"-..", 'D'},   {".", 'E'},     {"..-.", 'F'},
  {"--.", 'G'},   {"....", 'H'},  {"..", 'I'},    {".---", 'J'},  {"-.-", 'K'},   {".-..", 'L'},
  {"--", 'M'},    {"-.", 'N'},    {"---", 'O'},   {".--.", 'P'},  {"--.-", 'Q'},   {".-.", 'R'},
  {"...", 'S'},   {"-", 'T'},     {"..-", 'U'},   {"...-", 'V'},  {".--", 'W'},    {"-..-", 'X'},
  {"-.--", 'Y'},  {"--..", 'Z'},
  {"-----", '0'}, {".----", '1'}, {"..---", '2'}, {"...--", '3'}, {"....-", '4'},
  {".....", '5'}, {"-....", '6'}, {"--...", '7'}, {"---..", '8'}, {"----.", '9'}
};
const int MORSE_COUNT = sizeof(MORSE_TABLE) / sizeof(MORSE_TABLE[0]);

// The button state machine: is the button down, when did it go down, and when did it last change.
bool wasPressed = false;
unsigned long pressedAtMs = 0;
unsigned long lastEdgeMs = 0;

// The letter being keyed right now, as a text of dots and dashes, and the time of the last release.
char pattern[MAX_SYMBOLS + 1] = "";
int patternLength = 0;
unsigned long lastReleaseMs = 0;

// The message so far, and whether a space is still due after the last letter.
String message = "";
bool spaceDue = false;

// Looks a pattern up in the table and gives back its character, or '?' if the pattern is not in the table.
char decodePattern(const char *keyed) {
  for (int i = 0; i < MORSE_COUNT; i++) {
    if (strcmp(keyed, MORSE_TABLE[i].pattern) == 0) {
      return MORSE_TABLE[i].character;
    }
  }
  return '?';
}

// Adds a dot or a dash to the letter being keyed. A sixth symbol is dropped, because no pattern is that long.
void addSymbol(char symbol) {
  if (patternLength < MAX_SYMBOLS) {
    pattern[patternLength] = symbol;
    patternLength++;
    pattern[patternLength] = '\0';
  }
}

// Prints the message so far in quotes, so a trailing space can be seen.
void printMessage() {
  Serial.print("   message: \"");
  Serial.print(message);
  Serial.println("\"");
}

// Decodes the finished letter, prints it with its pattern, and clears the pattern for the next letter.
void finishLetter() {
  char letter = decodePattern(pattern);
  message += letter;

  Serial.print(pattern);
  Serial.print(" = ");
  Serial.print(letter);
  printMessage();

  patternLength = 0;
  pattern[0] = '\0';
  spaceDue = true;
}

// Adds a space to the message and prints it.
void addSpace() {
  message += ' ';
  Serial.print("(space)");
  printMessage();
  spaceDue = false;
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  Serial.println("MORSE: tap for a dot, hold for a dash. 0.7 s of silence ends a letter, 2 s adds a space.");
}

void loop() {
  unsigned long now = millis();

  // The pull-up makes a press read LOW, so a pressed button gives true here.
  bool pressed = (digitalRead(BUTTON_PIN) == LOW);

  // Act only when the button changes state and the change is not bounce.
  if (pressed != wasPressed && now - lastEdgeMs >= DEBOUNCE_MS) {
    wasPressed = pressed;
    lastEdgeMs = now;

    if (pressed) {
      // The button has just gone down: note the time and show the press on the LED.
      pressedAtMs = now;
      digitalWrite(LED_PIN, HIGH);
    } else {
      // The button has just come up: the length of the press decides dot or dash.
      digitalWrite(LED_PIN, LOW);
      unsigned long heldMs = now - pressedAtMs;
      addSymbol(heldMs < DASH_MS ? '.' : '-');
      lastReleaseMs = now;
    }
  }

  // While the button is up, the silence since the last release decides what happens next.
  if (!wasPressed) {
    unsigned long silenceMs = now - lastReleaseMs;

    if (patternLength > 0 && silenceMs >= LETTER_GAP_MS) {
      finishLetter();
    } else if (patternLength == 0 && spaceDue && silenceMs >= WORD_GAP_MS) {
      addSpace();
    }
  }
}
