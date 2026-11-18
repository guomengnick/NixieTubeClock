#include "display.h"

#include "config.h"

namespace {
  // K155ID1 的 DCBA 就是數字本身的 BCD; 1111 (15) = 不亮
  uint8_t Bcd(uint8_t digit) {
    return digit <= 9 ? digit : 0x0F;
  }

  // 由左到右第 tube 支在哪一顆 595
  TimeUnit UnitOfTube(uint8_t tube) {
    return tube < 2 ? kHour : (tube < 4 ? kMinute : kSecond);
  }
}

void Display::Begin() {
  // 順序 = TimeUnit (秒 / 分 / 時)
  const uint8_t latch_pins[TubeDriver::kRegisters] = {kSecondLatchPin, kMinuteLatchPin, kHourLatchPin};
  const uint8_t clock_pins[TubeDriver::kRegisters] = {kSecondClockPin, kMinuteClockPin, kHourClockPin};
  const uint8_t data_pins[TubeDriver::kRegisters] = {kSecondDataPin, kMinuteDataPin, kHourDataPin};
  TubeDriver::Begin(latch_pins, clock_pins, data_pins);
}

void Display::ShowPair(TimeUnit unit, uint8_t tens, uint8_t ones, uint8_t fade_ticks) {
  // 595 的高 4 位 (Q7..Q4) 接個位數的 K155ID1, 低 4 位 (Q3..Q0) 接十位數
  const uint8_t data = (Bcd(ones) << 4) | Bcd(tens);
  if (last_data_[unit] == data) {
    return;  // 沒變就不送, 每個 loop 呼叫也不會重新淡入
  }
  last_data_[unit] = data;
  TubeDriver::Show(unit, data, fade_ticks);
}

void Display::ShowNumber(TimeUnit unit, uint8_t value, uint8_t fade_ticks) {
  if (value == kBlank) {
    ShowPair(unit, kBlank, kBlank, fade_ticks);
  } else {
    ShowPair(unit, value / 10, value % 10, fade_ticks);
  }
}

void Display::ShowDigits(const uint8_t digits[6], uint8_t fade_ticks) {
  ShowPair(kHour, digits[0], digits[1], fade_ticks);
  ShowPair(kMinute, digits[2], digits[3], fade_ticks);
  ShowPair(kSecond, digits[4], digits[5], fade_ticks);
}

void Display::ShowTime(const Ds1302::DateTime& t, uint8_t fade_ticks) {
  ShowNumber(kHour, t.hour, fade_ticks);
  ShowNumber(kMinute, t.minute, fade_ticks);
  ShowNumber(kSecond, t.second, fade_ticks);
}

void Display::ShowDate(const Ds1302::DateTime& t, uint8_t fade_ticks) {
  ShowNumber(kHour, t.year, fade_ticks);
  ShowNumber(kMinute, t.month, fade_ticks);
  ShowNumber(kSecond, t.day, fade_ticks);
}

uint8_t Display::ShownDigit(uint8_t tube) const {
  const int data = last_data_[UnitOfTube(tube)];
  if (data < 0) {
    return kBlank;
  }
  const uint8_t digit = tube % 2 == 0 ? (data & 0x0F) : (data >> 4);  // 左邊那支是十位
  return digit <= 9 ? digit : kBlank;
}
