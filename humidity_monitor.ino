#include <VirtualDelay.h>
#include <dht11.h>

// Create a DHT11 object to interface with the sensor
dht11 DHT11;

// Use VirtualDelay to keep track of the DHT11 sample rate (1hz or 1000ms)
// This allows the display to scan without being blocked by the delay() command
VirtualDelay dht_timer(millis);

// Set DHT11 Pin
const int DHT11_PIN = 2;

// Display setup
// set  up pins for digital display
const int DIGIT_PINS[4] = {4, 5, 6, 7};

// Shift register pins
const int DATA_PIN = 8;   //74HC595  pin 14 DS
const int LATCH_PIN = 9;  //74HC595  pin 12 STCP
const int CLOCK_PIN = 10; //74HC595  pin 11 SHCP

// Code map for numbers and characters
static const byte DIGIT_CODE_MAP[] = {
  //             Segments   
  B00111111, // 0   "0"  
  B00000110, // 1   "1" 
  B01011011, // 2   "2" 
  B01001111, // 3   "3"   
  B01100110, // 4   "4"  
  B01101101, // 5   "5" 
  B01111101, // 6   "6" 
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

// buffered_digits keeps track of which numbers or characters should show up on each digit display
// scan_digit keeps track of which segment is being updated during any given loop
int buffered_digits[4] = {0, 0, 0, 0};
int scan_digit = 0;

// For my setup, LOW is on for the digit pins, while HIGH is off.
bool DIGIT_ON = LOW;
bool DIGIT_OFF = HIGH;

// The rest of the pins are the opposite, so naming them this way made it easier for me to read/
bool PIN_ON = HIGH;
bool PIN_OFF = LOW;

// Function prototypes
void bufferDigits(int num);
void sampleDHT();
void updateDisplay();

/* *
Sets serial baud rate to 9600.
Sets all pins to output mode.
/* */
void setup() {
  Serial.begin(9600);
  for (int i = 0; i < 4; i++) {
    pinMode(DIGIT_PINS[i], OUTPUT);
  }
  pinMode(LATCH_PIN, OUTPUT);
  pinMode(CLOCK_PIN, OUTPUT);
  pinMode(DATA_PIN, OUTPUT);
}

// Each loop checks to see if it's been 1000 ms since we last sampled the DHT11
// and if so it samples it and updates the buffered_digits.
// The display is updated each loop regardless of whether the DHT11 was sampled.
void loop() {
  if (dht_timer.done(1000)) {
    sampleDHT();
  }
  updateDisplay();
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

// Status logging using DHT11 library
/* */  
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
/* */
  // Buffer digits globally because whatever
  bufferDigits((int)DHT11.humidity);

  // Debugging messages
  /* *
  char status_log_chars[12];
  sprintf(status_log_chars, "D1: %d, D2: %d", buffered_digits[0], buffered_digits[1]);
  Serial.println(status_log_chars);
  /* */
  Serial.print("Humidity (%): ");
  Serial.println((int)DHT11.humidity);
  /* */
}

// Updates the display each loop. Each loop updates the next digit on the display.
// This scanning happens fast enough that, to the human eye, it looks like multiple
// digits are displaying at once.
void updateDisplay() {
  // Set all pins off to start
  for (byte j = 0; j < 4; j++) {
    digitalWrite(DIGIT_PINS[j], DIGIT_OFF);
  }

  // Set the shift register to blank so that when we turn on the digit pin in the next step
  // there won't be a glow coming from digit segments that should be turned off
  digitalWrite(LATCH_PIN, LOW);
  shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, B00000000);
  digitalWrite(LATCH_PIN, HIGH);

  // Turn on the pin that we're updating this cycle
  // This is the only pin that will display during this cycle
  digitalWrite(DIGIT_PINS[scan_digit], DIGIT_ON);

  // Update the shift register to display a the number for the current digit.
  digitalWrite(LATCH_PIN, LOW);
  shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, DIGIT_CODE_MAP[buffered_digits[scan_digit]]);
  digitalWrite(LATCH_PIN, HIGH);

  // WHY DOES REMOVING THIS BREAK THINGS?!
  Serial.println(buffered_digits[scan_digit]);

  // Increase the scan_digit we're on and reset it to 0 if we've gone over 1.
  // Increase this to three to allow all four digit displays to be used [would require new bufferDigits()].
  scan_digit++;
  if (scan_digit > 1){
    scan_digit = 0;
  }
  /* */
}
