#ifndef ANIMATIONS_H
#define ANIMATIONS_H

#include <Arduino.h>
#include <Ds1302.h>

#include "display.h"

// 管子上的動畫。清管、滾輪都是「Start() 一次, 之後每個 loop 呼叫 Update()」,
// 不用 delay(), 跑動畫時按鈕照樣有反應; Update() 回傳 false 表示播完了。

// 清管 (防陰極中毒): 每個數字快速亮過兩輪; 另外負責排「下一次自動清管」的時間
class TubeCleaner {
 public:
  void ScheduleNext();  // 從現在起隨機 kAutoCleanMinSeconds~kAutoCleanMaxSeconds 秒後再清
  bool Due() const;     // 自動清管的時間到了
  void Start(Display& display);
  bool Update(Display& display);

 private:
  void ShowStep(Display& display);

  uint8_t step_ = 0;
  unsigned long step_ms_ = 0;          // 這一步要停多久, 越來越快
  unsigned long step_start_ms_ = 0;
  unsigned long scheduled_at_ms_ = 0;  // 上一次排時間的時刻
  unsigned long wait_seconds_ = 0;     // 排完之後再等多久
};

// 滾輪換字 (吃角子老虎): 時、分 4 支管子裡數字有變的往上滾, 由左到右一支一支停 (見 config.h)
class SlotRoll {
 public:
  // 從管子上現在的數字滾到 now 的時、分; 回傳 false = 沒有管子要滾
  bool Start(const Display& display, const Ds1302::DateTime& now);
  bool Update(Display& display);

 private:
  uint8_t digit_[4];          // 時十 時個 分十 分個 目前滾到的數字
  uint8_t steps_left_[4];     // 還要滾幾格; 0 = 已經停在新數字
  unsigned long next_ms_[4];  // 下一格的時間
};

// 解碼秀 (說明見 config.h 的 kBootAnimation): 會卡住約 1.5 秒, 只在開機、USB 對時、整點報時用。
// sweep_digits = nullptr 時跳過掃描; 最後顯示 RTC 的現在時間
void RunDecodeShow(Display& display, Ds1302& rtc, const uint8_t sweep_digits[10],
                   unsigned long sweep_step_ms);

#endif  // ANIMATIONS_H
