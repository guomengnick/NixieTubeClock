#ifndef NIXIE_TUBE_CLOCK_H
#define NIXIE_TUBE_CLOCK_H

#include <Arduino.h>
#include <Ds1302.h>

#include "animations.h"
#include "button_input.h"
#include "display.h"

// 時鐘本體: 一個狀態機
//
// 任何時候只會在一個狀態 (state_), 由這個狀態決定「管子顯示什麼」和「按鈕做什麼」。
// 原本用 current_mode_ + cleaning_ + date_showing_ + rolling_ 好幾個旗標拼出現在在幹嘛,
// 每加一個功能就要回頭在好幾個地方補 if; 現在要加新畫面, 只要加一個狀態, 寫它的進入、更新、按鈕三件事。
//
//   狀態            怎麼進來                       怎麼離開 (都回到 kTime)
//   kTime           開機、其他狀態結束
//   kRolling        kTime 每分鐘 0 秒              4 支管子都停好
//   kCleaning       每 1~3 分鐘 / 兩顆一起按 1 秒   清完 (每次隨機挑一種動畫)
//   kDate           短按 front                     3 秒 / 再短按一次
//   kSetSecond..    長按 back 2 秒 (back 循環 秒->分->時)   8 秒沒按
//   kSetBrightness  長按 front 2 秒                5 秒沒按
// 前 4 個是「正常」這一組: 在它們之間按鈕手勢都一樣, 例如清管到一半也可以短按看日期。
class NixieTubeClock {
 public:
  NixieTubeClock();
  void Setup();  // Serial.begin() 之後呼叫
  void Loop();   // 每個 loop 呼叫一次, 不會卡住 (除了解碼秀)

 private:
  enum class State : uint8_t {
    // 「正常」這一組: 看得到時間或時間相關的畫面, 按鈕手勢都一樣
    kTime,           // 正常走時
    kRolling,        // 換分鐘掃描 (電梯)
    kCleaning,       // 清管動畫
    kDate,           // 短按 front 看日期
    // 設定
    kSetSecond,
    kSetMinute,
    kSetHour,
    kSetBrightness,
  };
  static bool IsNormal(State s) { return s <= State::kDate; }
  static bool IsSettingTime(State s) { return s >= State::kSetSecond && s <= State::kSetHour; }

  // 狀態機的三件事
  void EnterState(State next);  // 離開舊狀態、進入新狀態 (只有這裡會改 state_)
  void HandleInput();           // 按鈕事件 -> 換狀態 / 設定
  void UpdateState();           // 每個 loop: 推進動畫、檢查逾時、更新顯示

  void ReadClock();             // 每個 loop 讀一次 RTC, 跳秒時順便做誤差補償
  void UpdateColon();
  void PrintSecondLine();       // 每秒一行日誌
  void HandleSerial();          // USB 對時

  // 設定時間
  TimeUnit SettingUnit() const;
  void HandleTimeSettingInput();
  void AdjustTime();
  void BlinkSettingTube();
  void FinishTimeSetting();
  // 亮度
  void LoadBrightness();
  void HandleBrightnessInput();
  void ApplyBrightness(bool save);
  void ShowBrightness();
  // RTC 與誤差補償
  void InitRtc();
  bool WriteRtcVerified(const Ds1302::DateTime& time);
  void ApplyDriftCompensation(Ds1302::DateTime* now);
  void PrintDriftRate();

  Ds1302 rtc_;
  Display display_;
  ButtonInput buttons_;
  TubeCleaner cleaner_;
  LadderWalk roll_;  // 換分鐘掃描

  State state_ = State::kTime;
  unsigned long state_start_ms_ = 0;    // 進入目前狀態的時間
  unsigned long last_activity_ms_ = 0;  // 設定模式最後一次按鍵 (逾時用)

  Ds1302::DateTime now_;        // 這一輪 loop 讀到的時間
  bool new_second_ = false;     // 這一輪剛跳秒
  uint8_t last_second_ = 255;   // 255 = 還沒讀過
  // 少讀 RTC (見 config.h 的 kRtcQuietMs): 抓到跳秒的時刻, 之後休息到快下一秒
  unsigned long second_start_ms_ = 0;  // 抓到跳秒的 millis()
  unsigned long rtc_read_ms_ = 0;      // 上一次讀 RTC 的 millis()
  bool second_edge_known_ = false;     // 確定 second_start_ms_ 就是跳秒那一刻; 寫過 RTC 就要重抓
  unsigned long breath_start_ms_ = 0;  // 冒號呼吸燈: 這次呼吸開始的時間

  unsigned long blink_start_ms_ = 0;      // 設定時間: 閃爍從什麼時候開始算
  unsigned long repeat_last_ms_ = 0;      // 按住連加: 上一次 +1 的時間
  unsigned long repeat_interval_ms_ = 0;  // 按住連加: 距離下一次 +1 的間隔, 越按越短
  bool time_adjusted_ = false;            // 這次設定有改到時間, 離開時通知誤差補償
  int last_logged_show_value_ = -1;       // 診斷日誌: 設定中的管子上次印出的數字, 變了才再印

  uint8_t brightness_percent_ = 0;  // 管子亮度 (%), 開機從 EEPROM 讀
  bool rtc_was_reset_ = false;      // 開機時 RTC 停著 (電池沒電), 被設回預設時間
};

#endif  // NIXIE_TUBE_CLOCK_H
