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
  unsigned long last_clean_tube_time_millsecond_;
  unsigned long target_time_seconds_ = 0;  // 目標時間，單位為秒

  //*************************************按鈕*****************************
  Button back_set_btn_;   // A1: 長按3秒進入設定模式 / 設定模式下切換時分秒
  Button front_add_btn_;  // A2: 設定模式下加一
  bool back_btn_used_in_combo_ = false;   // 兩顆一起按過(清理管子)，放開前不進入設定模式
  bool manual_clean_triggered_ = false;   // 同時長按只觸發一次清理
  unsigned long last_setting_activity_millsecond_ = 0;  // 設定模式下最後一次按鍵時間
  //*************************************清理管子*************************
  bool cleaning_ = false;
  uint8_t clean_step_ = 0;
  float clean_step_delay_millsecond_ = 0;
  unsigned long clean_step_start_millsecond_ = 0;
  //**********************************************************************
  unsigned long last_blink_millsecond_ = 0;
  unsigned long last_repeat_millsecond_ = 0;  // 按住連加: 上一次 +1 的時間

  ClockMode current_mode_;
  
  void RandomizeNextCleaningTime();
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
  void SetTime(const Ds1302::DateTime& now);

 public:
  NixieTubeClock();
  void Setup();
  void CheckButtonStatus();
  void UpdateTimeIfNextSecond();
};

#endif // NIXIE_TUBE_CLOCK_INO