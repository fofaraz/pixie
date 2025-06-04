#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

/* Uncomment the initialize the I2C address , uncomment only one, If you get a totally blank screen try the other*/
// #define i2c_Address 0x78   //initialize with the I2C addr 0x3C Typically eBay OLED's
#define i2c_Address 0x3c // initialize with the I2C addr 0x3C Typically eBay OLED's
// #define i2c_Address 0x3d //initialize with the I2C addr 0x3D Typically Adafruit OLED's

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET -1    //   QT-PY / XIAO
Adafruit_SH1106G display = Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Emotion states
enum EmotionState
{
  IDLE,
  HAPPY,
  SAD,
  SCARED,
  WINK,
  DEAD,
  NOISE
};

// Global variables for facial features
EmotionState state = HAPPY;
int eyeSize = 35;
int eyeDistance = 70;   // Increased to accommodate bigger eyes
int eyebrewLength = 20; // Increased to match bigger eyes
int eyebrewOffset = 20; // Reduced to move eyebrews down
int eyeOffset = 0;      // Reduced to move eyes down
int mouthSize = 20;     // Increased mouth size
int mouthOffset = 20;   // Reduced to move mouth down
int faceCenterX = SCREEN_WIDTH / 2;
int faceCenterY = SCREEN_HEIGHT / 2;

// Pupil movement variables
int pupilOffsetX = 0;
int pupilOffsetY = 0;
int targetPupilOffsetX = 0;
int targetPupilOffsetY = 0;
unsigned long lastPupilMove = 0;
const unsigned long PUPIL_MOVE_INTERVAL = 2000;
const int MAX_PUPIL_OFFSET = 4; // Increased for bigger eyes
const int PUPIL_MOVE_STEP = 1;  // How many pixels to move per update

// Blinking variables
bool isBlinking = false;
unsigned long lastBlinkTime = 0;
const unsigned long BLINK_DURATION = 150;           // How long each blink lasts
const unsigned long MIN_TIME_BETWEEN_BLINKS = 2000; // Minimum time between blinks
const unsigned long MAX_TIME_BETWEEN_BLINKS = 5000; // Maximum time between blinks
unsigned long nextBlinkTime = 0;

// Dead eye animation variables
bool deadEyeOpen = false;
unsigned long lastDeadEyeChange = 0;
const unsigned long DEAD_EYE_OPEN_DURATION = 1000;  // How long the eye stays open
const unsigned long MIN_TIME_BETWEEN_DEAD_EYE = 5000; // Minimum time between eye openings
const unsigned long MAX_TIME_BETWEEN_DEAD_EYE = 15000; // Maximum time between eye openings
unsigned long nextDeadEyeTime = 0;

// State transition timing
unsigned long lastStateChange = 0;
const unsigned long STATE_DURATION = 10000;
const unsigned long WINK_DURATION = 500;  // 0.5 seconds for wink state

// Function declarations
void drawEyebrews(EmotionState state);
void drawEyes(EmotionState state);
void drawMouth(EmotionState state);
void drawIdle();
void drawHappy();
void drawSad();
void drawScared();
void drawWink();
void drawDead();
void drawNoise();
void updateState();
void updatePupilPosition();
void updateBlink();
void updateDeadEye();

void setup()
{
  Serial.begin(9600);
  delay(250);                       // wait for the OLED to power up
  display.begin(i2c_Address, true); // Address 0x3C default
  display.clearDisplay();
  display.display();
  lastStateChange = millis();
  lastPupilMove = millis();
}

void loop()
{
  updateState();
  updatePupilPosition();
  updateBlink();
  updateDeadEye();

  display.clearDisplay();

  switch (state)
  {
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
  case WINK:
    drawWink();
    break;
  case DEAD:
    drawDead();
    break;
  case NOISE:
    drawNoise();
    break;
  }

  display.display();
  
  // Use shorter delay for NOISE state to make animation smoother
  if (state == NOISE) {
    delay(20);  // 20ms delay for noise (50fps)
  } else {
    delay(100); // 100ms delay for other states (10fps)
  }
}

void updateState()
{
  unsigned long currentTime = millis();
  unsigned long requiredDuration = (state == WINK) ? WINK_DURATION : STATE_DURATION;

  // Check if it's time to change state
  if (currentTime - lastStateChange >= requiredDuration)
  {
    // Move to next state
    switch (state)
    {
    case IDLE:
      state = WINK; // Go to blink after idle
      break;
    case WINK:
      state = HAPPY; // Go to happy after blink
      break;
    case HAPPY:
      state = SAD;
      break;
    case SAD:
      state = SCARED;
      break;
    case SCARED:
      state = DEAD;
      deadEyeOpen = false;  // Ensure both eyes start as X's
      nextDeadEyeTime = currentTime + random(MIN_TIME_BETWEEN_DEAD_EYE, MAX_TIME_BETWEEN_DEAD_EYE);
      break;
    case DEAD:
      state = NOISE;
      break;
    case NOISE:
      state = IDLE;
      break;
    }
    lastStateChange = currentTime;
  }
}

void updatePupilPosition()
{
  unsigned long currentTime = millis();

  // Set new target position when it's time
  if (currentTime - lastPupilMove >= PUPIL_MOVE_INTERVAL)
  {
    targetPupilOffsetX = random(-MAX_PUPIL_OFFSET, MAX_PUPIL_OFFSET + 1);
    targetPupilOffsetY = random(-MAX_PUPIL_OFFSET, MAX_PUPIL_OFFSET + 1);
    lastPupilMove = currentTime;
  }

  // Gradually move current position towards target
  if (pupilOffsetX < targetPupilOffsetX) {
    pupilOffsetX += PUPIL_MOVE_STEP;
  } else if (pupilOffsetX > targetPupilOffsetX) {
    pupilOffsetX -= PUPIL_MOVE_STEP;
  }

  if (pupilOffsetY < targetPupilOffsetY) {
    pupilOffsetY += PUPIL_MOVE_STEP;
  } else if (pupilOffsetY > targetPupilOffsetY) {
    pupilOffsetY -= PUPIL_MOVE_STEP;
  }
}

void updateBlink()
{
  // Don't blink in DEAD state
  if (state == DEAD) {
    isBlinking = false;
    return;
  }

  unsigned long currentTime = millis();

  if (isBlinking)
  {
    // Check if blink duration has passed
    if (currentTime - lastBlinkTime >= BLINK_DURATION)
    {
      isBlinking = false;
      nextBlinkTime = currentTime + random(MIN_TIME_BETWEEN_BLINKS, MAX_TIME_BETWEEN_BLINKS);
    }
  }
  else
  {
    // Check if it's time for next blink
    if (currentTime >= nextBlinkTime)
    {
      isBlinking = true;
      lastBlinkTime = currentTime;
    }
  }
}

void updateDeadEye()
{
  if (state != DEAD) {
    deadEyeOpen = false;
    return;
  }

  unsigned long currentTime = millis();

  if (deadEyeOpen) {
    // Check if eye should close
    if (currentTime - lastDeadEyeChange >= DEAD_EYE_OPEN_DURATION) {
      deadEyeOpen = false;
      nextDeadEyeTime = currentTime + random(MIN_TIME_BETWEEN_DEAD_EYE, MAX_TIME_BETWEEN_DEAD_EYE);
    }
  } else {
    // Check if eye should open
    if (currentTime >= nextDeadEyeTime) {
      deadEyeOpen = true;
      lastDeadEyeChange = currentTime;
    }
  }
}

void drawEyebrews(EmotionState state)
{
  int leftX = faceCenterX - eyeDistance / 2 - eyebrewLength / 2;
  int rightX = faceCenterX + eyeDistance / 2 - eyebrewLength / 2;
  int y = faceCenterY - eyebrewOffset;

  int sadEyebrewYOffset = -5;
  int sadEyebrewLength = eyebrewLength * .7;

  switch (state)
  {
  case IDLE:
    for (int i = 0; i < eyebrewLength; i++)
    {
      int yOffset = 4 * sin((float)i / eyebrewLength * PI);
      display.drawPixel(leftX + i, y - yOffset, SH110X_WHITE);
      display.drawPixel(rightX + i, y - yOffset, SH110X_WHITE);
    }
    break;
  case HAPPY:
    // Use same eyebrows as idle
    for (int i = 0; i < eyebrewLength; i++)
    {
      int yOffset = 4 * sin((float)i / eyebrewLength * PI);
      display.drawPixel(leftX + i, y - yOffset, SH110X_WHITE);
      display.drawPixel(rightX + i, y - yOffset, SH110X_WHITE);
    }
    break;
  case SAD:
    display.drawLine(leftX+(eyebrewLength-sadEyebrewLength), y + sadEyebrewYOffset+5, leftX+(eyebrewLength-sadEyebrewLength) + sadEyebrewLength, y + sadEyebrewYOffset, SH110X_WHITE);
    display.drawLine(rightX, y + sadEyebrewYOffset, rightX + sadEyebrewLength, y + sadEyebrewYOffset+5, SH110X_WHITE);
    break;
  case SCARED:
    // Raised and angled eyebrows
    display.drawLine(leftX, y, leftX + eyebrewLength, y - 4, SH110X_WHITE);
    display.drawLine(rightX, y, rightX + eyebrewLength, y - 4, SH110X_WHITE);
    break;
  }
}

void drawEyes(EmotionState state)
{
  int leftX = faceCenterX - eyeDistance / 2 - eyeSize / 2;
  int rightX = faceCenterX + eyeDistance / 2 - eyeSize / 2;
  int y = faceCenterY - eyeOffset;

  // Handle blinking for all states except WINK and DEAD
  if (isBlinking && state != WINK && state != DEAD)
  {
    // Draw both eyes as arcs when blinking
    for (int i = 0; i < eyeSize; i++)
    {
      int yOffset = 2 * sin((float)i / eyeSize * PI);
      display.drawPixel(leftX + i, y - yOffset, SH110X_WHITE);
      display.drawPixel(rightX + i, y - yOffset, SH110X_WHITE);
    }
    return;
  }

  switch (state)
  {
  case WINK:
    // Left eye as white arc
    for (int i = 0; i < eyeSize; i++)
    {
      int yOffset = 2 * sin((float)i / eyeSize * PI);
      display.drawPixel(leftX + i, y - yOffset, SH110X_WHITE);
    }
    // Right eye normal
    display.fillCircle(rightX + eyeSize / 2, y, eyeSize / 2, SH110X_WHITE);
    display.fillCircle(rightX + eyeSize / 2 + pupilOffsetX, y + pupilOffsetY, eyeSize / 6, SH110X_BLACK);
    break;
  case DEAD:
    // Left eye either X or normal based on deadEyeOpen flag
    if (deadEyeOpen) {
      // Normal eye with pupil
      display.fillCircle(leftX + eyeSize / 2, y, eyeSize / 2, SH110X_WHITE);
      display.fillCircle(leftX + eyeSize / 2 + pupilOffsetX, y + pupilOffsetY, eyeSize / 6, SH110X_BLACK);
    } else {
      // X-shaped eye
      display.drawLine(leftX + eyeSize/4, y - eyeSize/4, leftX + 3*eyeSize/4, y + eyeSize/4, SH110X_WHITE);
      display.drawLine(leftX + eyeSize/4, y + eyeSize/4, leftX + 3*eyeSize/4, y - eyeSize/4, SH110X_WHITE);
    }
    // Right eye always X
    display.drawLine(rightX + eyeSize/4, y - eyeSize/4, rightX + 3*eyeSize/4, y + eyeSize/4, SH110X_WHITE);
    display.drawLine(rightX + eyeSize/4, y + eyeSize/4, rightX + 3*eyeSize/4, y - eyeSize/4, SH110X_WHITE);
    break;
  default:
    // Normal eyes with pupils
    display.fillCircle(leftX + eyeSize / 2, y, eyeSize / 2, SH110X_WHITE);
    display.fillCircle(rightX + eyeSize / 2, y, eyeSize / 2, SH110X_WHITE);
    // Add pupils
    display.fillCircle(leftX + eyeSize / 2 + pupilOffsetX, y + pupilOffsetY, eyeSize / 6, SH110X_BLACK);
    display.fillCircle(rightX + eyeSize / 2 + pupilOffsetX, y + pupilOffsetY, eyeSize / 6, SH110X_BLACK);
    break;
  }
}

void drawMouth(EmotionState state)
{
  int x = faceCenterX - mouthSize / 2;
  int y = faceCenterY + mouthOffset;

  switch (state)
  {
  case IDLE:
    // display.drawLine(x, y, x + mouthSize, y, SH110X_WHITE);
    for (int i = 0; i < mouthSize; i++)
    {
      int yOffset = 5 * sin((float)i / mouthSize * PI); // Increased curve
      display.drawPixel(x + i, y + yOffset, SH110X_WHITE);
    }
    break;
  case HAPPY:
    // Draw downward D shape for happy mouth
    display.drawLine(x, y , x + mouthSize, y, SH110X_WHITE);  //  horizontal line
    for(int i = 0; i < mouthSize; i++) {
      int yOffset = mouthSize/2 * sin((float)i / (mouthSize) * PI);
      display.drawPixel(x + i, y + yOffset, SH110X_WHITE);  // Down curved line
    }
    break;
  case SAD:
    // Draw V shape for sad mouth
    display.drawLine(x, y, x + mouthSize / 2, y - mouthSize / 2, SH110X_WHITE);
    display.drawLine(x + mouthSize / 2, y - mouthSize / 2, x + mouthSize, y, SH110X_WHITE);
    break;
  case SCARED:
    // Open mouth
    display.drawCircle(x + mouthSize / 2, y, mouthSize / 3, SH110X_WHITE);
    break;
  }
}

void drawIdle()
{
  drawEyebrews(IDLE);
  drawEyes(IDLE);
  drawMouth(IDLE);
}

void drawHappy()
{
  drawEyebrews(HAPPY);
  drawEyes(HAPPY);
  drawMouth(HAPPY);
}

void drawSad()
{
  drawEyebrews(SAD);
  drawEyes(SAD);
  drawMouth(SAD);
}

void drawScared()
{
  drawEyebrews(SCARED);
  drawEyes(SCARED);
  drawMouth(SCARED);
}

void drawWink()
{
  drawEyebrews(IDLE); // Use same eyebrows as idle
  drawEyes(WINK);
  drawMouth(IDLE); // Use same mouth as idle
}

void drawDead()
{
  // No eyebrows for dead state
  drawEyes(DEAD);
  drawMouth(SAD); // Use same mouth as sad state
}

void drawNoise()
{
  // Generate random noise pattern
  for(int y = 0; y < SCREEN_HEIGHT; y++) {
    for(int x = 0; x < SCREEN_WIDTH; x++) {
      // Create a more TV-like noise pattern by grouping pixels
      if(random(100) < 50) {  // 50% chance of pixel being on
        // Draw a small cluster of pixels for more realistic TV noise
        for(int dy = 0; dy < 2; dy++) {
          for(int dx = 0; dx < 2; dx++) {
            if(x + dx < SCREEN_WIDTH && y + dy < SCREEN_HEIGHT) {
              display.drawPixel(x + dx, y + dy, SH110X_WHITE);
            }
          }
        }
      }
    }
  }
}
