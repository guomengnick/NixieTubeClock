#include "colon_led.h"

#include <math.h>

namespace {
  const uint8_t kLevels = 128;      // 軟體 PWM 的亮度階數
  const float kGamma = 2.2;         // 人眼對亮度是非線性的, 不做 gamma 校正會「一下就很亮, 然後一直卡在亮」
  const uint8_t kCurveSteps = 64;   // 吸氣半段的曲線取樣點數

  volatile uint8_t* g_port = nullptr;  // LED 腳位的輸出暫存器 (直接寫暫存器, 中斷裡用 digitalWrite 太慢)
  uint8_t g_mask = 0;
  volatile uint8_t g_duty = 0;          // 0 = 全暗, kLevels = 全亮
  uint8_t g_curve[kCurveSteps + 1];     // 吸氣曲線: 位置 0..kCurveSteps -> 亮度 0..kLevels
}

// Timer2 比較中斷, 每秒 16000 次: 每 128 次是一個 PWM 週期, 前 g_duty 次亮、其餘暗
ISR(TIMER2_COMPA_vect) {
  static uint8_t tick = 0;
  tick = (tick + 1) & (kLevels - 1);
  if (tick < g_duty) {
    *g_port |= g_mask;
  } else {
    *g_port &= ~g_mask;
  }
}

namespace ColonLed {

void Begin(uint8_t pin) {
  // 呼吸曲線: 半個餘弦 (慢慢起步、慢慢收尾) 再做 gamma 校正, 開機算一次存起來, 之後只查表
  for (uint8_t i = 0; i <= kCurveSteps; ++i) {
    const float eased = (1.0 - cos(PI * i / kCurveSteps)) / 2.0;  // 0 -> 1
    g_curve[i] = (uint8_t)(pow(eased, kGamma) * kLevels + 0.5);
  }

  pinMode(pin, OUTPUT);
  g_port = portOutputRegister(digitalPinToPort(pin));
  g_mask = digitalPinToBitMask(pin);

  // Timer2: CTC 模式, 除頻 8 -> 2MHz, 數到 125 觸發一次 -> 16kHz
  noInterrupts();
  TCCR2A = _BV(WGM21);
  TCCR2B = _BV(CS21);
  OCR2A = 124;
  TCNT2 = 0;
  TIMSK2 = _BV(OCIE2A);
  interrupts();
}

void Update(unsigned long phase_ms) {
  const unsigned long half = kBreathPeriodMs / 2;
  phase_ms %= kBreathPeriodMs;
  // 前半段往上爬, 後半段原路下降
  const unsigned long pos = phase_ms < half ? phase_ms : kBreathPeriodMs - phase_ms;
  g_duty = g_curve[pos * kCurveSteps / half];  // 單一 byte, 寫入本來就是原子的
}

}  // namespace ColonLed
