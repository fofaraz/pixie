#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <VL53L0X.h>
// Adafruit_GFX version 1.12.0
// Adafruit_SH110X version 2.1.12

/* Uncomment the initialize the I2C address , uncomment only one, If you get a totally blank screen try the other*/
// #define i2c_Address 0x78   //initialize with the I2C addr 0x3C Typically eBay OLED's
#define i2c_Address 0x3c // initialize with the I2C addr 0x3C Typically eBay OLED's
// #define i2c_Address 0x3d //initialize with the I2C addr 0x3D Typically Adafruit OLED's

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET -1    //   QT-PY / XIAO
Adafruit_SH1106G display = Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// VL53L0X sensor pins
#define VL53L0X_SDA 18
#define VL53L0X_SCL 19
#define LIGHT_SENSOR_PIN 36    // Changed to GPIO36 (ADC1_CH0)
#define LAUGHING_THRESHOLD 200 // Distance in mm to trigger laughing state
#define SCARED_THRESHOLD 100   // Distance in mm to trigger scared state

// Brightness control
#define MIN_CONTRAST 0   // Minimum contrast value (0-255)
#define MAX_CONTRAST 255 // Maximum contrast value (0-255)
#define MIN_LIGHT 0      // Minimum light sensor value
#define MAX_LIGHT 4095   // Maximum light sensor value (12-bit ADC)

// Sensor variables
VL53L0X sensor;
TwoWire I2Ctwo = TwoWire(1); // Use I2C bus 1
unsigned long lastSensorRead = 0;
const unsigned long SENSOR_READ_INTERVAL = 100; // Read sensor every ... second
unsigned long lastLightRead = 0;                // For light sensor reading
const unsigned long LIGHT_READ_INTERVAL = 1000; // Read light sensor every second
bool isInteracting = false;

// Emotion states
enum EmotionState
{
  IDLE,
  HAPPY,
  SAD,
  SCARED,
  WINK,
  DEAD,
  NOISE,
  LAUGHING,
  ANGRY,
  SLEEP
};

// Global variables for facial features
EmotionState state = SLEEP;
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

// Scared state pupil shaking variables
const unsigned long SCARED_SHAKE_INTERVAL = 50; // Shake every 50ms
const int SCARED_SHAKE_AMOUNT = 4;              // How far to shake left/right
unsigned long lastScaredShake = 0;
bool shakeDirection = true; // true = right, false = left

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
const unsigned long DEAD_EYE_OPEN_DURATION = 1000;     // How long the eye stays open
const unsigned long MIN_TIME_BETWEEN_DEAD_EYE = 5000;  // Minimum time between eye openings
const unsigned long MAX_TIME_BETWEEN_DEAD_EYE = 15000; // Maximum time between eye openings
unsigned long nextDeadEyeTime = 0;

// Laughing animation variables
unsigned long lastJiggleTime = 0;
const unsigned long JIGGLE_INTERVAL = 10; // Update frequency during jiggle (100fps)
float eyeJiggleOffset = 0;
float mouthJiggleOffset = 0;
const float MAX_JIGGLE = 2.0;        // Maximum jiggle amount in pixels
const float EYE_JIGGLE_SPEED = .6;   //  speed for eyes
const float MOUTH_JIGGLE_SPEED = .6; //  speed for mouth
const float PHASE_OFFSET = PI / 4;   // 45 degrees phase offset between eyes and mouth

// Jiggle timing
unsigned long lastJiggleStateChange = 0;
const unsigned long JIGGLE_DURATION = 1000; // 0.5 second of jiggling
const unsigned long STATIC_DURATION = 1000; // 0.5 second of static pose
bool isJiggling = false;

// State transition timing
unsigned long lastStateChange = 0;
const unsigned long STATE_DURATION = 10000;
const unsigned long WINK_DURATION = 500; // 0.5 seconds for wink state

// Sleep mouth animation variables
const float MIN_SLEEP_MOUTH = 3.0;   // Minimum circle radius
const float MAX_SLEEP_MOUTH = 8.0;   // Maximum circle radius
const float SLEEP_MOUTH_SPEED = 0.1; // Speed of size change
bool sleepMouthGrowing = true;
float sleepMouthSize = MIN_SLEEP_MOUTH;

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
void drawLaughing();
void drawAngry();
void drawSleep();
void updateState();
void updatePupilPosition();
void updateBlink();
void updateDeadEye();
void updateJiggle();
void setState(EmotionState newState);

// Function to map light sensor value to contrast
uint8_t mapLightToContrast(int lightValue)
{
  // Invert the mapping - darker environment = higher contrast
  return map(lightValue, MIN_LIGHT, MAX_LIGHT, MAX_CONTRAST, MIN_CONTRAST);
}

void setup()
{
  Serial.begin(9600);
  Serial.println("Starting up...");
  delay(250);                       // wait for the OLED to power up
  display.begin(i2c_Address, true); // Address 0x3C default
  display.clearDisplay();
  display.display();
  lastStateChange = millis();
  lastPupilMove = millis();

  // Initialize light sensor pin and ADC
  pinMode(LIGHT_SENSOR_PIN, INPUT);
  analogSetAttenuation(ADC_11db); // Set ADC attenuation for full 0-3.3V range

  // Initialize VL53L0X sensor
  I2Ctwo.begin(VL53L0X_SDA, VL53L0X_SCL, 100000); // Initialize with 100kHz
  delay(100);                                     // Give some time for I2C to stabilize

  sensor.setBus(&I2Ctwo);
  sensor.setTimeout(500);

  if (!sensor.init())
  {
    Serial.println("Failed to detect and initialize VL53L0X sensor!");
    while (1)
    {
      delay(1000);
    }
  }

  // Start continuous measurement
  sensor.startContinuous();
  Serial.println("VL53L0X sensor initialized successfully!");
}

void loop()
{
  // Read light sensor every second
  unsigned long currentTime = millis();
  if (currentTime - lastLightRead >= LIGHT_READ_INTERVAL)
  {
    int lightValue = analogRead(LIGHT_SENSOR_PIN);
    // Map light value to contrast and set display brightness
    uint8_t contrast = mapLightToContrast(lightValue);
    display.setContrast(contrast);

    lastLightRead = currentTime;
  }

  // Read sensor every 0.1 seconds
  if (currentTime - lastSensorRead >= SENSOR_READ_INTERVAL)
  {
    uint16_t distance = sensor.readRangeContinuousMillimeters();
    if (sensor.timeoutOccurred())
    {
      Serial.println("Sensor timeout!");
    }
    else
    {
      if (distance < SCARED_THRESHOLD)
      {
        if (state != SCARED)
        {
          Serial.println("Interaction: Scared state");
          setState(SCARED);
          isInteracting = true;
        }
      }
      else if (distance < LAUGHING_THRESHOLD)
      {
        if (state != LAUGHING)
        {
          Serial.println("Interaction: Laughing state");
          setState(LAUGHING);
          isInteracting = true;
        }
      }
      else if (isInteracting)
      {
        Serial.println("End interaction");
        isInteracting = false;
        setState(IDLE);
      }
    }
    lastSensorRead = currentTime;
  }

  updateState();
  updatePupilPosition();
  updateBlink();
  updateDeadEye();
  updateJiggle();

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
  case LAUGHING:
    drawLaughing();
    break;
  case ANGRY:
    drawAngry();
    break;
  case SLEEP:
    drawSleep();
    break;
  }

  display.display();

  // Use shorter delay for NOISE state to make animation smoother
  if (state == NOISE)
  {
    delay(20); // 20ms delay for noise (50fps)
  }
  else if (state == SCARED)
  {
    delay(20); // 10ms delay for scared (100fps)
  }
  else if (state == LAUGHING)
  {
    delay(5); // 5ms delay for laughing (200fps)
  }
  else if (state == SLEEP)
  {
    delay(10); // 10ms delay for sleep (100fps)
  }
  else
  {
    delay(100); // 100ms delay for other states (10fps)
  }
}

void setState(EmotionState newState)
{
  return;
  // Perform any necessary actions before changing state
  switch (newState)
  {
  case DEAD:
    deadEyeOpen = false; // Ensure both eyes start as X's
    nextDeadEyeTime = millis() + random(MIN_TIME_BETWEEN_DEAD_EYE, MAX_TIME_BETWEEN_DEAD_EYE);
    break;
  case LAUGHING:
    isJiggling = false;               // Reset jiggle state
    lastJiggleStateChange = millis(); // Reset jiggle timer
    break;
  case NOISE:
    // Could add noise-specific initialization here
    break;
  }

  // Change the state
  state = newState;
  lastStateChange = millis();
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
      setState(WINK); // Go to blink after idle
      break;
    case WINK:
      setState(HAPPY); // Go to happy after blink
      break;
    case HAPPY:
      setState(LAUGHING); // Go to laughing after happy
      break;
    case LAUGHING:
      setState(SAD);
      break;
    case SAD:
      setState(ANGRY); // Go to angry after sad
      break;
    case ANGRY:
      setState(SCARED);
      break;
    case SCARED:
      setState(DEAD);
      break;
    case DEAD:
      setState(NOISE);
      break;
    case NOISE:
      setState(SLEEP); // Go to sleep after noise
      break;
    case SLEEP:
      setState(IDLE); // Go back to idle after sleep
      break;
    }
  }
}

void updatePupilPosition()
{
  unsigned long currentTime = millis();

  if (state == SCARED)
  {
    // Quick left-right shaking for scared state
    if (currentTime - lastScaredShake >= SCARED_SHAKE_INTERVAL)
    {
      // Toggle shake direction
      shakeDirection = !shakeDirection;
      // Set pupil position based on direction
      targetPupilOffsetX = shakeDirection ? SCARED_SHAKE_AMOUNT : -SCARED_SHAKE_AMOUNT;
      targetPupilOffsetY = -MAX_PUPIL_OFFSET; // Keep looking up
      lastScaredShake = currentTime;
    }
  }
  else
  {
    // Normal random movement for other states
    if (currentTime - lastPupilMove >= PUPIL_MOVE_INTERVAL)
    {
      targetPupilOffsetX = random(-MAX_PUPIL_OFFSET, MAX_PUPIL_OFFSET + 1);
      targetPupilOffsetY = random(-MAX_PUPIL_OFFSET, MAX_PUPIL_OFFSET + 1);
      lastPupilMove = currentTime;
    }
  }

  // Gradually move current position towards target
  if (pupilOffsetX < targetPupilOffsetX)
  {
    pupilOffsetX += PUPIL_MOVE_STEP;
  }
  else if (pupilOffsetX > targetPupilOffsetX)
  {
    pupilOffsetX -= PUPIL_MOVE_STEP;
  }

  if (pupilOffsetY < targetPupilOffsetY)
  {
    pupilOffsetY += PUPIL_MOVE_STEP;
  }
  else if (pupilOffsetY > targetPupilOffsetY)
  {
    pupilOffsetY -= PUPIL_MOVE_STEP;
  }
}

void updateBlink()
{
  // Don't blink in DEAD state
  if (state == DEAD)
  {
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
  if (state != DEAD)
  {
    deadEyeOpen = false;
    return;
  }

  unsigned long currentTime = millis();

  if (deadEyeOpen)
  {
    // Check if eye should close
    if (currentTime - lastDeadEyeChange >= DEAD_EYE_OPEN_DURATION)
    {
      deadEyeOpen = false;
      nextDeadEyeTime = currentTime + random(MIN_TIME_BETWEEN_DEAD_EYE, MAX_TIME_BETWEEN_DEAD_EYE);
    }
  }
  else
  {
    // Check if eye should open
    if (currentTime >= nextDeadEyeTime)
    {
      deadEyeOpen = true;
      lastDeadEyeChange = currentTime;
    }
  }
}

void updateJiggle()
{
  if (state != LAUGHING)
  {
    eyeJiggleOffset = 0;
    mouthJiggleOffset = 0;
    isJiggling = false;
    return;
  }

  unsigned long currentTime = millis();

  // Check if it's time to change jiggle state
  if (currentTime - lastJiggleStateChange >= (isJiggling ? JIGGLE_DURATION : STATIC_DURATION))
  {
    isJiggling = !isJiggling;
    lastJiggleStateChange = currentTime;
    if (!isJiggling)
    {
      // Reset offsets when entering static state
      eyeJiggleOffset = 0;
      mouthJiggleOffset = 0;
    }
  }

  // Only update jiggle if in jiggling state
  if (isJiggling && currentTime - lastJiggleTime >= JIGGLE_INTERVAL)
  {
    // Update jiggle offsets using sine waves with different speeds and phase offset
    eyeJiggleOffset = MAX_JIGGLE * sin((float)currentTime * EYE_JIGGLE_SPEED);
    mouthJiggleOffset = MAX_JIGGLE * sin((float)currentTime * MOUTH_JIGGLE_SPEED + PHASE_OFFSET);
    lastJiggleTime = currentTime;
  }
}

void drawEyebrews(EmotionState state)
{
  int leftX = faceCenterX - eyeDistance / 2 - eyebrewLength / 2;
  int rightX = faceCenterX + eyeDistance / 2 - eyebrewLength / 2;
  int y = faceCenterY - eyebrewOffset;

  int sadEyebrewYOffset = -5;
  int sadEyebrewLength = eyebrewLength * .7;
  int angryEyebrewOffset = 15; // How much to move eyebrows inward for angry state

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
    display.drawLine(leftX + (eyebrewLength - sadEyebrewLength), y + sadEyebrewYOffset + 5, leftX + (eyebrewLength - sadEyebrewLength) + sadEyebrewLength, y + sadEyebrewYOffset, SH110X_WHITE);
    display.drawLine(rightX, y + sadEyebrewYOffset, rightX + sadEyebrewLength, y + sadEyebrewYOffset + 5, SH110X_WHITE);
    break;
  case ANGRY:
    // Inverted sad eyebrows, but closer together
    display.drawLine(leftX + angryEyebrewOffset + (eyebrewLength - sadEyebrewLength), y + sadEyebrewYOffset,
                     leftX + angryEyebrewOffset + (eyebrewLength - sadEyebrewLength) + sadEyebrewLength, y + sadEyebrewYOffset + 10, SH110X_WHITE);
    display.drawLine(rightX - angryEyebrewOffset, y + sadEyebrewYOffset + 10,
                     rightX - angryEyebrewOffset + sadEyebrewLength, y + sadEyebrewYOffset, SH110X_WHITE);
    break;
  case SCARED:
    // Raised and angled eyebrows
    display.drawLine(leftX, y, leftX + eyebrewLength, y - 4, SH110X_WHITE);
    display.drawLine(rightX, y, rightX + eyebrewLength, y - 4, SH110X_WHITE);
    break;
  case SLEEP:
    // Draw eyebrows with slightly lower ends
    display.drawLine(leftX, y, leftX + eyebrewLength, y - 5, SH110X_WHITE);
    display.drawLine(rightX, y - 5, rightX + eyebrewLength, y, SH110X_WHITE);
    break;
  }
}

void drawEyes(EmotionState state)
{
  int leftX = faceCenterX - eyeDistance / 2 - eyeSize / 2;
  int rightX = faceCenterX + eyeDistance / 2 - eyeSize / 2;
  int y = faceCenterY - eyeOffset;
  int laughingEyeWidth = eyeSize * 0.7;             // Make eyes 70% of normal width
  int eyeOffset = (eyeSize - laughingEyeWidth) / 2; // Center the narrower eyes

  // Handle blinking for all states except WINK, DEAD, LAUGHING, and SLEEP
  if (isBlinking && state != WINK && state != DEAD && state != LAUGHING && state != SLEEP)
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
    if (deadEyeOpen)
    {
      // Normal eye with pupil
      display.fillCircle(leftX + eyeSize / 2, y, eyeSize / 2, SH110X_WHITE);
      display.fillCircle(leftX + eyeSize / 2 + pupilOffsetX, y + pupilOffsetY, eyeSize / 6, SH110X_BLACK);
    }
    else
    {
      // X-shaped eye
      display.drawLine(leftX + eyeSize / 4, y - eyeSize / 4, leftX + 3 * eyeSize / 4, y + eyeSize / 4, SH110X_WHITE);
      display.drawLine(leftX + eyeSize / 4, y + eyeSize / 4, leftX + 3 * eyeSize / 4, y - eyeSize / 4, SH110X_WHITE);
    }
    // Right eye always X
    display.drawLine(rightX + eyeSize / 4, y - eyeSize / 4, rightX + 3 * eyeSize / 4, y + eyeSize / 4, SH110X_WHITE);
    display.drawLine(rightX + eyeSize / 4, y + eyeSize / 4, rightX + 3 * eyeSize / 4, y - eyeSize / 4, SH110X_WHITE);
    break;
  case LAUGHING:
    // Draw closed eyes as half-ellipses with jiggle
    for (int i = 0; i <= laughingEyeWidth; i++)
    {
      // Calculate y offset using ellipse equation: y = b * sqrt(1 - (x/a)^2)
      // where a = laughingEyeWidth/2 and b = eyeSize/4
      float x = (float)i / (laughingEyeWidth / 2) - 1.0; // Normalize x to [-1, 1]
      int yOffset = (eyeSize / 4) * sqrt(1.0 - x * x);
      // Add eye jiggle to y position
      int jiggledY = y - yOffset + eyeJiggleOffset;
      display.drawPixel(leftX + eyeOffset + i, jiggledY, SH110X_WHITE);
      display.drawPixel(rightX + eyeOffset + i, jiggledY, SH110X_WHITE);
    }
    break;
  case SLEEP:
    // Draw closed eyes as inverted half-ellipses (similar to laughing but upside down)
    for (int i = 0; i <= laughingEyeWidth; i++)
    {
      float x = (float)i / (laughingEyeWidth / 2) - 1.0; // Normalize x to [-1, 1]
      int yOffset = (eyeSize / 4) * sqrt(1.0 - x * x);
      display.drawPixel(leftX + eyeOffset + i, y + yOffset, SH110X_WHITE);  // Changed - to + for inversion
      display.drawPixel(rightX + eyeOffset + i, y + yOffset, SH110X_WHITE); // Changed - to + for inversion
    }
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
  unsigned long currentTime = millis();

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
    display.drawLine(x, y, x + mouthSize, y, SH110X_WHITE); //  horizontal line
    for (int i = 0; i < mouthSize; i++)
    {
      int yOffset = mouthSize / 2 * sin((float)i / (mouthSize)*PI);
      display.drawPixel(x + i, y + yOffset, SH110X_WHITE); // Down curved line
    }
    break;
  case SAD:
    // Draw V shape for sad mouth
    display.drawLine(x, y, x + mouthSize / 2, y - mouthSize / 2, SH110X_WHITE);
    display.drawLine(x + mouthSize / 2, y - mouthSize / 2, x + mouthSize, y, SH110X_WHITE);
    break;
  case ANGRY:
    // Straight horizontal line for angry mouth
    display.drawLine(x, y, x + mouthSize, y, SH110X_WHITE);
    break;
  case SLEEP:
    // Animate sleeping mouth circle
    if (sleepMouthSize > MAX_SLEEP_MOUTH)
      sleepMouthGrowing = false;
    else if (sleepMouthSize < MIN_SLEEP_MOUTH)
      sleepMouthGrowing = true;

    if (sleepMouthGrowing)
      sleepMouthSize += SLEEP_MOUTH_SPEED;
    else 
      sleepMouthSize -= SLEEP_MOUTH_SPEED;

    display.drawCircle(faceCenterX, y, sleepMouthSize, SH110X_WHITE);
    break;
  case SCARED:
    // Open mouth
    display.drawCircle(x + mouthSize / 2, y, mouthSize / 3, SH110X_WHITE);
    break;
  case LAUGHING:
    // Draw complete oval mouth with jiggle
    for (int i = 0; i <= mouthSize; i++)
    {
      // Calculate y offset using ellipse equation: y = b * sqrt(1 - (x/a)^2)
      // where a = mouthSize/2 and b = mouthSize/4
      float x = (float)i / (mouthSize / 2) - 1.0; // Normalize x to [-1, 1]
      int yOffset = (mouthSize / 4) * sqrt(1.0 - x * x);
      // Add mouth jiggle to y position
      int jiggledY = y + yOffset + mouthJiggleOffset;
      display.drawPixel(faceCenterX - mouthSize / 2 + i, jiggledY, SH110X_WHITE);
      // Draw bottom half of oval
      jiggledY = y - yOffset + mouthJiggleOffset;
      display.drawPixel(faceCenterX - mouthSize / 2 + i, jiggledY, SH110X_WHITE);
    }
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
  for (int y = 0; y < SCREEN_HEIGHT; y++)
  {
    for (int x = 0; x < SCREEN_WIDTH; x++)
    {
      // Create a more TV-like noise pattern by grouping pixels
      if (random(100) < 50)
      { // 50% chance of pixel being on
        // Draw a small cluster of pixels for more realistic TV noise
        for (int dy = 0; dy < 2; dy++)
        {
          for (int dx = 0; dx < 2; dx++)
          {
            if (x + dx < SCREEN_WIDTH && y + dy < SCREEN_HEIGHT)
            {
              display.drawPixel(x + dx, y + dy, SH110X_WHITE);
            }
          }
        }
      }
    }
  }
}

void drawLaughing()
{
  drawEyebrews(IDLE); // Use same eyebrows as idle
  drawEyes(LAUGHING);
  drawMouth(LAUGHING);
}

void drawAngry()
{
  drawEyebrews(ANGRY);
  drawEyes(ANGRY);
  drawMouth(ANGRY);
}

void drawSleep()
{
  drawEyebrews(SLEEP);
  drawEyes(SLEEP);
  drawMouth(SLEEP);
}
