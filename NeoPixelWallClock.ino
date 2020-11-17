// NEOPIXEL BEST PRACTICES for most reliable operation:
// - Add 1000 uF CAPACITOR between NeoPixel strip's + and - connections.
// - MINIMIZE WIRING LENGTH between microcontroller board and first pixel.
// - NeoPixel strip's DATA-IN should pass through a 300-500 OHM RESISTOR.
// - AVOID connecting NeoPixels on a LIVE CIRCUIT. If you must, ALWAYS
//   connect GROUND (-) first, then +, then data.
// - When using a 3.3V microcontroller with a 5V-powered NeoPixel strip,
//   a LOGIC-LEVEL CONVERTER on the data line is STRONGLY RECOMMENDED.
// (Skipping these may work OK on your workbench but can fail in the field)

#include <Adafruit_NeoPixel.h>

// Date and time functions using a DS3231 RTC connected via I2C and Wire lib
#include "RTClib.h"

// Which pin on the Arduino is connected to the NeoPixels?
// On a Trinket or Gemma we suggest changing this to 1:

// Pro Micro, 10
#define LED_PIN    10

// Pro Micro, A3
#define TIME_SET_PIN    21

// LED indices for each digit
#define DIGIT_1_INDEX    0
#define DIGIT_2_INDEX    35
#define DIGIT_3_INDEX    72
#define DIGIT_4_INDEX    107

// LED index of colon
#define COLON_INDEX    70

// LED index offsets for each segment (within each digit)
#define SEGMENT_1_OFFSET    0
#define SEGMENT_2_OFFSET    5
#define SEGMENT_3_OFFSET    10
#define SEGMENT_4_OFFSET    15
#define SEGMENT_5_OFFSET    20
#define SEGMENT_6_OFFSET    25
#define SEGMENT_7_OFFSET    30

#define BLACK strip.Color(0, 0, 0)

// How many NeoPixels are attached to the Arduino?
#define LED_COUNT 142

// Declare our NeoPixel strip object:
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
// Argument 1 = Number of pixels in NeoPixel strip
// Argument 2 = Arduino pin number (most are valid)
// Argument 3 = Pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
//   NEO_RGBW    Pixels are wired for RGBW bitstream (NeoPixel RGBW products)

RTC_DS3231 rtc;

int firstDigit;
int secondDigit;
int thirdDigit;
int fourthDigit;

unsigned long lastTimeCheckMillis = 0;

int timeSetButtonState = HIGH;
int timeSetButtonLastReading = HIGH;
unsigned long timeSetButtonLastDebounceTime = 0;
unsigned long debounceDelay = 50;    // the debounce time; increase if the output flickers
unsigned long timeSetButtonTimePressed = 0;
unsigned long timeSetButtonTimeLastIncremented = 0;
int timeSetButtonMinutesIncremented = 0;
int timeSetButtonIncrementFastDelay = 1000;    // Time you have to press the button until in increments fast
int timeSetButtonTimeBetweenFastIncrements = 7;    // Time between fast increments

int tempHours = 0;
int tempMinutes = 0;
int tempSeconds = 0;

// setup() function -- runs once at startup --------------------------------

void setup() {
  // initialize serial communication at 9600 bits per second:
  Serial.begin(9600);
  
  delay(3000); // wait for console opening
  
  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }

  if (rtc.lostPower()) {
    Serial.println("RTC lost power, lets set the time!");
    rtc.adjust(DateTime(2020, 1, 1, 0, 0, 0));
  }

  strip.begin();           // INITIALIZE NeoPixel strip object (REQUIRED)
  strip.show();            // Turn OFF all pixels ASAP
  strip.setBrightness(255);
  
  pinMode(TIME_SET_PIN, INPUT_PULLUP);
}


// loop() function -- runs repeatedly as long as board is on ---------------

void loop() {
  unsigned long currentMillis = millis();

  // Handle button
  if (debounceTimeSetButton()) {
    // Debouncing, skip other code
    return;
  }
  
  if (timeSetButtonState == LOW) {
    // Skip normal clock cycle when button is pressed, no delay
    if (timeSetButtonMinutesIncremented > 0) {
      // We have already incremented, after a specified time. Start counting up fast
      if ((currentMillis - timeSetButtonTimePressed) > timeSetButtonIncrementFastDelay) {
        if ((currentMillis - timeSetButtonTimeLastIncremented) > timeSetButtonTimeBetweenFastIncrements) {
          addMinuteToTime();
          printTime(tempHours, tempMinutes, 0, false);
          timeSetButtonTimeLastIncremented = currentMillis;
        }
      }
    } else {
      // Increment one
      addMinuteToTime();
      printTime(tempHours, tempMinutes, 0, false);
    }
    return;
  } else {
    timeSetButtonMinutesIncremented = 0;
  }

  // Only check time every tenth of a second
  if (currentMillis - lastTimeCheckMillis > 100) {
    lastTimeCheckMillis = currentMillis;
    
    // Copy time to variables, to be able to increment later if changing with button
    DateTime now = rtc.now();
    int newHours = now.hour();
    int newMinutes = now.minute();
    int newSeconds = now.second();

    if (newHours != tempHours || newMinutes != tempMinutes || newSeconds != tempSeconds) {
      // Time has changed, update variables and leds
      tempHours = newHours;
      tempMinutes = newMinutes;
      tempSeconds = newSeconds;
      
      printTime(tempHours, tempMinutes, tempSeconds, true);
    }
  }

  delay(30);
}

void addMinuteToTime() {
  tempMinutes++;
  if (tempMinutes >= 60) {
    tempMinutes = 0;
    tempHours++;
  }
  if (tempHours >= 24) {
    tempHours = 0;
  }
  timeSetButtonMinutesIncremented++;
}

void printTime(int hours, int minutes, int seconds, bool doFade) {
  long currentColorHue = (minutes * 60 + seconds) * 65536 / 3600; // Cycles each hour
  uint32_t currentColor = strip.gamma32(strip.ColorHSV(currentColorHue));
  
  Serial.print("Time ");
  Serial.print(hours);
  Serial.print(':');
  Serial.print(minutes);
  Serial.print(':');
  Serial.print(seconds);
  Serial.print(" Hue: ");
  Serial.print(currentColorHue);
  Serial.println();
  
  int newFirstDigit = hours / 10;
  int newSecondDigit = hours % 10;
  int newThirdDigit = minutes / 10;
  int newFourthDigit = minutes % 10;
  
  // Set digits
  colonSet(COLON_INDEX, currentColor);

  bool firstDigitChanged = newFirstDigit != firstDigit;
  bool secondDigitChanged = newSecondDigit != secondDigit;
  bool thirdDigitChanged = newThirdDigit != thirdDigit;
  bool fourthDigitChanged = newFourthDigit != fourthDigit;

  // Print without transition if requested, or if digit is not changed
  if (!doFade || !firstDigitChanged) {
    digitSet(DIGIT_1_INDEX, newFirstDigit, currentColor);
  }
  if (!doFade || !secondDigitChanged) {
    digitSet(DIGIT_2_INDEX, newSecondDigit, currentColor);
  }
  if (!doFade || !thirdDigitChanged) {
    digitSet(DIGIT_3_INDEX, newThirdDigit, currentColor);
  }
  if (!doFade || !fourthDigitChanged) {
    digitSet(DIGIT_4_INDEX, newFourthDigit, currentColor);
  }

  // Print transition on all digits that have changed
  if (doFade && (firstDigitChanged || secondDigitChanged || thirdDigitChanged || fourthDigitChanged)) {
    // Animate transition from 0-100
    for (int i = 0; i <= 100; i++) {
      if (firstDigitChanged) {
        digitSetTransition(DIGIT_1_INDEX, newFirstDigit, firstDigit, currentColorHue, i);
      }
      if (secondDigitChanged) {
        digitSetTransition(DIGIT_2_INDEX, newSecondDigit, secondDigit, currentColorHue, i);
      }
      if (thirdDigitChanged) {
        digitSetTransition(DIGIT_3_INDEX, newThirdDigit, thirdDigit, currentColorHue, i);
      }
      if (fourthDigitChanged) {
        digitSetTransition(DIGIT_4_INDEX, newFourthDigit, fourthDigit, currentColorHue, i);
      }
      strip.show();
    }
  } else {
    strip.show();
  }
  
  firstDigit = newFirstDigit;
  secondDigit = newSecondDigit;
  thirdDigit = newThirdDigit;
  fourthDigit = newFourthDigit;
}

bool debounceTimeSetButton() {
  int timeSetButtonReading = digitalRead(TIME_SET_PIN);
  if (timeSetButtonReading != timeSetButtonLastReading) {
    // Start debounce timer
    timeSetButtonLastDebounceTime = millis();
  }
  
  if (timeSetButtonLastDebounceTime > 0 && (millis() - timeSetButtonLastDebounceTime) > debounceDelay) {
    timeSetButtonLastDebounceTime = 0;
    if (timeSetButtonReading != timeSetButtonState) {
      timeSetButtonState = timeSetButtonReading;
      
      Serial.print("Button changed state to: ");
      Serial.print(timeSetButtonState);
      Serial.println();

      if (timeSetButtonState == LOW) {
        // Button was pressed
        timeSetButtonTimePressed = millis();
      } else {
        // Button was released, set time to clock module
        timeSetButtonTimePressed = 0;
        rtc.adjust(DateTime(2020, 1, 1, tempHours, tempMinutes, 0));
        Serial.print("RTC set to 2020-01-01 ");
        Serial.print(tempHours);
        Serial.print(":");
        Serial.print(tempMinutes);
        Serial.print(":00");
        Serial.println();
      }
    }
  }

  timeSetButtonLastReading = timeSetButtonReading;

  if (timeSetButtonLastDebounceTime > 0) {
    // Skip normal clock cycle while debouncing
    return true;
  }
  
  return false; // Not debouncing
}

void digitSet(int index, int digit, uint32_t color) {
  bool newSegment1Enabled = isSegment1Enabled(digit);
  bool newSegment2Enabled = isSegment2Enabled(digit);
  bool newSegment3Enabled = isSegment3Enabled(digit);
  bool newSegment4Enabled = isSegment4Enabled(digit);
  bool newSegment5Enabled = isSegment5Enabled(digit);
  bool newSegment6Enabled = isSegment6Enabled(digit);
  bool newSegment7Enabled = isSegment7Enabled(digit);

  uint32_t newSegment1Color, newSegment2Color, newSegment3Color, newSegment4Color, newSegment5Color, newSegment6Color, newSegment7Color;
  
  if (newSegment1Enabled) {
    newSegment1Color = color;
  } else {
    newSegment1Color = BLACK;
  }

  if (newSegment2Enabled) {
    newSegment2Color = color;
  } else {
    newSegment2Color = BLACK;
  }

  if (newSegment3Enabled) {
    newSegment3Color = color;
  } else {
    newSegment3Color = BLACK;
  }

  if (newSegment4Enabled) {
    newSegment4Color = color;
  } else {
    newSegment4Color = BLACK;
  }

  if (newSegment5Enabled) {
    newSegment5Color = color;
  } else {
    newSegment5Color = BLACK;
  }

  if (newSegment6Enabled) {
    newSegment6Color = color;
  } else {
    newSegment6Color = BLACK;
  }

  if (newSegment7Enabled) {
    newSegment7Color = color;
  } else {
    newSegment7Color = BLACK;
  }
  
  segmentSet(index+SEGMENT_1_OFFSET, newSegment1Color);
  segmentSet(index+SEGMENT_2_OFFSET, newSegment2Color);
  segmentSet(index+SEGMENT_3_OFFSET, newSegment3Color);
  segmentSet(index+SEGMENT_4_OFFSET, newSegment4Color);
  segmentSet(index+SEGMENT_5_OFFSET, newSegment5Color);
  segmentSet(index+SEGMENT_6_OFFSET, newSegment6Color);
  segmentSet(index+SEGMENT_7_OFFSET, newSegment7Color);
}

void digitSetTransition(int index, int digit, int oldDigit, long colorHue, int transitionStep){
  bool oldSegment1Enabled = isSegment1Enabled(oldDigit);
  bool oldSegment2Enabled = isSegment2Enabled(oldDigit);
  bool oldSegment3Enabled = isSegment3Enabled(oldDigit);
  bool oldSegment4Enabled = isSegment4Enabled(oldDigit);
  bool oldSegment5Enabled = isSegment5Enabled(oldDigit);
  bool oldSegment6Enabled = isSegment6Enabled(oldDigit);
  bool oldSegment7Enabled = isSegment7Enabled(oldDigit);
  
  bool newSegment1Enabled = isSegment1Enabled(digit);
  bool newSegment2Enabled = isSegment2Enabled(digit);
  bool newSegment3Enabled = isSegment3Enabled(digit);
  bool newSegment4Enabled = isSegment4Enabled(digit);
  bool newSegment5Enabled = isSegment5Enabled(digit);
  bool newSegment6Enabled = isSegment6Enabled(digit);
  bool newSegment7Enabled = isSegment7Enabled(digit);

  uint32_t colorEnabled = strip.gamma32(strip.ColorHSV(colorHue));
  uint32_t colorTransitioningUp = strip.gamma32(strip.ColorHSV(colorHue, 255, transitionStep*255/100));
  uint32_t colorTransitioningDown = strip.gamma32(strip.ColorHSV(colorHue, 255, (100-transitionStep)*255/100));
  uint32_t newSegment1Color, newSegment2Color, newSegment3Color, newSegment4Color, newSegment5Color, newSegment6Color, newSegment7Color;

  if (newSegment1Enabled && oldSegment1Enabled) {
    // No need to transition, keep enabled
    newSegment1Color = colorEnabled;
  } else if (newSegment1Enabled) {
    // Transition from off to on
    newSegment1Color = colorTransitioningUp;
  } else if (oldSegment1Enabled) {
    // Transition from on to off
    newSegment1Color = colorTransitioningDown;
  } else {
    // No need to transition, keep off
    newSegment1Color = BLACK;
  }

  if (newSegment2Enabled && oldSegment2Enabled) {
    // No need to transition, keep enabled
    newSegment2Color = colorEnabled;
  } else if (newSegment2Enabled) {
    // Transition from off to on
    newSegment2Color = colorTransitioningUp;
  } else if (oldSegment2Enabled) {
    // Transition from on to off
    newSegment2Color = colorTransitioningDown;
  } else {
    // No need to transition, keep off
    newSegment2Color = BLACK;
  }

  if (newSegment3Enabled && oldSegment3Enabled) {
    // No need to transition, keep enabled
    newSegment3Color = colorEnabled;
  } else if (newSegment3Enabled) {
    // Transition from off to on
    newSegment3Color = colorTransitioningUp;
  } else if (oldSegment3Enabled) {
    // Transition from on to off
    newSegment3Color = colorTransitioningDown;
  } else {
    // No need to transition, keep off
    newSegment3Color = BLACK;
  }

  if (newSegment4Enabled && oldSegment4Enabled) {
    // No need to transition, keep enabled
    newSegment4Color = colorEnabled;
  } else if (newSegment4Enabled) {
    // Transition from off to on
    newSegment4Color = colorTransitioningUp;
  } else if (oldSegment4Enabled) {
    // Transition from on to off
    newSegment4Color = colorTransitioningDown;
  } else {
    // No need to transition, keep off
    newSegment4Color = BLACK;
  }

  if (newSegment5Enabled && oldSegment5Enabled) {
    // No need to transition, keep enabled
    newSegment5Color = colorEnabled;
  } else if (newSegment5Enabled) {
    // Transition from off to on
    newSegment5Color = colorTransitioningUp;
  } else if (oldSegment5Enabled) {
    // Transition from on to off
    newSegment5Color = colorTransitioningDown;
  } else {
    // No need to transition, keep off
    newSegment5Color = BLACK;
  }

  if (newSegment6Enabled && oldSegment6Enabled) {
    // No need to transition, keep enabled
    newSegment6Color = colorEnabled;
  } else if (newSegment6Enabled) {
    // Transition from off to on
    newSegment6Color = colorTransitioningUp;
  } else if (oldSegment6Enabled) {
    // Transition from on to off
    newSegment6Color = colorTransitioningDown;
  } else {
    // No need to transition, keep off
    newSegment6Color = BLACK;
  }

  if (newSegment7Enabled && oldSegment7Enabled) {
    // No need to transition, keep enabled
    newSegment7Color = colorEnabled;
  } else if (newSegment7Enabled) {
    // Transition from off to on
    newSegment7Color = colorTransitioningUp;
  } else if (oldSegment7Enabled) {
    // Transition from on to off
    newSegment7Color = colorTransitioningDown;
  } else {
    // No need to transition, keep off
    newSegment7Color = BLACK;
  }
  
  segmentSet(index+SEGMENT_1_OFFSET, newSegment1Color);
  segmentSet(index+SEGMENT_2_OFFSET, newSegment2Color);
  segmentSet(index+SEGMENT_3_OFFSET, newSegment3Color);
  segmentSet(index+SEGMENT_4_OFFSET, newSegment4Color);
  segmentSet(index+SEGMENT_5_OFFSET, newSegment5Color);
  segmentSet(index+SEGMENT_6_OFFSET, newSegment6Color);
  segmentSet(index+SEGMENT_7_OFFSET, newSegment7Color);
}

bool isSegment1Enabled(int digit) {
  switch (digit) {
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 8:
    case 9:
      return true;
    default:
      return false;
  }
}
  
bool isSegment2Enabled(int digit) {
  switch (digit) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 7:
    case 8:
    case 9:
      return true;
    default:
      return false;
  }
}
  
bool isSegment3Enabled(int digit) {
  switch (digit) {
    case 0:
    case 2:
    case 3:
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
      return true;
    default:
      return false;
  }
}
  
bool isSegment4Enabled(int digit) {
  switch (digit) {
    case 0:
    case 4:
    case 5:
    case 6:
    case 8:
    case 9:
      return true;
    default:
      return false;
  }
}
  
bool isSegment5Enabled(int digit) {
  switch (digit) {
    case 0:
    case 2:
    case 6:
    case 8:
      return true;
    default:
      return false;
  }
}
  
bool isSegment6Enabled(int digit) {
  switch (digit) {
    case 0:
    case 2:
    case 3:
    case 5:
    case 6:
    case 8:
    case 9:
      return true;
    default:
      return false;
  }
}
  
bool isSegment7Enabled(int digit) {
  switch (digit) {
    case 0:
    case 1:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
      return true;
    default:
      return false;
  }
}

void segmentSet(int ledIndex, uint32_t color){
  for(int i=ledIndex; i<ledIndex+5; i++){
    strip.setPixelColor(i, color);
  }  
}

void colonSet(int ledIndex, uint32_t color){
  for(int i=ledIndex; i<ledIndex+2; i++){
    strip.setPixelColor(i, color);
  }  
}
