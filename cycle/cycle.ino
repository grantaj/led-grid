#include "LedControl.h"

// Define pins: (DIN, CLK, CS/LOAD, number of devices)
LedControl lc = LedControl(11, 13, 10, 1);

void setup() {
  lc.shutdown(0, false);   // Wake up the MAX7219
  lc.setIntensity(0, 8);   // Set brightness (0-15)
  lc.clearDisplay(0);      // Clear display
}

void loop() {
  for (int row = 0; row < 8; row++) {
    for (int col = 0; col < 8; col++) {
      lc.setLed(0, row, col, true);  // Turn LED on
      delay(100);                   // Small delay
      lc.setLed(0, row, col, false); // Turn LED off
    }
  }
}
