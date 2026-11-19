#include "nixie_tube_clock.h"

#include <EEPROM.h>

#include "colon_led.h"
#include "config.h"
#include "drift_comp.h"
#include "fast_tick.h"
#include "time_sync.h"
#include "tube_driver.h"

namespace {
  const char* const kUnitNames[3] = {"second", "minute", "hour"};  // 順序 = TimeUnit

  uint8_t& FieldOf(Ds1302::DateTime& t, TimeUnit unit) {
    return unit == kHour ? t.hour : (unit == kMinute ? t.minute : t.second);
  }

  // 把送進 74HC595 的 byte 印成 "595 Q7..Q0=0001 0001 (ones=1 tens=1)" 並換行
  // 高 4 位 (Q7..Q4) 接個位數 K155ID1 的 D C B A, 低 4 位 (Q3..Q0) 接十位數; 15 = 不亮
  // (直接 print, 不用 String 串接: Nano 只有 2KB RAM, String 反覆配置會讓記憶體碎掉)
  void Print595Line(int data) {
    if (data < 0) {
      Serial.println(F("595 not sent yet"));
      return;
    }
    Serial.print(F("595 Q7..Q0="));
    for (int8_t b = 7; b >= 0; --b) {
      Serial.print(((data >> b) & 1) ? '1' : '0');
      if (b == 4) {
        Serial.print(' ');
      }
    }
    Serial.print(F(" (ones="));
    Serial.print((data >> 4) & 0x0F);
    Serial.print(F(" tens="));
    Serial.print(data & 0x0F);
    Serial.println(')');
  }
}

NixieTubeClock::NixieTubeClock()
  : rtc_(kRtcEnablePin, kRtcClockPin, kRtcDataPin),
    buttons_(kBackButtonPin, kFrontButtonPin, kEnterBrightnessHoldMs, kEnterSettingHoldMs,
             kManualCleanHoldMs) {
  // 這裡不碰硬體: 全域物件的建構子比 Arduino 的 init() 和 setup() 都早跑,
  // 這時候 Serial 還沒開, 印出來的字會直接不見
}

void NixieTubeClock::Setup() {
  InitRtc();
  buttons_.Begin();
  randomSeed(analogRead(kRandomSeedPin));  // 解碼秀、清管間隔的亂數每次開機都不一樣

  // 從這裡開始, 送給 595 的資料都由 Timer2 中斷送出 (淡入淡出要準時切換)
  display_.Begin();
  if (kColonBreathing) {
    ColonLed::Begin(kColonLedPin);
  } else {
    pinMode(kColonLedPin, OUTPUT);
  }
  FastTick::Begin();
  LoadBrightness();

  DriftComp::Begin();
  if (rtc_was_reset_) {
    DriftComp::Forget();
  }
  PrintDriftRate();
  if (kDriftCompensation) {
    // 關機期間 RTC 靠電池照樣跑快/跑慢, 開機時一次補回來。
    // 先等到跳秒那一刻再寫, 不會吃掉這一秒已經走掉的零頭
    Ds1302::DateTime now;
    rtc_.getDateTime(&now);
    if (DriftComp::DueSeconds(TimeSync::ToEpoch(now)) != 0) {
      const uint8_t second = now.second;
      const unsigned long wait_start = millis();
      while (now.second == second && millis() - wait_start < 1100) {
        rtc_.getDateTime(&now);
      }
      ApplyDriftCompensation(&now);
    }
  }

  if (kBootAnimation) {
    Serial.println(F("[BOOT]"));
    RunDecodeShow(display_, rtc_, kBootSweepDigits, kBootSweepStepMs);
  }
  rtc_.getDateTime(&now_);
  EnterState(State::kTime);
  cleaner_.ScheduleNext();
}

void NixieTubeClock::Loop() {
  buttons_.Update();
  ReadClock();
  HandleSerial();
  HandleInput();
  UpdateState();
  if (new_second_) {
    PrintSecondLine();
  }
}

//******************************** 狀態機 ********************************

void NixieTubeClock::EnterState(State next) {
  const State prev = state_;

  // ---- 離開舊狀態 ----
  if (prev == State::kCleaning) {
    cleaner_.ScheduleNext();  // 清完或被打斷, 都從現在重新倒數
  }
  if (IsSettingTime(prev) && !IsSettingTime(next)) {
    FinishTimeSetting();
  }

  state_ = next;
  state_start_ms_ = millis();
  last_activity_ms_ = millis();

  // ---- 進入新狀態 ----
  switch (next) {
    case State::kTime:
      display_.ShowTime(now_, kTimeFade);  // 從別的畫面柔和地換回時間
      break;
    case State::kRolling:
      break;  // roll_.Start() 在決定要走的時候已經做好
    case State::kCleaning:
      Serial.println(F("[MODE] clean"));
      cleaner_.Start(display_);
      break;
    case State::kDate: {
      char line[24];
      snprintf_P(line, sizeof(line), PSTR("[MODE] date 20%02u-%02u-%02u"), now_.year, now_.month, now_.day);
      Serial.println(line);
      display_.ShowDate(now_, kTimeFade);
      break;
    }
    case State::kSetSecond:
    case State::kSetMinute:
    case State::kSetHour:
      Serial.print(F("[MODE] set "));
      Serial.println(kUnitNames[SettingUnit()]);
      blink_start_ms_ = millis();
      last_logged_show_value_ = -1;
      break;
    case State::kSetBrightness:
      Serial.println(F("[MODE] set brightness"));
      ShowBrightness();
      break;
  }
}

void NixieTubeClock::HandleInput() {
  if (IsNormal(state_)) {
    // 「正常」這一組的手勢都一樣, 不管現在是走時、換字掃描、清管還是看日期
    if (buttons_.Has(ButtonInput::kBothHold)) {
      EnterState(State::kCleaning);
    } else if (buttons_.Has(ButtonInput::kBackHold)) {
      EnterState(State::kSetSecond);
    } else if (buttons_.Has(ButtonInput::kFrontHold)) {
      EnterState(State::kSetBrightness);
    } else if (buttons_.Has(ButtonInput::kFrontTap)) {
      EnterState(state_ == State::kDate ? State::kTime : State::kDate);
    }
  } else if (state_ == State::kSetBrightness) {
    HandleBrightnessInput();
  } else {
    HandleTimeSettingInput();
  }
}

void NixieTubeClock::UpdateState() {
  switch (state_) {
    case State::kTime:
      if (new_second_) {
        if (kChime && now_.second == 0 && now_.minute % kChimeEveryMinutes == 0) {
          Serial.println(F("[CHIME]"));
          RunDecodeShow(display_, rtc_, kChimeSweepDigits, kChimeSweepStepMs);
          break;
        }
        if (kRollOnMinute && now_.second == 0) {
          const uint8_t changed = ChangedTimeTubes(display_, now_);
          if (changed != 0) {
            static const LadderWalk::Style kSweepStyle = {kRollStepMs, 0, 0, kRollFade, true, false};
            roll_.Start(changed, kSweepStyle);
            EnterState(State::kRolling);  // 有變的管子走梯子, 其他照常顯示時間
            break;
          }
        }
        display_.ShowTime(now_, kTimeFade);
      }
      if (cleaner_.Due()) {
        EnterState(State::kCleaning);
      }
      break;

    case State::kRolling:
      if (!roll_.Update(display_, now_)) {
        EnterState(State::kTime);
      }
      break;

    case State::kCleaning:
      if (!cleaner_.Update(display_, now_)) {
        EnterState(State::kTime);
      }
      break;

    case State::kDate:
      if (millis() - state_start_ms_ >= kDateShowMs) {
        EnterState(State::kTime);
      }
      break;

    case State::kSetSecond:
    case State::kSetMinute:
    case State::kSetHour:
      if (millis() - last_activity_ms_ > kSettingTimeoutMs) {
        EnterState(State::kTime);
      } else {
        BlinkSettingTube();
      }
      break;

    case State::kSetBrightness:
      if (millis() - last_activity_ms_ > kBrightnessTimeoutMs) {
        EnterState(State::kTime);
      }
      break;
  }
}

//******************************** 時間、冒號、日誌 ********************************

void NixieTubeClock::ReadClock() {
  rtc_.getDateTime(&now_);
  new_second_ = now_.second != last_second_;
  UpdateColon();
  if (new_second_ && kDriftCompensation && now_.second == kDriftFixSecond && IsNormal(state_)) {
    ApplyDriftCompensation(&now_);  // 剛跳秒就撥, 不會吃掉零頭
  }
  last_second_ = now_.second;
}

void NixieTubeClock::UpdateColon() {
  if (kColonBreathing) {
    // 偶數秒開始吸氣、奇數秒開始吐氣, 呼吸跟著秒數走, 不會跟時鐘慢慢錯開
    if (new_second_ && now_.second % 2 == 0) {
      breath_start_ms_ = millis();
    }
    ColonLed::Update(millis() - breath_start_ms_);
  } else if (new_second_) {
    digitalWrite(kColonLedPin, now_.second % 2 == 0 ? HIGH : LOW);  // 亮 1 秒暗 1 秒
  }
}

// 每秒一行, 補零對齊: "10:00:06", 不是正常走時就在後面標註狀態
void NixieTubeClock::PrintSecondLine() {
  const char* tag = "";
  switch (state_) {
    case State::kTime: break;
    case State::kRolling: tag = "  [roll]"; break;
    case State::kCleaning: tag = "  [clean]"; break;
    case State::kDate: tag = "  [date]"; break;
    case State::kSetSecond: tag = "  [set second]"; break;
    case State::kSetMinute: tag = "  [set minute]"; break;
    case State::kSetHour: tag = "  [set hour]"; break;
    case State::kSetBrightness: tag = "  [set brightness]"; break;
  }
  char line[28];
  snprintf(line, sizeof(line), "%02u:%02u:%02u%s", now_.hour, now_.minute, now_.second, tag);
  Serial.println(line);
}

// USB 對時 (指令格式見 time_sync.h, 電腦端用 tools/sync_time.bat)
void NixieTubeClock::HandleSerial() {
  Ds1302::DateTime received;
  const TimeSync::Command command = TimeSync::Poll(&received);
  if (command == TimeSync::kNone) {
    return;
  }
  char text[20];
  Ds1302::DateTime clock_time;
  rtc_.getDateTime(&clock_time);  // 重讀一次: 誤差要算到這一刻

  if (command == TimeSync::kQuery) {
    TimeSync::Format(clock_time, text, sizeof(text));
    Serial.print(F("NOW "));
    Serial.println(text);
    return;
  }
  if (command == TimeSync::kBad) {
    Serial.println(F("ERR format, expected: T2026-11-18 10:00:00"));
    return;
  }

  // 對時前時鐘快 (+) 或慢 (-) 了幾秒, 印出來就知道 DS1302 這段時間跑掉多少
  const long drift = TimeSync::DiffSeconds(clock_time, received);
  if (!WriteRtcVerified(received)) {
    Serial.println(F("ERR RTC write failed"));
    return;
  }
  const DriftComp::SyncInfo info = DriftComp::OnSync(TimeSync::ToEpoch(received), drift);
  // 一行給電腦端腳本解析: OK <時間> drift <秒> comp <已自動修正秒> raw <沒補償會差幾秒>
  //                       rate <ppb|none> learn <這輪學了幾秒>
  TimeSync::Format(received, text, sizeof(text));
  Serial.print(F("OK "));
  Serial.print(text);
  Serial.print(F(" drift "));
  Serial.print(drift);
  Serial.print(F(" comp "));
  Serial.print(info.applied);
  Serial.print(F(" raw "));
  Serial.print(info.raw_drift);
  Serial.print(F(" rate "));
  if (DriftComp::HasRate()) {
    Serial.print(DriftComp::RatePpb());
  } else {
    Serial.print(F("none"));
  }
  Serial.print(F(" learn "));
  Serial.println(info.learn_seconds);
  if (info.rate_updated) {
    PrintDriftRate();
  }

  // 讓人看得出來剛對完時: 不管在哪個狀態都回到走時, 先亮日期, 再解碼鎖定成新時間
  time_adjusted_ = false;  // 已經用 USB 對準了, 不用再當手動設定
  now_ = received;
  EnterState(State::kTime);
  display_.ShowDate(received);
  delay(kSyncDateShowMs);
  RunDecodeShow(display_, rtc_, nullptr, 0);
  cleaner_.ScheduleNext();
}

//******************************** 設定時間 ********************************

TimeUnit NixieTubeClock::SettingUnit() const {
  return state_ == State::kSetHour ? kHour : (state_ == State::kSetMinute ? kMinute : kSecond);
}

// back: 切換 秒 -> 分 -> 時 -> 秒; front: 按一下 +1, 按住越久加越快, 放開就停
void NixieTubeClock::HandleTimeSettingInput() {
  if (buttons_.Has(ButtonInput::kBackPress)) {
    EnterState(state_ == State::kSetSecond ? State::kSetMinute
               : state_ == State::kSetMinute ? State::kSetHour : State::kSetSecond);
  }
  if (buttons_.Has(ButtonInput::kFrontPress)) {
    last_activity_ms_ = millis();
    AdjustTime();
    repeat_last_ms_ = millis();
    repeat_interval_ms_ = kRepeatStartIntervalMs;  // 每次重新按下都從慢速開始
    return;
  }
  // 秒只能歸零, 不需要連加; 兩顆一起按時也不連加
  const Button& front = buttons_.front();
  if (!front.IsPressed() || buttons_.back().IsPressed() || state_ == State::kSetSecond) {
    return;
  }
  last_activity_ms_ = millis();  // 按住期間不會逾時跳回正常模式
  if (front.PressedDurationMs() < kRepeatDelayMs) {
    return;
  }
  if (millis() - repeat_last_ms_ >= repeat_interval_ms_) {
    repeat_last_ms_ = millis();
    AdjustTime();
    // 每加一次, 下一次的間隔縮短一點, 直到最快速度
    repeat_interval_ms_ = max(kRepeatMinIntervalMs, repeat_interval_ms_ * kRepeatSpeedUpPercent / 100);
  }
}

void NixieTubeClock::AdjustTime() {
  Ds1302::DateTime now;
  rtc_.getDateTime(&now);
  const TimeUnit unit = SettingUnit();
  uint8_t& field = FieldOf(now, unit);
  const uint8_t before = field;
  if (unit == kSecond) {
    field = 0;  // 秒只能歸零
  } else if (unit == kMinute) {
    field = (field + 1) % 60;
  } else {
    field = (field + 1) % 24;
  }
  const uint8_t target = field;
  display_.ShowNumber(unit, target);
  FieldOf(now_, unit) = target;  // 這一輪後面閃爍時用的也是新數字, 不會閃回舊的
  time_adjusted_ = true;

  // 寫進 RTC 後馬上讀回來確認; 不一致就重寫, 日誌會留下紀錄
  uint8_t readback = 0;
  for (uint8_t attempt = 1; attempt <= kRtcWriteTries; ++attempt) {
    rtc_.setDateTime(&now);
    Ds1302::DateTime check;
    rtc_.getDateTime(&check);
    readback = FieldOf(check, unit);
    if (readback == target) {
      break;
    }
    Serial.print(F("[ADD] RTC write #"));
    Serial.print(attempt);
    Serial.print(F(" read back "));
    Serial.print(readback);
    Serial.print(F(", expected "));
    Serial.println(target);
  }

  if (kVerboseLog) {
    Serial.print(F("[ADD] "));
    Serial.print(kUnitNames[unit]);
    Serial.print(F(": "));
    Serial.print(before);
    Serial.print(F(" -> "));
    Serial.print(target);
    Serial.print(F(", RTC readback="));
    Serial.print(readback);
    Serial.print(F(", "));
    Print595Line(display_.LastData(unit));
  } else {
    // 短短一行, 按住每秒加 20 下時序列埠也來得及送
    Serial.print(kUnitNames[unit]);
    Serial.print('=');
    Serial.println(target);
  }
}

// 閃爍正在設定的那組管子, 其他照常顯示; 按住 front 加數字時保持常亮, 才看得清楚數字在跑
void NixieTubeClock::BlinkSettingTube() {
  Ds1302::DateTime shown = now_;
  const TimeUnit unit = SettingUnit();
  const bool holding_add = buttons_.front().IsPressed();
  const bool lit = holding_add || (millis() - blink_start_ms_) % kBlinkPeriodMs < kBlinkOnMs;
  if (!lit) {
    FieldOf(shown, unit) = Display::kBlank;
  }
  display_.ShowTime(shown);  // Display 會略過沒變的值, 每輪呼叫不會多送資料
  if (holding_add) {
    blink_start_ms_ = millis();  // 放開後從「亮」開始閃
  }
  if (kVerboseLog && lit) {
    // 診斷日誌: 設定中的管子實際被送了什麼數字 (數字有變才印), 拿來跟管子上看到的數字比對
    const uint8_t value = FieldOf(shown, unit);
    if (value != last_logged_show_value_) {
      last_logged_show_value_ = value;
      Serial.print(F("[SHOW] "));
      Serial.print(kUnitNames[unit]);
      Serial.print(F(" tube <- "));
      Serial.print(value);
      Serial.print(F(", "));
      Print595Line(display_.LastData(unit));
    }
  }
}

// 離開設定時間: 手動改過時間的話, 誤差補償從現在重新起算 (離開時才寫一次 EEPROM, 按住連加不會一直寫)
void NixieTubeClock::FinishTimeSetting() {
  if (!time_adjusted_) {
    return;
  }
  time_adjusted_ = false;
  Ds1302::DateTime now;
  rtc_.getDateTime(&now);
  DriftComp::OnManualSet(TimeSync::ToEpoch(now));
}

//******************************** 亮度 ********************************

void NixieTubeClock::LoadBrightness() {
  brightness_percent_ = EEPROM.read(kBrightnessEepromAddress);
  if (brightness_percent_ < kBrightnessMin || brightness_percent_ > kBrightnessMax ||
      brightness_percent_ % kBrightnessStep != 0) {
    brightness_percent_ = kBrightnessDefault;  // 沒存過 (EEPROM 預設 0xFF) 或資料不對
  }
  ApplyBrightness(false);
}

// front 亮一階, back 暗一階, 到頭就停在那裡
void NixieTubeClock::HandleBrightnessInput() {
  int8_t step = 0;
  if (buttons_.Has(ButtonInput::kFrontPress)) {
    step = 1;
  }
  if (buttons_.Has(ButtonInput::kBackPress)) {
    step = -1;
  }
  if (step == 0) {
    return;
  }
  last_activity_ms_ = millis();
  const int next = brightness_percent_ + step * kBrightnessStep;
  if (next >= kBrightnessMin && next <= kBrightnessMax) {
    brightness_percent_ = next;
    ApplyBrightness(true);
  }
  ShowBrightness();
}

void NixieTubeClock::ApplyBrightness(bool save) {
  TubeDriver::SetBrightness(brightness_percent_);
  ColonLed::SetBrightness(brightness_percent_);
  if (save) {
    EEPROM.update(kBrightnessEepromAddress, brightness_percent_);  // 沒變就不寫
  }
  Serial.print(F("[BRIGHTNESS] "));
  Serial.print(brightness_percent_);
  Serial.println('%');
}

// 亮度顯示在右邊 3 支管子, 例如 "    80" / "   100", 其他不亮
void NixieTubeClock::ShowBrightness() {
  const uint8_t p = brightness_percent_;
  const uint8_t digits[6] = {Display::kBlank, Display::kBlank, Display::kBlank,
                             (uint8_t)(p >= 100 ? p / 100 : Display::kBlank),
                             (uint8_t)(p / 10 % 10), (uint8_t)(p % 10)};
  display_.ShowDigits(digits);
}

//******************************** RTC 與誤差補償 ********************************

void NixieTubeClock::InitRtc() {
  rtc_.init();
  if (rtc_.isHalted()) {  // 電池沒電或第一次用: RTC 停著, 先給一個時間讓它跑起來
    Serial.println(F("RTC is halted. Setting time..."));
    rtc_was_reset_ = true;
    Ds1302::DateTime dt = {.year = 24, .month = Ds1302::MONTH_OCT,
      .day = 3, .hour = 2, .minute = 2, .second = 22, .dow = Ds1302::DOW_TUE};
    rtc_.setDateTime(&dt);
  }
}

// 寫進 RTC 後讀回確認 (秒數允許差 1, 寫完剛好跳秒), 不一致就重寫
bool NixieTubeClock::WriteRtcVerified(const Ds1302::DateTime& time) {
  Ds1302::DateTime writable = time;
  for (uint8_t attempt = 1; attempt <= kRtcWriteTries; ++attempt) {
    rtc_.setDateTime(&writable);
    Ds1302::DateTime check;
    rtc_.getDateTime(&check);
    const long diff = TimeSync::DiffSeconds(check, time);
    if (diff >= 0 && diff <= 1) {
      return true;
    }
  }
  return false;
}

// 時鐘快/慢累積滿 1 秒就撥回來 (見 drift_comp.h); now 會更新成撥完的時間
void NixieTubeClock::ApplyDriftCompensation(Ds1302::DateTime* now) {
  const long due = DriftComp::DueSeconds(TimeSync::ToEpoch(*now));
  if (due == 0) {
    return;
  }
  Ds1302::DateTime fixed;
  TimeSync::FromEpoch(TimeSync::ToEpoch(*now) - due, &fixed);
  if (!WriteRtcVerified(fixed)) {
    Serial.println(F("[DRIFT] RTC write failed"));
    return;
  }
  DriftComp::MarkApplied(due);
  *now = fixed;
  Serial.print(F("[DRIFT] clock was "));
  Serial.print(due > 0 ? F("fast") : F("slow"));
  Serial.print(F(", moved "));
  Serial.print(due > 0 ? F("back ") : F("forward "));
  Serial.print(labs(due));
  Serial.println(F(" s"));
}

// 印出目前的速率, 例如 "[DRIFT] rate +23150 ppb (+2.00 s/day)"
void NixieTubeClock::PrintDriftRate() {
  if (!DriftComp::HasRate()) {
    Serial.println(F("[DRIFT] rate not measured yet (needs 2 USB syncs >= 3 days apart)"));
    return;
  }
  const long ppb = DriftComp::RatePpb();
  // 每天幾秒 = ppb * 86400 / 1e9; 印到小數 2 位 -> 以 0.01 秒為單位
  const long centi = (long)(((int64_t)ppb * 86400LL + (ppb >= 0 ? 5000000LL : -5000000LL)) / 10000000LL);
  char line[48];
  snprintf_P(line, sizeof(line), PSTR("[DRIFT] rate %+ld ppb (%c%ld.%02ld s/day)"),
             ppb, centi < 0 ? '-' : '+', labs(centi) / 100, labs(centi) % 100);
  Serial.println(line);
}
