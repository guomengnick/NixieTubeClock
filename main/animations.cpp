#include "animations.h"

#include "config.h"

//******************************** 走梯子 ********************************

// 路線上第 pos 格是梯子的第幾階: 0~9 往上, 10 以後往下 (10 -> 第 8 階 ... 18 -> 第 0 階)
static uint8_t RungAt(uint8_t pos) {
  return pos <= 9 ? pos : 18 - pos;
}

// 數字在梯子的第幾階
static uint8_t RungOf(uint8_t digit) {
  uint8_t rung = 0;
  while (rung < 9 && kLadder[rung] != digit) {
    ++rung;
  }
  return rung;
}

// 24 小時制下, 每支管子平常最大會顯示到幾 (時十位 0~2, 分十位、秒十位 0~5); 比這大的就是「平常不會亮的數字」
static const uint8_t kNormalMaxDigit[6] = {2, 9, 5, 9, 5, 9};

void LadderWalk::Start(uint8_t move_mask, const Style& style) {
  const unsigned long now = millis();
  style_ = style;
  together_max_ = 9;
  bool first = true;
  for (uint8_t i = 0; i < 6; ++i) {
    if (!((move_mask >> i) & 1)) {
      pos_[i] = kLanded;
      continue;
    }
    // 接力: 只有第一支馬上起步, 其他排隊; 不是接力: 依 stagger 由左到右起步
    pos_[i] = style.relay && !first ? kQueued : kWaiting;
    next_ms_[i] = now + (style.relay ? 0 : i * style.stagger_ms);  // 第一格 = 跳到梯子最底下
    first = false;
    if (kNormalMaxDigit[i] < together_max_) {
      together_max_ = kNormalMaxDigit[i];
    }
  }
}

bool LadderWalk::Update(Display& display, const Ds1302::DateTime& now_time) {
  const unsigned long now = millis();
  const uint8_t target[6] = {
    (uint8_t)(now_time.hour / 10), (uint8_t)(now_time.hour % 10),
    (uint8_t)(now_time.minute / 10), (uint8_t)(now_time.minute % 10),
    (uint8_t)(now_time.second / 10), (uint8_t)(now_time.second % 10)};
  uint8_t digit[6];
  bool walking_pair[3] = {false, false, false};
  bool any = false;
  bool hand_off = false;  // 接力: 這一格有管子到站, 排隊的下一支馬上起步
  for (uint8_t i = 0; i < 6; ++i) {
    uint8_t& pos = pos_[i];
    if (pos == kQueued && hand_off) {
      pos = kWaiting;
      next_ms_[i] = now;
      hand_off = false;
    }
    if (pos != kLanded && pos != kQueued && (long)(now - next_ms_[i]) >= 0) {
      pos = pos == kWaiting ? 0 : pos + 1;
      // 終點: 往下走回時間的數字 (會跟著時間變, 例如秒個位, 所以每一格都重新算); 不往下走就是到頂的下一格
      const uint8_t last = style_.descend ? 18 - RungOf(target[i]) : 10;
      if (pos >= last) {
        pos = kLanded;
        walking_pair[i / 2] = true;  // 到站這一格也用走梯子的淡入淡出
        hand_off = style_.relay;
      } else {
        const uint8_t shown = kLadder[RungAt(pos)];
        unsigned long wait = style_.step_ms;
        const uint8_t normal_max = style_.together ? together_max_ : kNormalMaxDigit[i];
        if (pos <= 9 && shown > normal_max && style_.rare_dwell_ms > wait) {
          wait = style_.rare_dwell_ms;  // 平常不會亮的數字多停一下 (together: 大家一起停, 保持同步)
        }
        // 往下走回時間時, 最後 kLadderSlowSteps 格越來越慢, 像電梯到站
        // (together 往上時不放慢, 不然新數字是 7~9 的那支會在往上途中跟大家錯開)
        const uint8_t left = last - pos;
        if (style_.descend && left <= kLadderSlowSteps && !(style_.together && pos <= 9)) {
          wait += (kLadderSlowSteps + 1 - left) * (style_.step_ms / 2);
        }
        next_ms_[i] = now + wait;
      }
    }
    if (pos == kLanded || pos == kWaiting || pos == kQueued) {
      digit[i] = target[i];  // 停好 / 還沒起步: 顯示現在時間
    } else {
      digit[i] = kLadder[RungAt(pos)];
      walking_pair[i / 2] = true;
    }
    any = any || pos != kLanded;
  }
  // 有管子在走的那一組用走梯子的淡入淡出; 停好的組跟平常走時一樣 (ShowPair 沒變就不會重送)
  display.ShowPair(kHour, digit[0], digit[1], walking_pair[0] ? style_.fade : kTimeFade);
  display.ShowPair(kMinute, digit[2], digit[3], walking_pair[1] ? style_.fade : kTimeFade);
  display.ShowPair(kSecond, digit[4], digit[5], walking_pair[2] ? style_.fade : kTimeFade);
  return any;
}

uint8_t ChangedTimeTubes(const Display& display, const Ds1302::DateTime& now) {
  const uint8_t target[4] = {
    (uint8_t)(now.hour / 10), (uint8_t)(now.hour % 10),
    (uint8_t)(now.minute / 10), (uint8_t)(now.minute % 10)};
  uint8_t mask = 0;
  for (uint8_t i = 0; i < 4; ++i) {
    if (display.ShownDigit(i) != target[i]) {
      mask |= 1 << i;
    }
  }
  return mask;
}

//******************************** 清管 ********************************

void TubeCleaner::ScheduleNext() {
  scheduled_at_ms_ = millis();
  wait_seconds_ = random(kAutoCleanMinSeconds, kAutoCleanMaxSeconds + 1);
  Serial.print(F("[CLEAN] next in "));
  Serial.print(wait_seconds_);
  Serial.println(F(" s"));
}

bool TubeCleaner::Due() const {
  return (millis() - scheduled_at_ms_) / 1000 >= wait_seconds_;
}

void TubeCleaner::Start(Display& display) {
  static const LadderWalk::Style kElevatorStyle = {
    kCleanElevatorStepMs, 0, kCleanRareDwellMs, kCleanElevatorFade, true, false, true};
  static const LadderWalk::Style kWaveStyle = {
    kCleanWaveStepMs, kCleanWaveStaggerMs, kCleanRareDwellMs, kCleanWaveFade, true, false, false};
  static const LadderWalk::Style kRelayStyle = {
    kCleanRelayStepMs, 0, kCleanRelayRareDwellMs, kCleanRelayFade, false, true, false};
  mode_ = (Mode)random(kModeCount);
  switch (mode_) {
    case kFlow:
      Serial.println(F("[CLEAN] flow"));
      step_ = 0;
      step_ms_ = kCleanFirstStepMs;
      ShowFlowStep(display);
      break;
    case kElevator:
      Serial.println(F("[CLEAN] elevator"));
      walk_.Start(0x3F, kElevatorStyle);
      break;
    case kWave:
      Serial.println(F("[CLEAN] wave"));
      walk_.Start(0x3F, kWaveStyle);
      break;
    case kRelay:
      Serial.println(F("[CLEAN] relay"));
      walk_.Start(0x3F, kRelayStyle);
      break;
    default:
      break;
  }
}

bool TubeCleaner::Update(Display& display, const Ds1302::DateTime& now) {
  if (mode_ != kFlow) {
    return walk_.Update(display, now);
  }
  if (millis() - step_start_ms_ < step_ms_) {
    return true;
  }
  ++step_;
  step_ms_ -= kCleanStepSpeedUpMs;
  if (step_ >= kTubeCleanSteps) {
    return false;
  }
  ShowFlowStep(display);
  return true;
}

void TubeCleaner::ShowFlowStep(Display& display) {
  // 6 支管子同時顯示同一個數字 (例如 11 11 11); 快速溶接到下一步, 看起來像在流動
  const uint8_t number = kTubeShowNumbers[step_ % 10];
  display.ShowNumber(kHour, number, kCleanFade);
  display.ShowNumber(kMinute, number, kCleanFade);
  display.ShowNumber(kSecond, number, kCleanFade);
  step_start_ms_ = millis();
}

//******************************** 解碼秀 ********************************

void RunDecodeShow(Display& display, Ds1302& rtc, const uint8_t sweep_digits[10],
                   unsigned long sweep_step_ms) {
  uint8_t digits[6];

  // 1. 掃描: 每支管子的 0~9 都亮過一次
  for (uint8_t i = 0; sweep_digits != nullptr && i < 10; ++i) {
    memset(digits, sweep_digits[i], sizeof(digits));
    display.ShowDigits(digits);
    delay(sweep_step_ms);
  }

  // 2. 解碼: 還沒鎖定的管子亂跳, 由左到右一支一支鎖定成現在時間
  Ds1302::DateTime now;
  rtc.getDateTime(&now);
  const uint8_t target[6] = {
    (uint8_t)(now.hour / 10), (uint8_t)(now.hour % 10),
    (uint8_t)(now.minute / 10), (uint8_t)(now.minute % 10),
    (uint8_t)(now.second / 10), (uint8_t)(now.second % 10)};
  for (uint8_t locked = 0; locked < 6; ++locked) {
    for (uint8_t frame = 0; frame < kBootFramesPerLock; ++frame) {
      for (uint8_t i = 0; i < 6; ++i) {
        digits[i] = i < locked ? target[i] : random(10);
      }
      display.ShowDigits(digits);
      delay(kBootScrambleMs);
    }
  }

  // 3. 閃斷: 熄 / 亮 / 熄 / 亮
  for (uint8_t i = 0; i < 2; ++i) {
    memset(digits, Display::kBlank, sizeof(digits));
    display.ShowDigits(digits);
    delay(kBootGlitchMs);
    display.ShowDigits(target);
    delay(kBootGlitchMs);
  }

  rtc.getDateTime(&now);  // 動畫期間秒數可能已經跳了, 以 RTC 為準
  display.ShowTime(now, kTimeFade);
}
