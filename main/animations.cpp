#include "animations.h"

#include "config.h"

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
  step_ = 0;
  step_ms_ = kCleanFirstStepMs;
  ShowStep(display);
}

bool TubeCleaner::Update(Display& display) {
  if (millis() - step_start_ms_ < step_ms_) {
    return true;
  }
  ++step_;
  step_ms_ -= kCleanStepSpeedUpMs;
  if (step_ >= kTubeCleanSteps) {
    return false;
  }
  ShowStep(display);
  return true;
}

void TubeCleaner::ShowStep(Display& display) {
  // 6 支管子同時顯示同一個數字 (例如 11 11 11); 快速溶接到下一步, 看起來像在流動
  const uint8_t number = kTubeShowNumbers[step_ % 10];
  display.ShowNumber(kHour, number, kCleanFade);
  display.ShowNumber(kMinute, number, kCleanFade);
  display.ShowNumber(kSecond, number, kCleanFade);
  step_start_ms_ = millis();
}

//******************************** 換字掃描 ********************************

// 路線上第 pos 格是梯子的第幾階: 0~9 往上, 10 以後往下 (10 -> 第 8 階 ... 18 -> 第 0 階)
static uint8_t RungAt(uint8_t pos) {
  return pos <= 9 ? pos : 18 - pos;
}

bool DigitSweep::Start(Display& display, const Ds1302::DateTime& now) {
  const uint8_t target[4] = {
    (uint8_t)(now.hour / 10), (uint8_t)(now.hour % 10),
    (uint8_t)(now.minute / 10), (uint8_t)(now.minute % 10)};
  bool any = false;
  for (uint8_t i = 0; i < 4; ++i) {
    target_[i] = target[i];
    pos_[i] = last_[i] = 0;
    if (display.ShownDigit(i) == target[i]) {
      continue;  // 沒變的管子不動
    }
    uint8_t rung = 0;  // 新數字在梯子的第幾階
    while (rung < 9 && kRollLadder[rung] != target[i]) {
      ++rung;
    }
    last_[i] = 18 - rung;  // 走到頂 (9) 再往下走回這一階
    any = true;
  }
  if (any) {
    for (uint8_t i = 0; i < 4; ++i) {
      next_ms_[i] = millis() + kRollStepMs;  // 同時起步, 一起往上走
    }
    Show(display);  // 有變的管子先跳到最底下那個數字
  }
  return any;
}

bool DigitSweep::Update(Display& display) {
  const unsigned long now = millis();
  bool changed = false;
  bool rolling = false;
  for (uint8_t i = 0; i < 4; ++i) {
    if (pos_[i] >= last_[i]) {
      continue;  // 已經停好 (或這支沒變)
    }
    if ((long)(now - next_ms_[i]) >= 0) {
      const uint8_t left = last_[i] - ++pos_[i];
      // 最後 kRollSlowSteps 格越來越慢, 像電梯到站
      unsigned long wait = kRollStepMs;
      if (left > 0 && left <= kRollSlowSteps) {
        wait += (kRollSlowSteps + 1 - left) * kRollSlowDownMs;
      }
      next_ms_[i] = now + wait;
      changed = true;
    }
    rolling = rolling || pos_[i] < last_[i];
  }
  if (changed) {
    Show(display);
  }
  return rolling;  // 全部停好時顯示的就是新時間
}

void DigitSweep::Show(Display& display) const {
  uint8_t digit[4];
  for (uint8_t i = 0; i < 4; ++i) {
    digit[i] = last_[i] == 0 ? target_[i] : kRollLadder[RungAt(pos_[i])];
  }
  display.ShowPair(kHour, digit[0], digit[1], kRollFade);
  display.ShowPair(kMinute, digit[2], digit[3], kRollFade);
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
