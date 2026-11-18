#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>
#include <Ds1302.h>

#include "tube_driver.h"

// 3 顆 74HC595 (= TubeDriver 的編號) 各管兩支管子
enum TimeUnit : uint8_t { kSecond, kMinute, kHour };

// 6 支輝光管 (由左到右: 時十 時個 分十 分個 秒十 秒個)
// 只負責「顯示什麼數字」, 真正送資料給 595 的是 TubeDriver 的中斷。
// fade_ticks: 換字淡入的速度 (TubeDriver::kFadeNormal 等), 預設立刻切換
class Display {
 public:
  static const uint8_t kBlank = 255;  // 數字大於 9 = 那支管子不亮

  void Begin();  // 設定腳位, 交給 TubeDriver 的中斷接手

  void ShowDigits(const uint8_t digits[6], uint8_t fade_ticks = TubeDriver::kFadeInstant);
  // 一組 (兩支) 管子: 十位、個位分開給
  void ShowPair(TimeUnit unit, uint8_t tens, uint8_t ones, uint8_t fade_ticks = TubeDriver::kFadeInstant);
  // 一組管子顯示 0~99; kBlank = 兩支都不亮
  void ShowNumber(TimeUnit unit, uint8_t value, uint8_t fade_ticks = TubeDriver::kFadeInstant);
  void ShowTime(const Ds1302::DateTime& t, uint8_t fade_ticks = TubeDriver::kFadeInstant);  // HH MM SS
  void ShowDate(const Ds1302::DateTime& t, uint8_t fade_ticks = TubeDriver::kFadeInstant);  // YY MM DD

  // 第 tube 支 (0 = 最左) 現在要求顯示的數字; 不亮或還沒顯示過回傳 kBlank
  uint8_t ShownDigit(uint8_t tube) const;
  // 最後一次送給這顆 595 的 byte (Q7..Q0), 給診斷日誌用; 還沒送過是 -1
  int LastData(TimeUnit unit) const { return last_data_[unit]; }

 private:
  int16_t last_data_[TubeDriver::kRegisters] = {-1, -1, -1};
};

#endif  // DISPLAY_H
