#ifndef NIXIE_TUBE_CLOCK_INO
#define NIXIE_TUBE_CLOCK_INO
#include <Arduino.h>
#include <Ds1302.h>

#include "button.h"
#include "shift_register.h"
#include "tube_cleaner.h"


enum ClockMode { Normal, SettingSecond, SettingMinute, SettingHour };
enum TimeUnit {Second, Minut, Hour};

class NixieTubeClock{
  Ds1302 real_time_clock_;
  ShiftRegister time_registers_[3];
  friend class TubeCleaner;

  TubeCleaner cleaner_;

  //*************************************按鈕*****************************
  Button back_set_btn_;   // A1: 長按3秒進入設定模式 / 設定模式下切換時分秒
  Button front_add_btn_;  // A2: 設定模式下加一
  bool back_btn_used_in_combo_ = false;   // 兩顆一起按過(清理管子)，放開前不進入設定模式
  bool manual_clean_triggered_ = false;   // 同時長按只觸發一次清理
  unsigned long last_setting_activity_millsecond_ = 0;  // 設定模式下最後一次按鍵時間
  //*************************************清理管子*************************
  bool cleaning_ = false;
  unsigned long last_clean_tube_time_millsecond_ = 0;  // 上一次清管結束的時間
  unsigned long next_clean_after_seconds_ = 0;         // 距離上一次清管多久後再清
  uint8_t clean_step_ = 0;
  float clean_step_delay_millsecond_ = 0;
  unsigned long clean_step_start_millsecond_ = 0;
  //**********************************************************************
  unsigned long last_blink_millsecond_ = 0;
  unsigned long last_repeat_millsecond_ = 0;  // 按住連加: 上一次 +1 的時間
  unsigned long repeat_interval_ms_ = 0;      // 按住連加: 距離下一次 +1 的間隔, 越按越短
  unsigned long breath_start_millsecond_ = 0;  // 冒號呼吸燈: 這次呼吸開始的時間
  int last_logged_show_value_ = -1;
  bool time_adjusted_in_setting_ = false;  // 這次設定模式有改到時間, 離開時通知誤差補償
  bool rtc_was_reset_ = false;             // 開機時 RTC 停著 (電池沒電), 被設回預設時間  // 診斷日誌: 設定中的管子上次印出的數字, 變了才再印

  ClockMode current_mode_;
  
  void RunDecodeShow(const uint8_t sweep_digits[10], unsigned long sweep_step_ms);
  void ShowDigits(const uint8_t digits[6]);
  void ScheduleNextClean();
  void AutoCleanTube();
  void StartCleanTube();
  void UpdateCleanTube();
  void ShowCleanStep();
  void ShowCurrentTime();
  void CheckIfExecuteCleanTubeMode();
  void SetRealTimeClock();
  void ProcessSettingTime();
  void ProcessCleanTube();
  void CheckIfExecuteSettingTimeMode();
  void SwitchSettingTimeUnit();
  void ProcessAddButtonInSetting();
  void BlinkTube();
  void AdjustTime();
  void SetTimeUnit(const TimeUnit unit, const int now);
  bool WriteRtcVerified(const Ds1302::DateTime& time);
  void ApplyDriftCompensation(Ds1302::DateTime* now);
  void PrintDriftRate();
  // fade_ticks: 數字有變的管子淡入淡出的速度 (TubeDriver::kFadeNormal 等), 預設立刻切換
  void SetTime(const Ds1302::DateTime& now, uint8_t fade_ticks = TubeDriver::kFadeInstant);

 public:
  NixieTubeClock();
  void Setup();
  void CheckButtonStatus();
  void UpdateTimeIfNextSecond();
  void HandleSerial();  // USB 對時
};

#endif // NIXIE_TUBE_CLOCK_INO