#include <VirtualDelay.h>
#include <dht11.h>

const int DHT11_PIN = 2;
dht11 DHT11;

VirtualDelay dht_timer(millis);

/*
  const int SAMPLE_RATE = 1000; // 1HZ sample rate for DHT
  int sample_timer; // timer for making sure we don't sample the DHT too quickly
*/

// Display setup
// set  up pins for digital display
const int DIGIT_PINS[4] = {4, 5, 6, 7};
const int LATCH_PIN = 11;  //74HC595  pin 12 STCP
const int CLOCK_PIN = 12; //74HC595  pin 11 SHCP
const int DATA_PIN = 10;   //74HC595  pin 14 DS

static const byte DIGIT_CODE_MAP[] = {
  //     GFEDCBA  Segments      7-segment map:
  B00111111, // 0   "0"          AAA
  B00000110, // 1   "1"         F   B
  B01011011, // 2   "2"         F   B
  B01001111, // 3   "3"          GGG
  B01100110, // 4   "4"         E   C
  B01101101, // 5   "5"         E   C
  B01111101, // 6   "6"          DDD
  B00000111, // 7   "7"
  B01111111, // 8   "8"
  B01101111, // 9   "9"
  B01110111, // 65  'A'
  B01111100, // 66  'b'
  B00111001, // 67  'C'
  B01011110, // 68  'd'
  B01111001, // 69  'E'
  B01110001, // 70  'F'
  B00111101, // 71  'G'
  B01110110, // 72  'H'
  B00000110, // 73  'I'
  B00001110, // 74  'J'
  B01110110, // 75  'K'  Same as 'H'
  B00111000, // 76  'L'
  B00000000, // 77  'M'  NO DISPLAY
  B01010100, // 78  'n'
  B00111111, // 79  'O'
  B01110011, // 80  'P'
  B01100111, // 81  'q'
  B01010000, // 82  'r'
  B01101101, // 83  'S'
  B01111000, // 84  't'
  B00111110, // 85  'U'
  B00111110, // 86  'V'  Same as 'U'
  B00000000, // 87  'W'  NO DISPLAY
  B01110110, // 88  'X'  Same as 'H'
  B01101110, // 89  'y'
  B01011011, // 90  'Z'  Same as '2'
  B00000000, // 32  ' '  BLANK
  B01000000, // 45  '-'  DASH
};

int buffered_digits[4] = {0, 0, 0, 0};
int scan_digit = 0;

bool DIGIT_ON = LOW;
bool DIGIT_OFF = HIGH;

// Function prototypes
void bufferDigits(int num);
void sampleDHT();
void updateDisplay();

void displayTest();

void setup() {
  Serial.begin(9600);
  for (int i = 0; i < 4; i++) {
    pinMode(DIGIT_PINS[i], OUTPUT);
  }
  pinMode(LATCH_PIN, OUTPUT);
  pinMode(CLOCK_PIN, OUTPUT);
  pinMode(DATA_PIN, OUTPUT);
}

void loop() {
  if (dht_timer.done(1000)) {
    sampleDHT();
  }
  updateDisplay();
  //displayTest();
}

// Take a two digit number and store it in the buffered_digits array
void bufferDigits(int num) {
  buffered_digits[0] = num % 10;
  num /= 10;
  buffered_digits[1] = num;
}

// Sample the DHT and buffer the digits from it into the buffered_digits array.
void sampleDHT() {
  int chk = DHT11.read(DHT11_PIN);
  Serial.print("Read sensor: ");
  switch (chk) {
    case DHTLIB_OK:
      Serial.println("OK");
      break;
    case DHTLIB_ERROR_CHECKSUM:
      Serial.println("Checksum error");
      break;
    case DHTLIB_ERROR_TIMEOUT:
      Serial.println("Time out error");
      break;
    default:
      Serial.println("Unknown error");
      break;
  }

  // Buffer digits globally because whatever
  bufferDigits((int)DHT11.humidity);

  // Debugging messages
  /* */
  char status_log_chars[12];
  sprintf(status_log_chars, "D1: %d, D2: %d", buffered_digits[0], buffered_digits[1]);
  Serial.println(status_log_chars);
  /* */
  Serial.print("Humidity (%): ");
  Serial.println((int)DHT11.humidity);
  /* */
}

void updateDisplay() {
  for (byte j = 0; j < 4; j++) {
    digitalWrite(DIGIT_PINS[j], DIGIT_OFF);
  }

  digitalWrite(LATCH_PIN, LOW);
  shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, B00000000);
  digitalWrite(LATCH_PIN, HIGH);
  
  digitalWrite(DIGIT_PINS[scan_digit], DIGIT_ON);

  digitalWrite(LATCH_PIN, LOW);
  shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, ~DIGIT_CODE_MAP[buffered_digits[scan_digit]]);
  digitalWrite(LATCH_PIN, HIGH);

  scan_digit++;
  if (scan_digit > 1){
    scan_digit = 0;
  }
  /* */
}

// Temp function for testing; to be deleted
void displayTest() {
  for (byte j = 0; j < 4; j++) {
    digitalWrite(DIGIT_PINS[j], HIGH);
  }

  digitalWrite(LATCH_PIN, LOW);
  shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, B00000000);
  digitalWrite(LATCH_PIN, HIGH);
  
  digitalWrite(DIGIT_PINS[0], LOW);
  
  digitalWrite(LATCH_PIN, LOW);
  shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, DIGIT_CODE_MAP[1]);
  digitalWrite(LATCH_PIN, HIGH);
  
}

