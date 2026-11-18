#ifndef COLON_LED_H
#define COLON_LED_H

#include <Arduino.h>

// 冒號 LED 呼吸燈
//
// 冒號 LED 接在 pin 12, 不是硬體 PWM 腳 (Nano 只有 3/5/6/9/10/11 能 analogWrite, 而且都被 RTC 和 595 用掉了),
// 所以用沒人在用的 Timer2 每秒中斷 16000 次, 自己決定 LED 這一刻亮或暗 = 軟體 PWM (128 階, 約 125Hz 不會閃)。
//
// 注意: pin 12 同時是「時」那顆 74HC595 的 STCP (大平台)。中斷一直拉高拉低它只會把 595 裡同樣的資料
// 再鎖存一次, 不影響顯示; 送資料給 595 的那一小段時間 ShiftRegister 會暫停中斷, 避免鎖存到一半的資料。
namespace ColonLed {

// 設定 Timer2 並開始輸出, 只呼叫一次
void Begin(uint8_t pin);

// 依呼吸週期中的位置更新亮度, 每個 loop 呼叫:
// phase_ms 0 ~ kBreathPeriodMs-1, 前半段吸氣 (變亮), 後半段吐氣 (變暗)
void Update(unsigned long phase_ms);

const unsigned long kBreathPeriodMs = 2000;  // 一次呼吸 2 秒, 跟原本「亮 1 秒暗 1 秒」同節奏

}  // namespace ColonLed

#endif  // COLON_LED_H
