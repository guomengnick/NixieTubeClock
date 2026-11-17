#include <Arduino.h>

#include "nixie_tube_clock.h"

namespace{
  const int button_input_mode_back_pin_ = A1;//不能只寫'1'
  const int button_input_add_front_pin_ = A2;
  const int led_output_pin_ = 12;/*與74HC595 小時的'大平台'共用*/

  const unsigned long kEnterSettingHoldMs = 3000;   // 長按 back 3 秒進入設定模式
  const unsigned long kManualCleanHoldMs = 1000;    // 兩顆一起長按 1 秒清理管子
  const unsigned long kSettingTimeoutMs = 8000;     // 設定模式 8 秒沒按鍵就回到正常模式
  const uint8_t kTubeCleanSteps = 25;
  //由管子的後到前顯示數字
  const byte kTubeShowNumbers[10] = {11, 66, 22, 55, 0, 44, 77, 99, 88, 33};
}


NixieTubeClock::NixieTubeClock()
  : real_time_clock_(Ds1302(4/*enable_pin*/, 2/*clock_pin*/, 3/*data_pin*/)), 
    time_registers_{ ShiftRegister(6, 7, 5)/*秒*/, 
      ShiftRegister(9, 10, 8)/*分*/, ShiftRegister(12, 13, 11)/*時*/ },
    last_clean_tube_time_millsecond_(millis()),
    back_set_btn_(button_input_mode_back_pin_),
    front_add_btn_(button_input_add_front_pin_),
    current_mode_(ClockMode::Normal){
        
    RandomizeNextCleaningTime();
    SetRealTimeClock();
}

void NixieTubeClock::Setup(){
  back_set_btn_.Begin();
  front_add_btn_.Begin();
}

// 每個 loop 只讀一次按鈕，所有邏輯都用同一份防抖後的狀態，且不使用 delay()
void NixieTubeClock::CheckButtonStatus(){
  back_set_btn_.Update();
  front_add_btn_.Update();

  // back 放開前只要 front 也按過，就當作是「兩顆一起按」，不會誤進設定模式
  if (!back_set_btn_.IsPressed()) {
    back_btn_used_in_combo_ = false;
  } else if (front_add_btn_.IsPressed()) {
    back_btn_used_in_combo_ = true;
  }

  ProcessCleanTube();
  ProcessSettingTime();
}

void NixieTubeClock::RandomizeNextCleaningTime(){// 隨機選擇15至 30 秒 的時間
  target_time_seconds_ = random(15, 30);
  Serial.println("target_time_seconds_ : " + String(target_time_seconds_));
}

void NixieTubeClock::ProcessCleanTube(){
  CheckIfExecuteCleanTubeMode();//手動清輝光管
  AutoCleanTube();//固定時間清輝光管
  UpdateCleanTube();//非阻塞地推進清理動畫
}

void NixieTubeClock::UpdateTimeIfNextSecond(){
  static uint8_t last_second_ = 0;
  Ds1302::DateTime now;
  real_time_clock_.getDateTime(&now);// get the current time
  if (last_second_ == now.second){
    return;
  }
  static bool current_led_turn_on_ = false;
  if(current_led_turn_on_){
  Serial.println("ON");

    digitalWrite(led_output_pin_, HIGH);
  }
  else{
  Serial.println("OFF");
    digitalWrite(led_output_pin_, LOW);
  }
  current_led_turn_on_ = !current_led_turn_on_;

  last_second_ = now.second;
  if(current_mode_ != ClockMode::Normal || cleaning_){
    return;//設定模式或清理管子中, 不覆蓋顯示
  }
  
  SetTime(now);
  Serial.println(String(now.hour) + ":" + String(now.minute) + ":" + String(now.second));

}

void NixieTubeClock::SetRealTimeClock(){
  real_time_clock_.init();
  if (real_time_clock_.isHalted()){// if is halted, set a date-time
    Serial.println("RTC is halted. Setting time...");
    Ds1302::DateTime dt = {.year = 24, .month = Ds1302::MONTH_OCT,
      .day = 3,.hour = 2, .minute = 2, .second = 22, .dow = Ds1302::DOW_TUE};
    real_time_clock_.setDateTime(&dt);
  }
}

void NixieTubeClock::CheckIfExecuteCleanTubeMode(){
  const bool both_pressed = back_set_btn_.IsPressed() && front_add_btn_.IsPressed();
  if (!both_pressed) {
    manual_clean_triggered_ = false; // 放開後才能再觸發下一次
    return;
  }
  if (manual_clean_triggered_ || current_mode_ != ClockMode::Normal) {
    return;
  }
  // 兩顆都已穩定按住超過1秒
  if (back_set_btn_.PressedDurationMs() > kManualCleanHoldMs &&
      front_add_btn_.PressedDurationMs() > kManualCleanHoldMs) {
    manual_clean_triggered_ = true;
    Serial.println("Manual clean tube");
    StartCleanTube();
  }
}

void NixieTubeClock::AutoCleanTube(){
  if(current_mode_ != ClockMode::Normal || cleaning_){
    return;//只在正常模式下清理管子
  }
  const unsigned long time_diff_seconds =
    (millis() - last_clean_tube_time_millsecond_) / 1000/*轉成秒*/;
  // 檢查時間差是否達到清理時間
  if (time_diff_seconds >= target_time_seconds_){
    StartCleanTube();
  }
}

// 清理管子改成狀態機: 原本 CleanTube() 用 delay() 卡住約 2.2 秒, 期間按鈕完全讀不到
void NixieTubeClock::StartCleanTube(){
  cleaning_ = true;
  clean_step_ = 0;
  clean_step_delay_millsecond_ = 100;
  ShowCleanStep();
  clean_step_start_millsecond_ = millis();
}

void NixieTubeClock::UpdateCleanTube(){
  if(!cleaning_){
    return;
  }
  const bool interrupted = current_mode_ != ClockMode::Normal; // 清理中進入設定模式就停止
  if(!interrupted &&
     millis() - clean_step_start_millsecond_ < (unsigned long)clean_step_delay_millsecond_){
    return;
  }
  clean_step_++;
  clean_step_delay_millsecond_ -= 0.9;
  if(interrupted || clean_step_ >= kTubeCleanSteps){
    cleaning_ = false;
    last_clean_tube_time_millsecond_ = millis();
    RandomizeNextCleaningTime();
    if(!interrupted){
      ShowCurrentTime();//清理完管子後, 馬上顯示當前時間
    }
    return;
  }
  ShowCleanStep();
  clean_step_start_millsecond_ = millis();
}

void NixieTubeClock::ShowCleanStep(){
  const int show_num = kTubeShowNumbers[clean_step_ % 10];
  Ds1302::DateTime now;
  now.second = show_num;
  now.minute = show_num;
  now.hour = show_num;
  SetTime(now);
}

void NixieTubeClock::ShowCurrentTime(){
  Ds1302::DateTime now;
  real_time_clock_.getDateTime(&now);
  SetTime(now);
}


void NixieTubeClock::ProcessSettingTime(){
  if(current_mode_ == ClockMode::Normal){
    CheckIfExecuteSettingTimeMode();
    return;
  }

  if(back_set_btn_.WasPressed()){ // 只在「剛按下」的那一刻觸發一次
    SwitchSettingTimeUnit();//是否second minute hour 切換
  }
  if(front_add_btn_.WasPressed()){
    last_setting_activity_millsecond_ = millis();//有按按鍵,更新上次按下的時間
    AdjustTime();
  }
  // 經過８秒沒按任何按鍵, 就變成正常模式
  if(millis() - last_setting_activity_millsecond_ > kSettingTimeoutMs){
    current_mode_ = ClockMode::Normal;
    Serial.println("Set back to normal");
    ShowCurrentTime();
    return;
  }
  BlinkTube();//閃爍當前設定的管子
}


void NixieTubeClock::CheckIfExecuteSettingTimeMode(){
  // 如果在正常模式下長按back_button 且add按鈕沒按(清理模式會按著2顆按鈕)，則進入設定模式
  if (!back_set_btn_.IsPressed() || front_add_btn_.IsPressed() || back_btn_used_in_combo_){
    return;
  }
  if (back_set_btn_.PressedDurationMs() > kEnterSettingHoldMs){ // 長按超過3秒
    Serial.println("Entering setting second mode");
    current_mode_ = ClockMode::SettingSecond; // 進入設定秒數模式
    last_blink_millsecond_ = millis();
    last_setting_activity_millsecond_ = millis();
  }
}

// 在設定模式下切換設定項目, second->minute->hour->second...
void NixieTubeClock::SwitchSettingTimeUnit(){
  last_setting_activity_millsecond_ = millis();

  // 使用單一語句循環切換模式
  current_mode_ = static_cast<ClockMode>((current_mode_ + 1 - SettingSecond) % 3 + SettingSecond);

  if (current_mode_ == SettingSecond){
    Serial.println("Switch to set second mode");
  }
  else if (current_mode_ == SettingMinute){
    Serial.println("Switch to set minute mode");
  } 
  else if (current_mode_ == SettingHour){
    Serial.println("Switch to set hour mode");
  } 

  last_blink_millsecond_ = millis();
}

//閃爍當前設定的時間, 但當切換的瞬間, 需要將上一個快速顯示回去, 不能不顯示
void NixieTubeClock::BlinkTube(){ 
  unsigned long current_time = millis();
  unsigned long current_off_time_millisecond_ = 
    (current_time - last_blink_millsecond_);
  Ds1302::DateTime now;
  real_time_clock_.getDateTime(&now);
  if(current_off_time_millisecond_ % 1000 > 500){
    if (current_mode_ == SettingSecond){
      SetTimeUnit(TimeUnit::Second, now.second);
    } else if (current_mode_ == SettingMinute){
      SetTimeUnit(TimeUnit::Minut, now.minute);
    } else if (current_mode_ == SettingHour){
      SetTimeUnit(TimeUnit::Hour, now.hour);
    }
  }
  else{
    if (current_mode_ == SettingSecond){
      now.second = 255;
      SetTime(now);
    } else if (current_mode_ == SettingMinute){
      now.minute = 255;
      SetTime(now);
    } else if (current_mode_ == SettingHour){
      now.hour = 255;
      SetTime(now);
    }
  }
}

void NixieTubeClock::AdjustTime(){
  Ds1302::DateTime now;
  real_time_clock_.getDateTime(&now);
  if (current_mode_ == SettingSecond) {
    now.second = 0; // 將秒歸零
    SetTimeUnit(TimeUnit::Second, now.second);
    Serial.println("Set minut to: 0");
  } else if (current_mode_ == SettingMinute) {
    now.minute = (now.minute + 1) % 60; // 分鐘加一
    SetTimeUnit(TimeUnit::Minut, now.minute);
    Serial.println("Set minut to: " + String(now.minute));
  } else if (current_mode_ == SettingHour) {
    now.hour = (now.hour + 1) % 24; // 時數加一
    SetTimeUnit(TimeUnit::Hour, now.hour);
    Serial.println("Set hour to: " + String(now.hour));
  }

  real_time_clock_.setDateTime(&now); // 更新時間
}

void NixieTubeClock::SetTimeUnit(const TimeUnit unit, const int now){
  time_registers_[unit].SetClockTime(now);
}

void NixieTubeClock::SetTime(const Ds1302::DateTime& now){
  time_registers_[TimeUnit::Second].SetClockTime(now.second);
  time_registers_[TimeUnit::Minut].SetClockTime(now.minute);
  time_registers_[TimeUnit::Hour].SetClockTime(now.hour);
}
