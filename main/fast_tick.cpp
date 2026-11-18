#include "fast_tick.h"

#include <Arduino.h>

#include "colon_led.h"
#include "tube_driver.h"

ISR(TIMER2_COMPA_vect) {
  TubeDriver::Tick();  // 先送管子資料 (可能會拉動「時」的 latch = pin 12)
  ColonLed::Tick();    // 再決定冒號 LED 這一刻亮或暗, pin 12 的最後狀態由它決定
}

namespace FastTick {

void Begin() {
  // CTC 模式, 除頻 8 -> 2MHz, 數到 125 觸發一次 -> 16kHz
  noInterrupts();
  TCCR2A = _BV(WGM21);
  TCCR2B = _BV(CS21);
  OCR2A = 124;
  TCNT2 = 0;
  TIMSK2 = _BV(OCIE2A);
  interrupts();
}

}  // namespace FastTick
