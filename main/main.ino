#include <Arduino.h>

#include "nixie_tube_clock.h"

NixieTubeClock nixie_tube_clock;

void setup() {
  Serial.begin(9600);
  nixie_tube_clock.Setup();
}

void loop() {
  nixie_tube_clock.Loop();
  delay(2);  // 按鈕與動畫都用 millis() 計時, 這裡只需要很短的間隔
}
