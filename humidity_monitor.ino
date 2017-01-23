/*
 * Drafted by Joel Susaya
 * 
 * Please see the following sources:
   VirtualDelay: http://www.avdweb.nl/arduino/libraries/virtualdelay.html
   VirtualDelay allows for easy non-blocking delays/timers so you can make callbacks
   at set intervals. It has additional functionality you can read about at the website above.

   SevSeg: https://github.com/DeanIsMe/SevSeg
   SevSeg looks like a good library to use for the 4 digit 7 segment display,
   but I wasn't sure how to use it with a shift register.

   DHT11Lib: http://playground.arduino.cc/Main/DHT11Lib
   The library header and cpp file can be found towards the bottom of the page. I used this
   library specifically for the DHT11 because while using a general DHT library with VirtualDelay,
   I experienced sampling timeout issues. Using this library solved that issue.

   Here's a similar project by Rui Santos: https://gist.github.com/ruisantos16/5419223
   The project uses a similar (or the same) display and the same shift register. A temperature
   sensor is used instead.

   TODO:
   * Figure out how to clean up code.
   * Clean up code.
   * Consolidate DHT11 sampling/averaging pieces.
*/

#include <VirtualDelay.h>
#include <dht11.h>

// verbose mode for serial output (for debugging)
// Currently, when this is true, the display refreshes strangely due to the timing
const bool VERBOSE = true;

// Create a DHT11 object to interface with the sensor
dht11 DHT11;

// Use VirtualDelay to keep track of the DHT11 sample rate (1hz or 1000ms)
// This allows the display to scan without being blocked by the delay() command
VirtualDelay DHT_TIMER(millis);

// Set DHT11 Pin
const int DHT11_PIN = 2;

// Display setup
// set  up pins for digital display
const int DIGIT_PINS[4] = {4, 5, 6, 7};

// Shift register pins; 74HC595
const int DATA_PIN = 8;
const int LATCH_PIN = 9;
const int CLOCK_PIN = 10;

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

// Create an array to keep track of the last SAMPLE_SIZE samples so we can display the average
// Also create a counter to iterate over the samples
// Because the samples are updated once per second, the SAMPLE_SIZE is effectively equal
// to how many seconds of samples we're storing.
const int SAMPLE_SIZE = 60;
int samples[SAMPLE_SIZE];
int current_sample = 0;

// buffered_digits keeps track of which numbers or characters should show up on each digit display
// scan_digit keeps track of which segment is being updated during any given loop
int buffered_digits[4];
int scan_digit = 0;

// For my setup, LOW is on for the digit pins, while HIGH is off.
bool DIGIT_ON = LOW;
bool DIGIT_OFF = HIGH;

// The rest of the pins are the opposite, so naming them this way made it easier for me to read/
bool PIN_ON = HIGH;
bool PIN_OFF = LOW;

// Function prototypes
int sampleDHT();
float getSamplesAverage();
void bufferDigits(int number);
void updateDisplay();

/*
   Sets serial baud rate to 9600.
   Sets all pins to output mode.
*/
void setup() {

  if (VERBOSE) {
    Serial.begin(9600);
  }

  // Set pins to output mode
  for (int i = 0; i < 4; i++) {
    pinMode(DIGIT_PINS[i], OUTPUT);
  }
  pinMode(LATCH_PIN, OUTPUT);
  pinMode(CLOCK_PIN, OUTPUT);
  pinMode(DATA_PIN, OUTPUT);

  /* Sample the DHT11 once and populate the entire sample array with the initial
     value. That way when we average the samples, we won't need additional logic to
     deal with the case of the array not being full or having values of 0.
  */
  int initial_sample = sampleDHT();
  for (int sample = 0; sample < SAMPLE_SIZE; sample++) {
    samples[sample] = initial_sample;
  }
}

// Each loop checks to see if it's been 1000 ms since we last sampled the DHT11
// and if so it samples it and updates the buffered_digits.
// The display is updated each loop regardless of whether the DHT11 was sampled.
void loop() {
  
  // If the timer is done, we sample the DHT and store that as the current sample
  if (DHT_TIMER.done(1000)) {
    if (VERBOSE) {
      Serial.print("Current Sample: ");
      Serial.println(current_sample);
    }
    
    // Sample the DHT and check the status; if we got a -1 from sampling, then
    // return, otherwise, store the sample as the current sample.
    int sample = sampleDHT();
    if (sample == -1) return;
    samples[current_sample] = sample;

    // Get the average of the samples and multiply it by 100 to store the digits in
    // an integer.
    int average = (int)(getSamplesAverage() * 100);

    // Buffer the average so the display can be updated
    bufferDigits(average);

    // Increase the current_sample and reset back to 0 if we've reached the end
    current_sample++;
    if (current_sample >= SAMPLE_SIZE) {
      current_sample = 0;
    }

    if (VERBOSE) {
      updateDisplay();
      char message[256], *log_message = message;
      log_message += sprintf(log_message, "Samples: ");
      for (int sample = 0; sample < SAMPLE_SIZE; sample++) {
        log_message += sprintf(log_message,"%d,",samples[sample]);
      }
      Serial.println(message);
    }
  }

  // Update the display
  updateDisplay();

  // Add a delay to prevent the refresh from being too high (causes 'dimming' and/or excessive blinking)
  delay(2);
}

// Sample the DHT and buffer the digits from it into the buffered_digits array.
int sampleDHT() {
  int chk = DHT11.read(DHT11_PIN);
  if (chk == DHTLIB_OK) {
    if (VERBOSE) {
      Serial.print("Humidity (%): ");
      Serial.println(DHT11.humidity);
    }
    return (int)DHT11.humidity;
  }

  /* Status logging using DHT11 library
    /* */
    if (VERBOSE) {
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
    }
    /* */

  return -1;
}

// Too lazy to encapsulate this properly. Iterates over the samples array to get
// the average and return it. The samples array is populated with the initial
// humidity reading during setup(), so there's no need to worry about null or 0
// values.
float getSamplesAverage() {
  int sum;
  for (int sample = 0; sample < SAMPLE_SIZE; sample++) {
    sum += samples[sample];
  }
  return (float)sum / SAMPLE_SIZE;
}

// Take a float and buffer 4 significant digits in this form: %d%d.%d%d
// The decimal place will be taken care of during the updateDisplay() step.
// This simply buffers the digits as integers.
void bufferDigits(int number) {
  buffered_digits[0] = (int)(number % 10);
  number /= 10;
  buffered_digits[1] = number % 10;
  number /= 10;
  buffered_digits[2] = number % 10;
  number /= 10;
  buffered_digits[3] = number;
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
  digitalWrite(LATCH_PIN, PIN_OFF);
  shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, B00000000);
  digitalWrite(LATCH_PIN, PIN_ON);

  // Turn on the pin that we're updating this cycle
  // This is the only pin that will display during this cycle
  digitalWrite(DIGIT_PINS[scan_digit], DIGIT_ON);

  // Update the shift register to display a the number for the current digit.
  // If we're using the 3 digit from the right (scan_digit == 2), then we use an |
  // to switch on the bit that controls the "." segment on that digit.
  digitalWrite(LATCH_PIN, PIN_OFF);
  if (scan_digit == 2) {
    shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, (DIGIT_CODE_MAP[buffered_digits[scan_digit]]) | B10000000);
  }
  else {
    shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, DIGIT_CODE_MAP[buffered_digits[scan_digit]]);
  }
  digitalWrite(LATCH_PIN, PIN_ON);

  // Increase the scan_digit we're on and reset it to 0 if we've gone over 1.
  // Increase this to three to allow all four digit displays to be used [would require new bufferDigits()].
  scan_digit++;
  if (scan_digit > 3) {
    scan_digit = 0;
  }
  /* */
}
