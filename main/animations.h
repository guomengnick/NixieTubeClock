#ifndef ANIMATIONS_H
#define ANIMATIONS_H

#include <Arduino.h>
#include <Ds1302.h>

#include "display.h"

// 管子上的動畫。清管、換字掃描都是「Start() 一次, 之後每個 loop 呼叫 Update()」,
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

// 換字掃描 (電梯): 時、分 4 支管子裡數字有變的, 先跳到梯子最底下, 一階一階走到頂,
// 再往下走回新數字停住。例如 5 -> 6: 0 1 2 3 4 5 6 7 8 9 8 7 6。梯子的順序見 config.h 的 kRollLadder
class DigitSweep {
 public:
  // 回傳 false = 沒有管子要動; true 時已經顯示第一格
  bool Start(Display& display, const Ds1302::DateTime& now);
  bool Update(Display& display);  // 回傳 false = 全部停好

 private:
  void Show(Display& display) const;

  uint8_t target_[4];         // 時十 時個 分十 分個 的新數字
  uint8_t pos_[4];            // 走到路線上第幾格 (0~9 往上, 10~18 往下)
  uint8_t last_[4];           // 終點那一格; 0 = 這支沒變, 不動
  unsigned long next_ms_[4];  // 下一格的時間
};

// 解碼秀 (說明見 config.h 的 kBootAnimation): 會卡住約 1.5 秒, 只在開機、USB 對時、整點報時用。
// sweep_digits = nullptr 時跳過掃描; 最後顯示 RTC 的現在時間
void RunDecodeShow(Display& display, Ds1302& rtc, const uint8_t sweep_digits[10],
                   unsigned long sweep_step_ms);

#endif  // ANIMATIONS_H
