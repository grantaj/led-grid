#include <LedControl.h>

LedControl lc = LedControl(11, 13, 10, 1);

// Timing and phase control
unsigned long phaseStartTime;
unsigned long lastGlitchUpdate = 0;
unsigned long lastBreathUpdate = 0;

enum Phase { EMERGE, GLITCH, ERODE, WAIT };
Phase currentPhase = EMERGE;

// Grid states
bool grid[8][8] = {false};
bool decayGrid[8][8] = {false};
unsigned long decayStart[8][8] = {0};
bool glitchGrid[8][8] = {false};

// Emerge tracking
int litCount = 0;

// Breathing effect
int baseBrightness = 8; // Center brightness during glitch
int breathDirection = 1;
int currentBreath = 0;

// Configurable parameters
const int emergeDelay = 50;
const int glitchDuration = 10000;
const int glitchUpdateInterval = 300;
const int breathUpdateInterval = 100;
const int erodeDelay = 50;
const int waitDuration = 5000;

void setup() {
  lc.shutdown(0, false);
  lc.setIntensity(0, 0);
  lc.clearDisplay(0);
  randomSeed(analogRead(0));
  phaseStartTime = millis();
}

void loop() {
  switch (currentPhase) {
    case EMERGE:
      emergePhase();
      break;
    case GLITCH:
      glitchPhase();
      break;
    case ERODE:
      erodePhase();
      break;
    case WAIT:
      waitPhase();
      break;
  }
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

  int brightness = map(litCount, 0, 64, 0, 15);
  brightness = constrain(brightness, 0, 15);
  lc.setIntensity(0, brightness);

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

  // Breathing effect: oscillate brightness gently
  if (now - lastBreathUpdate > breathUpdateInterval) {
    currentBreath += breathDirection;
    if (currentBreath > 3 || currentBreath < -3) {
      breathDirection *= -1; // Reverse breathing direction
    }
    int breathBrightness = baseBrightness + currentBreath;
    breathBrightness = constrain(breathBrightness, 0, 15);
    lc.setIntensity(0, breathBrightness);

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
          decayStart[r][c] = now + random(1000, 6000) + random(0, 6000);
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
          int flickerChance;

          if (timeLeft > 4000) {
            flickerChance = 50;
          } else if (timeLeft > 2000) {
            flickerChance = 20;
          } else if (timeLeft > 1000) {
            flickerChance = 8;
          } else {
            flickerChance = 4;
          }

          if (random(flickerChance) == 0) {
            if (timeLeft > 1000) {
              // Normal single flicker
              lc.setLed(0, r, c, false);
              delay(30);  // Longer visible flicker
              lc.setLed(0, r, c, true);
            } else {
              // PANIC STORM MODE
              int flickerCount = random(2, 5); // 2 to 4 flickers
              for (int f = 0; f < flickerCount; f++) {
                lc.setLed(0, r, c, false);
                delay(random(20, 50)); // Random short blackout
                lc.setLed(0, r, c, true);
                delay(random(20, 50)); // Random short ON time
              }
            }
          }

        }
      }
    }
  }

  int brightness = map(aliveCount, 0, 64, 0, 15);
  brightness = constrain(brightness, 0, 15);
  lc.setIntensity(0, brightness);

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
  lc.setIntensity(0, 0); // Fully dim while waiting

  if (now - phaseStartTime > waitDuration) {
    currentPhase = EMERGE;
    phaseStartTime = millis();
  }
}
