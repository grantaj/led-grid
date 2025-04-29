#include <LedControl.h>

LedControl lc = LedControl(11, 13, 10, 1);

const int delayTime = 50;
unsigned long phaseStartTime;
enum Phase { EMERGE, GLITCH, ERODE };
Phase currentPhase = EMERGE;

void setup() {
  lc.shutdown(0, false);
  lc.setIntensity(0, 8);
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
  }
}

void emergePhase() {
  static int count = 0;
  static bool grid[8][8] = {false};

  int row = random(8);
  int col = random(8);
  if (!grid[row][col]) {
    grid[row][col] = true;
    lc.setLed(0, row, col, true);
    count++;
  }

  delay(delayTime);

  if (count >= 64) {
    currentPhase = GLITCH;
    phaseStartTime = millis();
  }
}

void glitchPhase() {
  int row = random(8);
  int col = random(8);
  bool state = random(2);
  lc.setLed(0, row, col, state);

  delay(40);

  if (millis() - phaseStartTime > 4000) { // 4 seconds of glitching
    currentPhase = ERODE;
    phaseStartTime = millis();
  }
}

void erodePhase() {
  static bool grid[8][8] = {true};

  int row = random(8);
  int col = random(8);
  if (grid[row][col]) {
    lc.setLed(0, row, col, false);
    grid[row][col] = false;
  }

  delay(60);

  // Check if all pixels are off
  bool anyOn = false;
  for (int r = 0; r < 8; r++) {
    for (int c = 0; c < 8; c++) {
      if (grid[r][c]) {
        anyOn = true;
        break;
      }
    }
    if (anyOn) break;
  }

  if (!anyOn) {
    lc.clearDisplay(0);
    currentPhase = EMERGE;
    phaseStartTime = millis();
  }
}
