#include <Arduino.h>

#include "nixie_tube_clock.h"

#include "colon_led.h"
#include "fast_tick.h"
#include "time_sync.h"
#include "tube_driver.h"

namespace{
  const int button_input_mode_back_pin_ = A1;//不能只寫'1'
  const int button_input_add_front_pin_ = A2;
  const int led_output_pin_ = 12;/*與74HC595 小時的'大平台'共用*/

  const unsigned long kEnterSettingHoldMs = 2000;   // 長按 back 2 秒進入設定模式
  const unsigned long kManualCleanHoldMs = 1000;    // 兩顆一起長按 1 秒清理管子
  const unsigned long kSettingTimeoutMs = 8000;     // 設定模式 8 秒沒按鍵就回到正常模式

  // 設定模式按住 front 連續加 (跟鍵盤按住連打一樣): 按下先 +1, 按住 0.5 秒開始連加,
  // 一開始每 200ms 加 1, 之後每加一次間隔縮短 15%, 按住約 1.3 秒後到最快每 50ms 加 1 (每秒 20 下), 放開就停
  const unsigned long kRepeatDelayMs = 500;
  const unsigned long kRepeatStartIntervalMs = 200;
  const unsigned long kRepeatMinIntervalMs = 50;
  const uint8_t kRepeatSpeedUpPercent = 85;  // 下一次間隔 = 這一次 x 85%

  // 設定模式閃爍: 一個週期 400ms, 亮 240ms 暗 160ms (約每秒閃 2.5 次)
  const unsigned long kBlinkPeriodMs = 400;
  const unsigned long kBlinkOnMs = 240;
  // 自動清管 (防陰極中毒): 每隔 15~60 秒 (隨機) 把每個數字快速亮過兩輪
  // 每步 70ms 起、每步快 1ms -> 一次約 1.2 秒 (原本 25 步約 2.2 秒, 而且前 5 個數字多亮一輪)
  const unsigned long kAutoCleanMinSeconds = 15;
  const unsigned long kAutoCleanMaxSeconds = 60;
  const uint8_t kTubeCleanSteps = 20;            // 10 個數字 x 2 輪, 每個數字亮的時間一樣
  const float kCleanFirstStepMs = 70;
  const float kCleanStepSpeedUpMs = 1.0;
  //由管子的後到前顯示數字
  const byte kTubeShowNumbers[10] = {11, 66, 22, 55, 0, 44, 77, 99, 88, 33};

  // 解碼秀 (開機一次、之後每到整點一次), 兼開機自我檢查與清管:
  //  1. 掃描: 6 支管子一起把 0~9 每個數字亮一次 (保證每個陰極都亮到 = 清管; 開機時也方便看哪個數字不亮)
  //  2. 解碼: 亂數狂跳, 再從左到右一支一支鎖定成現在時間
  //  3. 閃斷: 整排熄滅/亮起兩下, 像舊螢幕開機
  // 要慢慢檢查每個數字, 把 kBootSweepStepMs 改成 1500 即可
  const bool kBootAnimation = true;
  const unsigned long kBootSweepStepMs = 105;    // 開機掃描: 每個數字停留時間 (0 -> 9 順序, 方便除錯)
  const uint8_t kBootSweepDigits[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

  // 整點報時: 每隔這麼多分鐘 (在 0 秒那一刻) 跑一次解碼秀, 60 = 只在整點
  // 取代原本每 15~30 秒的自動清管: 白天不會一直被打斷, 每個數字還是定期亮到
  const bool kChime = false;  // 已經有每 15~60 秒的自動清管, 預設關閉; 想要整點報時改 true
  const uint8_t kChimeEveryMinutes = 60;
  const unsigned long kChimeSweepStepMs = 80;    // 整點掃描用管子由後到前的順序, 像一層層往前推
  const uint8_t kChimeSweepDigits[10] = {1, 6, 2, 5, 0, 4, 7, 9, 8, 3};
  const unsigned long kBootScrambleMs = 35;     // 解碼: 亂數每幾 ms 換一次
  const uint8_t kBootFramesPerLock = 4;         // 解碼: 亂跳幾次後鎖定下一支管子
  const unsigned long kBootGlitchMs = 50;       // 閃斷: 每次熄滅/亮起的時間
  const int kRandomSeedPin = A7;                // Nano 的 A7 沒接東西, 讀到的雜訊拿來當亂數種子

  // 冒號 LED 呼吸燈 (軟體 PWM, 見 colon_led.h); false = 回到原本亮 1 秒暗 1 秒
  const bool kColonBreathing = true;
  // 換數字時淡入淡出 (見 tube_driver.h, 速度調 kFadeNormal / kFadeQuick); false = 原本的立刻切換
  const bool kDigitFade = true;
  const uint8_t kTimeFade = kDigitFade ? TubeDriver::kFadeNormal : TubeDriver::kFadeInstant;
  const uint8_t kCleanFade = kDigitFade ? TubeDriver::kFadeQuick : TubeDriver::kFadeInstant;

  // 詳細診斷日誌 ([ADD]/[SHOW] 印出 RTC 讀回值與送進 595 的位元)。
  // 9600 baud 一秒只能送約 960 個字, 打開時按住快速連加會被序列埠拖慢, 平常保持 false
  const bool kVerboseLog = false;
  const uint8_t kRtcWriteTries = 3;  // 寫 RTC 後讀回不一致就重寫, 最多寫這麼多次
  const unsigned long kSyncDateShowMs = 1200;  // USB 對時成功: 先顯示日期 (YY MM DD) 這麼久, 再解碼成時間

  const char* const kUnitNames[3] = {"second", "minute", "hour"};

  TimeUnit UnitOfMode(ClockMode mode){
    return mode == SettingHour ? Hour : (mode == SettingMinute ? Minut : Second);
  }

  uint8_t& FieldOf(Ds1302::DateTime& t, TimeUnit unit){
    return unit == Hour ? t.hour : (unit == Minut ? t.minute : t.second);
  }

  // 把送進 74HC595 的 byte 印成 "Q7..Q0=0001 0001 (ones=1 tens=1)"
  // 高 4 位 (Q7..Q4) 接個位數 K155ID1 的 D C B A, 低 4 位 (Q3..Q0) 接十位數; 15 = 不亮
  String Describe595(int data){
    if (data < 0) {
      return "595 not sent yet";
    }
    String s = "595 Q7..Q0=";
    for (int8_t b = 7; b >= 0; --b) {
      s += ((data >> b) & 1) ? '1' : '0';
      if (b == 4) {
        s += ' ';
      }
    }
    return s + " (ones=" + String((data >> 4) & 0x0F) + " tens=" + String(data & 0x0F) + ")";
  }
}


NixieTubeClock::NixieTubeClock()
  : real_time_clock_(Ds1302(4/*enable_pin*/, 2/*clock_pin*/, 3/*data_pin*/)), 
    time_registers_{ ShiftRegister(Second, 6, 7, 5)/*秒*/,
      ShiftRegister(Minut, 9, 10, 8)/*分*/, ShiftRegister(Hour, 12, 13, 11)/*時*/ },
    back_set_btn_(button_input_mode_back_pin_),
    front_add_btn_(button_input_add_front_pin_),
    current_mode_(ClockMode::Normal){

    SetRealTimeClock();
}

void NixieTubeClock::Setup(){
  back_set_btn_.Begin();
  front_add_btn_.Begin();
  // 解碼秀的亂數每次開機都不一樣
  randomSeed(analogRead(kRandomSeedPin));

  // 從這裡開始, 送給 595 的資料都由 Timer2 中斷送出 (淡入淡出要準時切換)
  uint8_t latch_pins[TubeDriver::kRegisters];
  uint8_t clock_pins[TubeDriver::kRegisters];
  uint8_t data_pins[TubeDriver::kRegisters];
  for (uint8_t i = 0; i < TubeDriver::kRegisters; ++i) {
    latch_pins[i] = time_registers_[i].latch_pin();
    clock_pins[i] = time_registers_[i].clock_pin();
    data_pins[i] = time_registers_[i].data_pin();
  }
  TubeDriver::Begin(latch_pins, clock_pins, data_pins);
  if (kColonBreathing) {
    ColonLed::Begin(led_output_pin_);
  }
  FastTick::Begin();

  if (kBootAnimation) {
    Serial.println("[BOOT]");
    RunDecodeShow(kBootSweepDigits, kBootSweepStepMs);
  }
  ScheduleNextClean();
}

// 6 支管子各自的數字 (由左到右: 時十 時個 分十 分個 秒十 秒個); 255 = 該支不亮
void NixieTubeClock::ShowDigits(const uint8_t digits[6]){
  const TimeUnit units[3] = {Hour, Minut, Second};
  for (uint8_t pair = 0; pair < 3; ++pair) {
    const uint8_t tens = digits[pair * 2];
    const uint8_t ones = digits[pair * 2 + 1];
    time_registers_[units[pair]].SetClockTime(tens > 9 || ones > 9 ? 255 : tens * 10 + ones);
  }
}

// 解碼秀 (說明見最上面的 kBootAnimation); 約 1.5 秒, 期間不讀按鈕
// sweep_digits = nullptr 時跳過掃描, 直接解碼
void NixieTubeClock::RunDecodeShow(const uint8_t sweep_digits[10], unsigned long sweep_step_ms){
  uint8_t digits[6];

  // 1. 掃描: 每支管子的 0~9 都亮過一次
  for (uint8_t i = 0; sweep_digits != nullptr && i < 10; ++i) {
    memset(digits, sweep_digits[i], sizeof(digits));
    ShowDigits(digits);
    delay(sweep_step_ms);
  }

  // 2. 解碼: 還沒鎖定的管子亂跳, 由左到右一支一支鎖定成現在時間
  Ds1302::DateTime now;
  real_time_clock_.getDateTime(&now);
  const uint8_t target[6] = {
    (uint8_t)(now.hour / 10), (uint8_t)(now.hour % 10),
    (uint8_t)(now.minute / 10), (uint8_t)(now.minute % 10),
    (uint8_t)(now.second / 10), (uint8_t)(now.second % 10)};
  for (uint8_t locked = 0; locked < 6; ++locked) {
    for (uint8_t frame = 0; frame < kBootFramesPerLock; ++frame) {
      for (uint8_t i = 0; i < 6; ++i) {
        digits[i] = i < locked ? target[i] : random(10);
      }
      ShowDigits(digits);
      delay(kBootScrambleMs);
    }
  }

  // 3. 閃斷: 熄 / 亮 / 熄 / 亮
  for (uint8_t i = 0; i < 2; ++i) {
    memset(digits, 255, sizeof(digits));
    ShowDigits(digits);
    delay(kBootGlitchMs);
    ShowDigits(target);
    delay(kBootGlitchMs);
  }

  ShowCurrentTime();  // 動畫期間秒數可能已經跳了, 以 RTC 為準
}

// USB 對時 (指令格式見 time_sync.h, 電腦端用 tools/sync_time.bat)
void NixieTubeClock::HandleSerial(){
  Ds1302::DateTime received;
  const TimeSync::Command command = TimeSync::Poll(&received);
  if (command == TimeSync::kNone) {
    return;
  }
  char text[20];
  Ds1302::DateTime clock_time;
  real_time_clock_.getDateTime(&clock_time);

  if (command == TimeSync::kQuery) {
    TimeSync::Format(clock_time, text, sizeof(text));
    Serial.print("NOW ");
    Serial.println(text);
    return;
  }
  if (command == TimeSync::kBad) {
    Serial.println("ERR format, expected: T2026-11-18 10:00:00");
    return;
  }

  // 對時前時鐘快 (+) 或慢 (-) 了幾秒, 印出來就知道 DS1302 這段時間跑掉多少
  const long drift = TimeSync::DiffSeconds(clock_time, received);
  if (!WriteRtcVerified(received)) {
    Serial.println("ERR RTC write failed");
    return;
  }
  TimeSync::Format(received, text, sizeof(text));
  Serial.print("OK ");
  Serial.print(text);
  Serial.print(" drift ");
  if (drift > 0) {
    Serial.print('+');
  }
  Serial.println(drift);

  // 讓人看得出來剛對完時: 跳出設定模式/清管, 先亮日期, 再解碼鎖定成新時間
  current_mode_ = ClockMode::Normal;
  cleaning_ = false;
  const uint8_t date[6] = {
    (uint8_t)(received.year / 10), (uint8_t)(received.year % 10),
    (uint8_t)(received.month / 10), (uint8_t)(received.month % 10),
    (uint8_t)(received.day / 10), (uint8_t)(received.day % 10)};
  ShowDigits(date);
  delay(kSyncDateShowMs);
  RunDecodeShow(nullptr, 0);
  ScheduleNextClean();
}

// 寫進 RTC 後讀回確認 (秒數允許差 1, 寫完剛好跳秒), 不一致就重寫
bool NixieTubeClock::WriteRtcVerified(const Ds1302::DateTime& time){
  Ds1302::DateTime writable = time;
  for (uint8_t attempt = 1; attempt <= kRtcWriteTries; ++attempt) {
    real_time_clock_.setDateTime(&writable);
    Ds1302::DateTime check;
    real_time_clock_.getDateTime(&check);
    const long diff = TimeSync::DiffSeconds(check, time);
    if (diff >= 0 && diff <= 1) {
      return true;
    }
  }
  return false;
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

void NixieTubeClock::ProcessCleanTube(){
  CheckIfExecuteCleanTubeMode();//手動清輝光管
  AutoCleanTube();//每 15~60 秒自動清管
  UpdateCleanTube();//非阻塞地推進清理動畫
}

void NixieTubeClock::UpdateTimeIfNextSecond(){
  static uint8_t last_second_ = 0;
  Ds1302::DateTime now;
  real_time_clock_.getDateTime(&now);// get the current time
  const bool new_second = last_second_ != now.second;
  if (kColonBreathing) {
    // 偶數秒開始吸氣、奇數秒開始吐氣, 呼吸跟著秒數走, 不會跟時鐘慢慢錯開
    if (new_second && now.second % 2 == 0) {
      breath_start_millsecond_ = millis();
    }
    ColonLed::Update(millis() - breath_start_millsecond_);
  }
  if (!new_second){
    return;
  }
  if (!kColonBreathing) {
    static bool current_led_turn_on_ = false;
    digitalWrite(led_output_pin_, current_led_turn_on_ ? HIGH : LOW);  // 冒號 LED 每秒亮暗切換
    current_led_turn_on_ = !current_led_turn_on_;
  }
  last_second_ = now.second;

  // 每秒一行, 補零對齊: "10:00:06", 設定/清理中在後面標註狀態 (原本另外印 ON/OFF 兩行很雜)
  const char* tag = "";
  if (cleaning_) {
    tag = "  [clean]";
  } else if (current_mode_ == SettingSecond) {
    tag = "  [set second]";
  } else if (current_mode_ == SettingMinute) {
    tag = "  [set minute]";
  } else if (current_mode_ == SettingHour) {
    tag = "  [set hour]";
  }
  char line[28];
  snprintf(line, sizeof(line), "%02u:%02u:%02u%s", now.hour, now.minute, now.second, tag);
  Serial.println(line);

  if(current_mode_ != ClockMode::Normal || cleaning_){
    return;//設定模式或清理管子中, 不覆蓋顯示
  }
  if (kChime && now.second == 0 && now.minute % kChimeEveryMinutes == 0) {
    Serial.println("[CHIME]");
    RunDecodeShow(kChimeSweepDigits, kChimeSweepStepMs);  // 最後會自己顯示現在時間
    return;
  }
  SetTime(now, kTimeFade);
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

// 清理管子改成狀態機: 原本 CleanTube() 用 delay() 卡住約 2.2 秒, 期間按鈕完全讀不到
void NixieTubeClock::ScheduleNextClean(){
  last_clean_tube_time_millsecond_ = millis();
  next_clean_after_seconds_ = random(kAutoCleanMinSeconds, kAutoCleanMaxSeconds + 1);
  Serial.print("[CLEAN] next in ");
  Serial.print(next_clean_after_seconds_);
  Serial.println(" s");
}

void NixieTubeClock::AutoCleanTube(){
  if(current_mode_ != ClockMode::Normal || cleaning_){
    return;//只在正常模式下清理管子
  }
  if((millis() - last_clean_tube_time_millsecond_) / 1000 >= next_clean_after_seconds_){
    StartCleanTube();
  }
}

void NixieTubeClock::StartCleanTube(){
  cleaning_ = true;
  clean_step_ = 0;
  clean_step_delay_millsecond_ = kCleanFirstStepMs;
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
  clean_step_delay_millsecond_ -= kCleanStepSpeedUpMs;
  if(interrupted || clean_step_ >= kTubeCleanSteps){
    cleaning_ = false;
    ScheduleNextClean();  // 手動或自動清完都從現在重新倒數
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
  SetTime(now, kCleanFade);  // 每一步快速溶接到下一個數字, 看起來像在流動
}

void NixieTubeClock::ShowCurrentTime(){
  Ds1302::DateTime now;
  real_time_clock_.getDateTime(&now);
  SetTime(now, kTimeFade);  // 清管結束、離開設定模式時也柔和地換回時間
}


void NixieTubeClock::ProcessSettingTime(){
  if(current_mode_ == ClockMode::Normal){
    CheckIfExecuteSettingTimeMode();
    return;
  }

  if(back_set_btn_.WasPressed()){ // 只在「剛按下」的那一刻觸發一次
    SwitchSettingTimeUnit();//是否second minute hour 切換
  }
  ProcessAddButtonInSetting();
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
    last_logged_show_value_ = -1;
    last_blink_millsecond_ = millis();
    last_setting_activity_millsecond_ = millis();
  }
}

// 設定模式下的 front 按鍵: 按一下 +1, 按住越久加越快, 放開就停
void NixieTubeClock::ProcessAddButtonInSetting(){
  if(front_add_btn_.WasPressed()){
    last_setting_activity_millsecond_ = millis();//有按按鍵,更新上次按下的時間
    AdjustTime();
    last_repeat_millsecond_ = millis();
    repeat_interval_ms_ = kRepeatStartIntervalMs;  // 每次重新按下都從慢速開始
    return;
  }
  // 秒只能歸零, 不需要連加; 兩顆一起按時也不連加
  if(!front_add_btn_.IsPressed() || back_set_btn_.IsPressed() ||
     current_mode_ == ClockMode::SettingSecond){
    return;
  }
  last_setting_activity_millsecond_ = millis();//按住期間不會逾時跳回正常模式
  if(front_add_btn_.PressedDurationMs() < kRepeatDelayMs){
    return;
  }
  if(millis() - last_repeat_millsecond_ >= repeat_interval_ms_){
    last_repeat_millsecond_ = millis();
    AdjustTime();
    // 每加一次, 下一次的間隔縮短一點, 直到最快速度
    repeat_interval_ms_ = max(kRepeatMinIntervalMs,
                              repeat_interval_ms_ * kRepeatSpeedUpPercent / 100);
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

  last_logged_show_value_ = -1;
  last_blink_millsecond_ = millis();
}

//閃爍當前設定的時間; 其他沒在設定的管子永遠正常顯示
//按住 front 加數字時, 正在設定的管子保持常亮, 才看得清楚數字在跑
void NixieTubeClock::BlinkTube(){ 
  Ds1302::DateTime now;
  real_time_clock_.getDateTime(&now);
  const bool holding_add = front_add_btn_.IsPressed();
  const bool show = holding_add ||
    (millis() - last_blink_millsecond_) % kBlinkPeriodMs < kBlinkOnMs;
  if(!show){
    if (current_mode_ == SettingSecond){
      now.second = 255;
    } else if (current_mode_ == SettingMinute){
      now.minute = 255;
    } else if (current_mode_ == SettingHour){
      now.hour = 255;
    }
  }
  SetTime(now);//ShiftRegister 會略過沒變的值, 每輪呼叫不會多送資料
  if(holding_add){
    last_blink_millsecond_ = millis();//放開後從「亮」開始閃
  }
  if(kVerboseLog && show){
    // 診斷日誌: 設定中的管子實際被送了什麼數字 (數字有變才印), 拿來跟管子上看到的數字比對
    const TimeUnit unit = UnitOfMode(current_mode_);
    const uint8_t shown = FieldOf(now, unit);
    if(shown != last_logged_show_value_){
      last_logged_show_value_ = shown;
      Serial.println(String("[SHOW] ") + kUnitNames[unit] + " tube <- " + String(shown) +
                     ", " + Describe595(time_registers_[unit].LastData()));
    }
  }
}

void NixieTubeClock::AdjustTime(){
  Ds1302::DateTime now;
  real_time_clock_.getDateTime(&now);
  const TimeUnit unit = UnitOfMode(current_mode_);
  uint8_t& field = FieldOf(now, unit);
  const uint8_t before = field;
  if (unit == Second) {
    field = 0; // 秒只能歸零
  } else if (unit == Minut) {
    field = (field + 1) % 60; // 分鐘加一
  } else {
    field = (field + 1) % 24; // 時數加一
  }
  const uint8_t target = field;
  SetTimeUnit(unit, target);

  // 寫進 RTC 後馬上讀回來確認; 不一致就重寫, 日誌會留下紀錄
  uint8_t readback = 0;
  for (uint8_t attempt = 1; attempt <= kRtcWriteTries; ++attempt) {
    real_time_clock_.setDateTime(&now);
    Ds1302::DateTime check;
    real_time_clock_.getDateTime(&check);
    readback = FieldOf(check, unit);
    if (readback == target) {
      break;
    }
    Serial.println("[ADD] RTC write #" + String(attempt) + " read back " + String(readback) +
                   ", expected " + String(target));
  }

  if (kVerboseLog) {
    Serial.println(String("[ADD] ") + kUnitNames[unit] + ": " + String(before) + " -> " +
                   String(target) + ", RTC readback=" + String(readback) + ", " +
                   Describe595(time_registers_[unit].LastData()));
  } else {
    // 短短一行, 按住每秒加 20 下時序列埠也來得及送
    Serial.print(kUnitNames[unit]);
    Serial.print('=');
    Serial.println(target);
  }
}

void NixieTubeClock::SetTimeUnit(const TimeUnit unit, const int now){
  time_registers_[unit].SetClockTime(now);
}

void NixieTubeClock::SetTime(const Ds1302::DateTime& now, uint8_t fade_ticks){
  time_registers_[TimeUnit::Second].SetClockTime(now.second, fade_ticks);
  time_registers_[TimeUnit::Minut].SetClockTime(now.minute, fade_ticks);
  time_registers_[TimeUnit::Hour].SetClockTime(now.hour, fade_ticks);
}
