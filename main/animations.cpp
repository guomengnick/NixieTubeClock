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

//******************************** 滾輪換字 ********************************

bool SlotRoll::Start(const Display& display, const Ds1302::DateTime& now) {
  const uint8_t target[4] = {
    (uint8_t)(now.hour / 10), (uint8_t)(now.hour % 10),
    (uint8_t)(now.minute / 10), (uint8_t)(now.minute % 10)};
  uint8_t min_steps = kRollMinSteps;
  bool any = false;
  for (uint8_t i = 0; i < 4; ++i) {
    const uint8_t shown = display.ShownDigit(i);
    digit_[i] = target[i];
    steps_left_[i] = 0;
    if (shown == target[i]) {
      continue;  // 沒變的管子不動
    }
    const uint8_t from = shown <= 9 ? shown : target[i];  // 原本不亮就從新數字滾一整圈
    // 從舊數字一格一格往上 (9 -> 0 繞回來), 至少 min_steps 格, 剛好停在新數字
    uint8_t steps = min_steps;
    while ((from + steps) % 10 != target[i]) {
      ++steps;
    }
    digit_[i] = from;
    steps_left_[i] = steps;
    next_ms_[i] = millis();               // 第一格馬上動
    min_steps = steps + kRollStaggerSteps;  // 右邊那支比這支晚停
    any = true;
  }
  return any;
}

bool SlotRoll::Update(Display& display) {
  const unsigned long now = millis();
  bool changed = false;
  bool rolling = false;
  for (uint8_t i = 0; i < 4; ++i) {
    if (steps_left_[i] == 0) {
      continue;
    }
    if ((long)(now - next_ms_[i]) >= 0) {
      digit_[i] = (digit_[i] + 1) % 10;
      const uint8_t left = --steps_left_[i];
      // 最後 kRollSlowSteps 格越來越慢, 像滾輪停下來
      unsigned long wait = kRollStepMs;
      if (left > 0 && left <= kRollSlowSteps) {
        wait += (kRollSlowSteps + 1 - left) * kRollSlowDownMs;
      }
      next_ms_[i] = now + wait;
      changed = true;
    }
    rolling = rolling || steps_left_[i] > 0;
  }
  if (changed) {
    display.ShowPair(kHour, digit_[0], digit_[1], kRollFade);
    display.ShowPair(kMinute, digit_[2], digit_[3], kRollFade);
  }
  return rolling;  // 全部停好時顯示的就是新時間
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
