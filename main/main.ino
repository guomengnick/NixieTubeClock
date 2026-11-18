#include <Arduino.h>

#include "shift_register.h"
#include "nixie_tube_clock.h"


NixieTubeClock nixie_tube_clock_;

void setup(){
  Serial.begin(9600);
  nixie_tube_clock_.Setup();
}

void loop(){
  nixie_tube_clock_.UpdateTimeIfNextSecond();
  nixie_tube_clock_.CheckButtonStatus();
  nixie_tube_clock_.HandleSerial();
  delay(2);//按鈕與清理動畫都用 millis() 計時, 這裡只需要很短的間隔
}