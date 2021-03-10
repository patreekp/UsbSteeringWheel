#include <Keypad.h>
#include <Joystick.h>

#include <Adafruit_NeoPixel.h>

// ----- LEDS ---------
// Uncomment to enable leds
#define INCLUDE_WS2812B 
// How many leds
#define WS2812B_RGBLEDCOUNT 18
// Data pin
#define WS2812B_DATAPIN A3
// 0 left to right, 1 right to left
#define WS2812B_RIGHTTOLEFT 0

#define ENABLE_PULLUPS
#define NUMROTARIES 3
#define NUMBUTTONS 22
#define NUMROWS 4
#define NUMCOLS 4

byte buttons[NUMROWS][NUMCOLS] = {
  {0, 1, 2, 3},
  {4, 5, 6, 7},
  {8, 9, 10, 11},
  {12, 13, 14, 15},
};

struct rotariesdef {
  byte pin1;
  byte pin2;
  int ccwchar;
  int cwchar;
  volatile unsigned char state;
};

rotariesdef rotaries[NUMROTARIES] {
  {10, 11, 26, 27, 0},
  {12, 13, 28, 29, 0},
  {A0, A1, 30, 31, 0},

};

#define DIR_CCW 0x10
#define DIR_CW 0x20
#define R_START 0x0

#ifdef HALF_STEP
#define R_CCW_BEGIN 0x1
#define R_CW_BEGIN 0x2
#define R_START_M 0x3
#define R_CW_BEGIN_M 0x4
#define R_CCW_BEGIN_M 0x5
const unsigned char ttable[6][4] = {
  // R_START (00)
  {R_START_M,            R_CW_BEGIN,     R_CCW_BEGIN,  R_START},
  // R_CCW_BEGIN
  {R_START_M | DIR_CCW, R_START,        R_CCW_BEGIN,  R_START},
  // R_CW_BEGIN
  {R_START_M | DIR_CW,  R_CW_BEGIN,     R_START,      R_START},
  // R_START_M (11)
  {R_START_M,            R_CCW_BEGIN_M,  R_CW_BEGIN_M, R_START},
  // R_CW_BEGIN_M
  {R_START_M,            R_START_M,      R_CW_BEGIN_M, R_START | DIR_CW},
  // R_CCW_BEGIN_M
  {R_START_M,            R_CCW_BEGIN_M,  R_START_M,    R_START | DIR_CCW},
};
#else
#define R_CW_FINAL 0x1
#define R_CW_BEGIN 0x2
#define R_CW_NEXT 0x3
#define R_CCW_BEGIN 0x4
#define R_CCW_FINAL 0x5
#define R_CCW_NEXT 0x6

const unsigned char ttable[7][4] = {
  // R_START
  {R_START,    R_CW_BEGIN,  R_CCW_BEGIN, R_START},
  // R_CW_FINAL
  {R_CW_NEXT,  R_START,     R_CW_FINAL,  R_START | DIR_CW},
  // R_CW_BEGIN
  {R_CW_NEXT,  R_CW_BEGIN,  R_START,     R_START},
  // R_CW_NEXT
  {R_CW_NEXT,  R_CW_BEGIN,  R_CW_FINAL,  R_START},
  // R_CCW_BEGIN
  {R_CCW_NEXT, R_START,     R_CCW_BEGIN, R_START},
  // R_CCW_FINAL
  {R_CCW_NEXT, R_CCW_FINAL, R_START,     R_START | DIR_CCW},
  // R_CCW_NEXT
  {R_CCW_NEXT, R_CCW_FINAL, R_CCW_BEGIN, R_START},
};
#endif

void WriteToComputer() {
  while (Serial1.available()) {
    char c = (char)Serial1.read();
    Serial.write(c);
  }
}

#ifdef INCLUDE_WS2812B

Adafruit_NeoPixel WS2812B_strip = Adafruit_NeoPixel(WS2812B_RGBLEDCOUNT, WS2812B_DATAPIN, NEO_GRB + NEO_KHZ800);
bool LedsDisabled = false;
byte r, g, b;
void ReadLeds() {
  while (!Serial.available()) {}
  WS2812B_strip.setBrightness(Serial.read());

  for (int i = 0; i < WS2812B_RGBLEDCOUNT; i++) {
    while (!Serial.available()) {}
    r = Serial.read();
    while (!Serial.available()) {}
    g = Serial.read();
    while (!Serial.available()) {}
    b = Serial.read();
    if (WS2812B_RIGHTTOLEFT > 0)
      WS2812B_strip.setPixelColor(WS2812B_RGBLEDCOUNT - 1 - i, r, g, b);
    else {
      WS2812B_strip.setPixelColor(i, r, g, b);
    }
    WriteToComputer();
  }
  WS2812B_strip.show();

  for (int i = 0; i < 3; i++) {
    while (!Serial.available()) {}
    Serial.read();
  }
}

void DisableLeds() {
  LedsDisabled = true;
  Serial.write("Leds disabled");
}

#endif

byte rowPins[NUMROWS] = {2, 3, 4, 5};
byte colPins[NUMCOLS] = {6, 7, 8, 9};

Keypad buttbx = Keypad( makeKeymap(buttons), rowPins, colPins, NUMROWS, NUMCOLS);

Joystick_ Joystick;

static long baud = 9600;
static long newBaud = baud;

void lineCodingEvent(long baud, byte databits, byte parity, byte charFormat)
{
  newBaud = baud;
}

void setup() {
  Joystick.begin();
  rotary_init();

  pinMode(LED_BUILTIN, OUTPUT);

  Serial.begin(baud);
  Serial1.begin(baud);

#ifdef INCLUDE_WS2812B
  WS2812B_strip.begin();
  WS2812B_strip.setPixelColor(0, 0, 0, 0);
  WS2812B_strip.show();
#endif
}

int readSize = 0;
int endofOutcomingMessageCount = 0;
int messageend = 0;
String command = "";

void UpdateBaudRate() {
  // Update baudrate if required
  newBaud = Serial.baud();
  if (newBaud != baud) {
    baud = newBaud;
    Serial1.end();
    Serial1.begin(baud);
  }
}

void loop() {

  //RotaryS();

  Clutch();
  
  CheckAllButtons();

  CheckAllEncoders();

UpdateBaudRate();

  while (Serial.available()) {
    WriteToComputer();
#ifdef INCLUDE_WS2812B
    char c = (char)Serial.read();
    if (!LedsDisabled) {

      if (messageend < 6) {
        if (c == (char)0xFF) {
          messageend++;
        }
        else {
          messageend = 0;
        }
      }

      if (messageend >= 3 && c != (char)(0xff)) {
        command += c;
        while (command.length() < 5) {
          WriteToComputer();

          while (!Serial.available()) {}
          c = (char)Serial.read();
          command += c;
        }

        if (command == "sleds") {
          ReadLeds();
        }
        if (command == "dleds") {
          DisableLeds();
        }
        else {
          Serial1.print(command);
        }
        command = "";
        messageend = 0;
      }
      else {
        Serial1.write(c);
      }
    }
    else {
      Serial1.write(c);
    }
#else
    char c = (char)Serial.read();
    Serial1.write(c);
#endif

    }

  WriteToComputer();
}
