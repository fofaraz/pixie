#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

/* Uncomment the initialize the I2C address , uncomment only one, If you get a totally blank screen try the other*/
// #define i2c_Address 0x78   //initialize with the I2C addr 0x3C Typically eBay OLED's
#define i2c_Address 0x3c  //initialize with the I2C addr 0x3C Typically eBay OLED's
//#define i2c_Address 0x3d //initialize with the I2C addr 0x3D Typically Adafruit OLED's

#define SCREEN_WIDTH 128  // OLED display width, in pixels
#define SCREEN_HEIGHT 64  // OLED display height, in pixels
#define OLED_RESET -1     //   QT-PY / XIAO
Adafruit_SH1106G display = Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Emotion states
enum EmotionState {
  IDLE,
  HAPPY,
  SAD,
  SCARED
};

// Global variables for facial features
EmotionState state = IDLE;
int eyeSize = 24;  // Increased from 12 to 24 (4x bigger)
int eyeDistance = 45;  // Increased to accommodate bigger eyes
int eyebrewLength = 10;  // Increased to match bigger eyes
int eyebrewOffset = 20;  // Reduced to move eyebrews down
int eyeOffset = 0;  // Reduced to move eyes down
int mouthSize = 30;  // Increased mouth size
int mouthOffset = 20;  // Reduced to move mouth down
int faceCenterX = SCREEN_WIDTH / 2;
int faceCenterY = SCREEN_HEIGHT / 2;

// Pupil movement variables
int pupilOffsetX = 0;
int pupilOffsetY = 0;
unsigned long lastPupilMove = 0;
const unsigned long PUPIL_MOVE_INTERVAL = 2000;
const int MAX_PUPIL_OFFSET = 4;  // Increased for bigger eyes

// State transition timing
unsigned long lastStateChange = 0;
const unsigned long STATE_DURATION = 10000;

// Function declarations
void drawEyebrews(EmotionState state);
void drawEyes(EmotionState state);
void drawMouth(EmotionState state);
void drawIdle();
void drawHappy();
void drawSad();
void drawScared();
void updateState();
void updatePupilPosition();

void setup() {
  Serial.begin(9600);
  delay(250);                        // wait for the OLED to power up
  display.begin(i2c_Address, true);  // Address 0x3C default
  display.clearDisplay();
  display.display();
  lastStateChange = millis();
  lastPupilMove = millis();
}

void loop() {
  updateState();
  updatePupilPosition();
  
  display.clearDisplay();
  
  switch(state) {
    case IDLE:
      drawIdle();
      break;
    case HAPPY:
      drawHappy();
      break;
    case SAD:
      drawSad();
      break;
    case SCARED:
      drawScared();
      break;
  }
  
  display.display();
  delay(100);
}

void updateState() {
  unsigned long currentTime = millis();
  
  // Check if it's time to change state
  if (currentTime - lastStateChange >= STATE_DURATION) {
    // Move to next state
    switch(state) {
      case IDLE:
        state = HAPPY;
        break;
      case HAPPY:
        state = SAD;
        break;
      case SAD:
        state = SCARED;
        break;
      case SCARED:
        state = IDLE;
        break;
    }
    lastStateChange = currentTime;
  }
}

void updatePupilPosition() {
  unsigned long currentTime = millis();
  
  if (currentTime - lastPupilMove >= PUPIL_MOVE_INTERVAL) {
    // Random movement within bounds
    pupilOffsetX = random(-MAX_PUPIL_OFFSET, MAX_PUPIL_OFFSET + 1);
    pupilOffsetY = random(-MAX_PUPIL_OFFSET, MAX_PUPIL_OFFSET + 1);
    lastPupilMove = currentTime;
  }
}

void drawEyebrews(EmotionState state) {
  int leftX = faceCenterX - eyeDistance/2 - eyebrewLength/2;
  int rightX = faceCenterX + eyeDistance/2 - eyebrewLength/2;
  int y = faceCenterY - eyebrewOffset;
  
  switch(state) {
    case IDLE:
      // Straight eyebrows
      display.drawLine(leftX, y, leftX + eyebrewLength, y, SH110X_WHITE);
      display.drawLine(rightX, y, rightX + eyebrewLength, y, SH110X_WHITE);
      break;
    case HAPPY:
      // Raised eyebrows
      display.drawLine(leftX, y, leftX + eyebrewLength, y-2, SH110X_WHITE);
      display.drawLine(rightX, y, rightX + eyebrewLength, y-2, SH110X_WHITE);
      break;
    case SAD:
      // Lowered eyebrows
      display.drawLine(leftX, y, leftX + eyebrewLength, y+2, SH110X_WHITE);
      display.drawLine(rightX, y, rightX + eyebrewLength, y+2, SH110X_WHITE);
      break;
    case SCARED:
      // Raised and angled eyebrows
      display.drawLine(leftX, y, leftX + eyebrewLength, y-4, SH110X_WHITE);
      display.drawLine(rightX, y, rightX + eyebrewLength, y-4, SH110X_WHITE);
      break;
  }
}

void drawEyes(EmotionState state) {
  int leftX = faceCenterX - eyeDistance/2 - eyeSize/2;
  int rightX = faceCenterX + eyeDistance/2 - eyeSize/2;
  int y = faceCenterY - eyeOffset;
  
  switch(state) {
    case IDLE:
      // Normal eyes with pupils
      display.fillCircle(leftX + eyeSize/2, y, eyeSize/2, SH110X_WHITE);
      display.fillCircle(rightX + eyeSize/2, y, eyeSize/2, SH110X_WHITE);
      // Add pupils
      display.fillCircle(leftX + eyeSize/2 + pupilOffsetX, y + pupilOffsetY, eyeSize/6, SH110X_BLACK);
      display.fillCircle(rightX + eyeSize/2 + pupilOffsetX, y + pupilOffsetY, eyeSize/6, SH110X_BLACK);
      break;
    case HAPPY:
      // Happy eyes (slightly squinted)
      display.fillCircle(leftX + eyeSize/2, y, eyeSize/2, SH110X_WHITE);
      display.fillCircle(rightX + eyeSize/2, y, eyeSize/2, SH110X_WHITE);
      // Add pupils
      display.fillCircle(leftX + eyeSize/2 + pupilOffsetX, y + pupilOffsetY, eyeSize/6, SH110X_BLACK);
      display.fillCircle(rightX + eyeSize/2 + pupilOffsetX, y + pupilOffsetY, eyeSize/6, SH110X_BLACK);
      break;
    case SAD:
      // Sad eyes (half-closed)
      display.fillCircle(leftX + eyeSize/2, y, eyeSize/2, SH110X_WHITE);
      display.fillCircle(rightX + eyeSize/2, y, eyeSize/2, SH110X_WHITE);
      // Add pupils
      display.fillCircle(leftX + eyeSize/2 + pupilOffsetX, y + pupilOffsetY, eyeSize/6, SH110X_BLACK);
      display.fillCircle(rightX + eyeSize/2 + pupilOffsetX, y + pupilOffsetY, eyeSize/6, SH110X_BLACK);
      break;
    case SCARED:
      // Wide open eyes
      display.fillCircle(leftX + eyeSize/2, y, eyeSize/2, SH110X_WHITE);
      display.fillCircle(rightX + eyeSize/2, y, eyeSize/2, SH110X_WHITE);
      // Add pupils
      display.fillCircle(leftX + eyeSize/2 + pupilOffsetX, y + pupilOffsetY, eyeSize/6, SH110X_BLACK);
      display.fillCircle(rightX + eyeSize/2 + pupilOffsetX, y + pupilOffsetY, eyeSize/6, SH110X_BLACK);
      break;
  }
}

void drawMouth(EmotionState state) {
  int x = faceCenterX - mouthSize/2;
  int y = faceCenterY + mouthOffset;
  
  switch(state) {
    case IDLE:
      // Straight line
      display.drawLine(x, y, x + mouthSize, y, SH110X_WHITE);
      break;
    case HAPPY:
      // Smile using multiple points
      for(int i = 0; i < mouthSize; i++) {
        int yOffset = -3 * sin((float)i / mouthSize * PI);  // Increased curve
        display.drawPixel(x + i, y + yOffset, SH110X_WHITE);
      }
      break;
    case SAD:
      // Frown using multiple points
      for(int i = 0; i < mouthSize; i++) {
        int yOffset = 3 * sin((float)i / mouthSize * PI);  // Increased curve
        display.drawPixel(x + i, y + yOffset, SH110X_WHITE);
      }
      break;
    case SCARED:
      // Open mouth
      display.drawCircle(x + mouthSize/2, y, mouthSize/3, SH110X_WHITE);
      break;
  }
}

void drawIdle() {
  drawEyebrews(IDLE);
  drawEyes(IDLE);
  drawMouth(IDLE);
}

void drawHappy() {
  drawEyebrews(HAPPY);
  drawEyes(HAPPY);
  drawMouth(HAPPY);
}

void drawSad() {
  drawEyebrews(SAD);
  drawEyes(SAD);
  drawMouth(SAD);
}

void drawScared() {
  drawEyebrews(SCARED);
  drawEyes(SCARED);
  drawMouth(SCARED);
}
