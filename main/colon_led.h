#ifndef COLON_LED_H
#define COLON_LED_H

#include <Arduino.h>

// 冒號 LED 呼吸燈
//
// 冒號 LED 接在 pin 12, 不是硬體 PWM 腳 (Nano 只有 3/5/6/9/10/11 能 analogWrite, 而且都被 RTC 和 595 用掉了),
// 所以借 FastTick (Timer2) 每秒 16000 次的中斷, 自己決定 LED 這一刻亮或暗 = 軟體 PWM (128 階, 約 125Hz 不會閃)。
//
// 注意: pin 12 同時是「時」那顆 74HC595 的 STCP (大平台)。拉高拉低它只會把 595 裡同樣的資料再鎖存一次,
// 不影響顯示; 送資料給 595 也在同一個中斷裡 (TubeDriver), 不會互相打斷。
namespace ColonLed {

// 設定腳位並算好呼吸曲線, 只呼叫一次 (FastTick::Begin() 另外呼叫)
void Begin(uint8_t pin);

// 由 FastTick 的中斷呼叫, 不要在別的地方呼叫
void Tick();

// 依呼吸週期中的位置更新亮度, 每個 loop 呼叫:
// phase_ms 0 ~ kBreathPeriodMs-1, 前半段吸氣 (變亮), 後半段吐氣 (變暗)
void Update(unsigned long phase_ms);

// 最亮時的亮度 (%), 跟著管子的亮度一起調, 看起來才協調
void SetBrightness(uint8_t percent);

const unsigned long kBreathPeriodMs = 2000;  // 一次呼吸 2 秒, 跟原本「亮 1 秒暗 1 秒」同節奏

}  // namespace ColonLed

#endif  // COLON_LED_H
