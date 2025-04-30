#include <LedControl.h>
#include <NewPing.h>

// LED Matrix setup
LedControl lc = LedControl(11, 13, 10, 1);

// Ultrasonic sensor setup
const int trigPin = 7;
const int echoPin = 6;
const int maxDistance = 200; // cm
NewPing sonar(trigPin, echoPin, maxDistance);

// Timing
unsigned long phaseStartTime;
unsigned long lastGlitchUpdate = 0;
unsigned long lastBreathUpdate = 0;
unsigned long lastSensorRead = 0;
unsigned long lastPresenceTime = 0;

// Phases
enum Phase { SLEEP, EMERGE, GLITCH, ERODE, WAIT };
Phase currentPhase = SLEEP;

// Grid states
bool grid[8][8] = {false};
bool decayGrid[8][8] = {false};
unsigned long decayStart[8][8] = {0};
bool glitchGrid[8][8] = {false};

// Emerge tracking
int litCount = 0;

// Breathing effect
int baseBrightness = 8;
int breathDirection = 1;
int currentBreath = 0;
int currentBrightness = 0; // Shared global brightness

// Configurable parameters
const int emergeDelay = 50;
const int glitchDuration = 10000; // ms (10 seconds)
const int glitchUpdateInterval = 300;
const int breathUpdateInterval = 100;
const int erodeDelay = 50;
const int waitDuration = 2000;
const int sensorInterval = 300;
const int presenceThresholdCM = 80;
const unsigned long presenceTimeout = 2000; // ms (2 seconds)

void setup() {
  lc.shutdown(0, false);
  currentBrightness = 0;
  lc.setIntensity(0, currentBrightness);
  lc.clearDisplay(0);
  randomSeed(analogRead(0));
  phaseStartTime = millis();
}

void loop() {
  handlePresence();

  switch (currentPhase) {
    case SLEEP: sleepPhase(); break;
    case EMERGE: emergePhase(); break;
    case GLITCH: glitchPhase(); break;
    case ERODE: erodePhase(); break;
    case WAIT: waitPhase(); break;
  }
}

void handlePresence() {
  unsigned long now = millis();
  if (now - lastSensorRead > sensorInterval) {
    unsigned int distance = sonar.ping_cm();
    bool personNearby = (distance > 0 && distance < presenceThresholdCM);
    lastSensorRead = now;

    if (personNearby) {
      lastPresenceTime = now;
      if (currentPhase == SLEEP) {
        currentPhase = EMERGE;
        phaseStartTime = now;
      }
    } else {
      if ((now - lastPresenceTime > presenceTimeout) && currentPhase != SLEEP) {
        currentPhase = SLEEP;
        lc.clearDisplay(0);
        currentBrightness = 0;
        lc.setIntensity(0, currentBrightness);

        // 🧹 Clear state to avoid flicker or stuck LEDs after restart
        memset(grid, 0, sizeof(grid));
        memset(decayGrid, 0, sizeof(decayGrid));
        memset(decayStart, 0, sizeof(decayStart));
        litCount = 0;
      }
    }
  }
}

void sleepPhase() {
  // Do nothing while sleeping
}

void emergePhase() {
  if (litCount == 0) {
    lc.clearDisplay(0);
  }

  int row = random(8);
  int col = random(8);
  if (!grid[row][col]) {
    grid[row][col] = true;
    lc.setLed(0, row, col, true);
    litCount++;
  }

  if (random(10) == 0) {
    int flickerRow = random(8);
    int flickerCol = random(8);
    if (grid[flickerRow][flickerCol]) {
      lc.setLed(0, flickerRow, flickerCol, false);
      delay(20);
      lc.setLed(0, flickerRow, flickerCol, true);
    }
  }

  currentBrightness = map(litCount, 0, 64, 0, 15);
  currentBrightness = constrain(currentBrightness, 0, 15);
  lc.setIntensity(0, currentBrightness);

  delay(emergeDelay);

  if (litCount >= 64) {
    litCount = 0;
    currentPhase = GLITCH;
    phaseStartTime = millis();
    lastBreathUpdate = millis();
    baseBrightness = 8;
    currentBreath = 0;
    breathDirection = 1;
  }
}

void glitchPhase() {
  unsigned long now = millis();

  if (now - lastGlitchUpdate > glitchUpdateInterval) {
    for (int r = 0; r < 8; r++) {
      for (int c = 0; c < 8; c++) {
        glitchGrid[r][c] = (random(15) == 0);
      }
    }
    lastGlitchUpdate = now;
  }

  for (int r = 0; r < 8; r++) {
    for (int c = 0; c < 8; c++) {
      if (glitchGrid[r][c]) {
        lc.setLed(0, r, c, random(2));
      } else {
        lc.setLed(0, r, c, grid[r][c]);
      }
    }
  }

  if (now - lastBreathUpdate > breathUpdateInterval) {
    currentBreath += breathDirection;
    if (currentBreath > 3 || currentBreath < -3) {
      breathDirection *= -1;
    }
    int breathBrightness = baseBrightness + currentBreath;
    breathBrightness = constrain(breathBrightness, 0, 15);
    currentBrightness = breathBrightness;
    lc.setIntensity(0, currentBrightness);
    lastBreathUpdate = now;
  }

  delay(40);

  if (now - phaseStartTime > glitchDuration) {
    currentPhase = ERODE;
    phaseStartTime = now;

    for (int r = 0; r < 8; r++) {
      for (int c = 0; c < 8; c++) {
        decayGrid[r][c] = grid[r][c];
        if (grid[r][c]) {
          decayStart[r][c] = now + random(2000, 12000);
        } else {
          decayStart[r][c] = 0;
        }
      }
    }
    memset(glitchGrid, 0, sizeof(glitchGrid));
  }
}

void erodePhase() {
  unsigned long now = millis();
  int aliveCount = 0;

  for (int r = 0; r < 8; r++) {
    for (int c = 0; c < 8; c++) {
      if (decayGrid[r][c]) {
        if (now >= decayStart[r][c]) {
          lc.setLed(0, r, c, false);
          decayGrid[r][c] = false;
        } else {
          aliveCount++;

          unsigned long timeLeft = decayStart[r][c] - now;
          int flickerChance = (timeLeft > 4000) ? 30 : (timeLeft > 2000) ? 12 : (timeLeft > 1000) ? 6 : 3;

          if (random(flickerChance) == 0) {
            if (timeLeft > 1000) {
              lc.setLed(0, r, c, false);
              delay(30);
              lc.setLed(0, r, c, true);
            } else {
              int flickerCount = random(2, 5);
              for (int f = 0; f < flickerCount; f++) {
                lc.setLed(0, r, c, false);
                delay(random(20, 50));
                lc.setLed(0, r, c, true);
                delay(random(20, 50));
              }
            }
          }
        }
      }
    }
  }

  int targetBrightness = map(aliveCount, 0, 64, 0, 15);
  if (currentBrightness < targetBrightness) currentBrightness++;
  else if (currentBrightness > targetBrightness) currentBrightness--;
  currentBrightness = constrain(currentBrightness, 0, 15);
  lc.setIntensity(0, currentBrightness);

  delay(erodeDelay);

  if (aliveCount == 0) {
    lc.clearDisplay(0);
    memset(grid, 0, sizeof(grid));
    memset(decayGrid, 0, sizeof(decayGrid));
    memset(decayStart, 0, sizeof(decayStart));
    currentPhase = WAIT;
    phaseStartTime = millis();
  }
}

void waitPhase() {
  unsigned long now = millis();
  currentBrightness = 0;
  lc.setIntensity(0, currentBrightness);
  if (now - phaseStartTime > waitDuration) {
    currentPhase = EMERGE;
    phaseStartTime = millis();
  }
}


