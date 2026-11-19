#ifndef ANIMATIONS_H
#define ANIMATIONS_H

#include <Arduino.h>
#include <Ds1302.h>

#include "display.h"

// 管子上的動畫。清管、換字掃描都是「Start() 一次, 之後每個 loop 呼叫 Update()」,
// 不用 delay(), 跑動畫時按鈕照樣有反應; Update() 回傳 false 表示播完了。

// 走梯子 (電梯): 要走的管子先跳到梯子最底下, 一階一階往上走到頂, 再往下走回現在時間的數字停住。
// 例如 5 -> 6: 0 1 2 3 4 5 6 7 8 9 8 7 6。梯子的順序見 config.h 的 kLadder。
// 停好的、不用走的管子跟著時間走 (秒照樣跳), 所以播完時畫面就是現在時間, 不會「跳」回來。
// 換分鐘掃描、清管的電梯和波浪都是用這個。
class LadderWalk {
 public:
  // move_mask: 第 i 個 bit = 第 i 支管子要走 (0 = 最左, 時十位)
  // stagger_ms: 每往右一支晚多久起步 (0 = 一起走, >0 = 由左到右的波浪)
  void Start(uint8_t move_mask, unsigned long step_ms, unsigned long stagger_ms, uint8_t fade);
  bool Update(Display& display, const Ds1302::DateTime& now);  // 回傳 false = 全部停好

 private:
  static const uint8_t kLanded = 255;   // pos_: 已經停好 / 不用走
  static const uint8_t kWaiting = 254;  // pos_: 還沒輪到起步 (波浪)

  uint8_t pos_[6];            // 走到路線上第幾格: 0~9 往上, 10~18 往下
  unsigned long next_ms_[6];  // 下一格的時間
  unsigned long step_ms_ = 0;
  uint8_t fade_ = 0;
};

// 換分鐘時, 時、分 4 支管子裡數字會變的那幾支 (給 LadderWalk 的 move_mask); 0 = 都沒變
uint8_t ChangedTimeTubes(const Display& display, const Ds1302::DateTime& now);

// 清管 (防陰極中毒): 每支管子的 0~9 都亮一次就達到效果, 所以每次隨機挑一種動畫:
//   流動: 6 支一起顯示同一個數字, 快速流過兩輪 (清完淡回時間)
//   電梯: 6 支一起慢慢走梯子, 數字融化接過去, 最後停在現在時間
//   波浪: 由左到右一支接一支走梯子, 像水波掃過去, 最後停在現在時間
// 另外負責排「下一次自動清管」的時間
class TubeCleaner {
 public:
  void ScheduleNext();  // 從現在起隨機 kAutoCleanMinSeconds~kAutoCleanMaxSeconds 秒後再清
  bool Due() const;     // 自動清管的時間到了
  void Start(Display& display);  // 隨機挑一種模式開始
  bool Update(Display& display, const Ds1302::DateTime& now);

 private:
  enum Mode : uint8_t { kFlow, kElevator, kWave, kModeCount };
  void ShowFlowStep(Display& display);

  Mode mode_ = kFlow;
  LadderWalk walk_;
  uint8_t step_ = 0;                   // 流動: 第幾步
  unsigned long step_ms_ = 0;          // 流動: 這一步要停多久, 越來越快
  unsigned long step_start_ms_ = 0;
  unsigned long scheduled_at_ms_ = 0;  // 上一次排時間的時刻
  unsigned long wait_seconds_ = 0;     // 排完之後再等多久
};

// 解碼秀 (說明見 config.h 的 kBootAnimation): 會卡住約 1.5 秒, 只在開機、USB 對時、整點報時用。
// sweep_digits = nullptr 時跳過掃描; 最後顯示 RTC 的現在時間
void RunDecodeShow(Display& display, Ds1302& rtc, const uint8_t sweep_digits[10],
                   unsigned long sweep_step_ms);

#endif  // ANIMATIONS_H
