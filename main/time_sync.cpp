#include "time_sync.h"

namespace {
  const uint8_t kLineMax = 24;  // "T2026-11-18 10:00:00" 是 20 個字
  char g_line[kLineMax + 1];
  uint8_t g_length = 0;
  bool g_overflow = false;      // 這一行太長, 等換行後直接當錯誤

  bool IsLeap(uint16_t year) {
    return (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
  }

  uint8_t DaysInMonth(uint16_t year, uint8_t month) {
    static const uint8_t kDays[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    return month == 2 && IsLeap(year) ? 29 : kDays[month - 1];
  }

  // 從 2000-01-01 起算第幾天
  long DaysSince2000(const Ds1302::DateTime& t) {
    const uint16_t year = 2000 + t.year;
    long days = 0;
    for (uint16_t y = 2000; y < year; ++y) {
      days += IsLeap(y) ? 366 : 365;
    }
    for (uint8_t m = 1; m < t.month && m <= 12; ++m) {
      days += DaysInMonth(year, m);
    }
    return days + t.day - 1;
  }

  // 星期幾, 換成 DS1302 的編號 (1 = 星期一 ... 7 = 星期日)
  uint8_t DayOfWeek(uint16_t year, uint8_t month, uint8_t day) {
    static const uint8_t kOffset[12] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
    if (month < 3) {
      year -= 1;
    }
    const uint8_t sunday0 = (year + year / 4 - year / 100 + year / 400 + kOffset[month - 1] + day) % 7;
    return sunday0 == 0 ? 7 : sunday0;
  }

  // 讀 count 位數字; 有不是數字的字就失敗
  bool ReadNumber(const char* s, uint8_t count, uint16_t* value) {
    *value = 0;
    for (uint8_t i = 0; i < count; ++i) {
      if (s[i] < '0' || s[i] > '9') {
        return false;
      }
      *value = *value * 10 + (s[i] - '0');
    }
    return true;
  }

  // "T2026-11-18 10:00:00"
  bool ParseSetTime(const char* s, Ds1302::DateTime* out) {
    if (strlen(s) != 20 || s[5] != '-' || s[8] != '-' || s[11] != ' ' || s[14] != ':' || s[17] != ':') {
      return false;
    }
    uint16_t year, month, day, hour, minute, second;
    if (!ReadNumber(s + 1, 4, &year) || !ReadNumber(s + 6, 2, &month) || !ReadNumber(s + 9, 2, &day) ||
        !ReadNumber(s + 12, 2, &hour) || !ReadNumber(s + 15, 2, &minute) || !ReadNumber(s + 18, 2, &second)) {
      return false;
    }
    // DS1302 只存年份後兩位, 所以只收 2000~2099 年
    if (year < 2000 || year > 2099 || month < 1 || month > 12 || day < 1 ||
        day > DaysInMonth(year, month) || hour > 23 || minute > 59 || second > 59) {
      return false;
    }
    out->year = year - 2000;
    out->month = month;
    out->day = day;
    out->hour = hour;
    out->minute = minute;
    out->second = second;
    out->dow = DayOfWeek(year, month, day);
    return true;
  }
}

namespace TimeSync {

Command Poll(Ds1302::DateTime* out) {
  while (Serial.available() > 0) {
    const char c = Serial.read();
    if (c != '\n' && c != '\r') {
      if (g_length < kLineMax) {
        g_line[g_length++] = c;
      } else {
        g_overflow = true;
      }
      continue;
    }
    // 一行結束 ("\r\n" 的第二個字會變成空行, 直接略過)
    if (g_length == 0 && !g_overflow) {
      continue;
    }
    g_line[g_length] = '\0';
    const bool overflow = g_overflow;
    g_length = 0;
    g_overflow = false;
    if (overflow) {
      return kBad;
    }
    if (strcmp(g_line, "?") == 0) {
      return kQuery;
    }
    if ((g_line[0] == 'T' || g_line[0] == 't') && ParseSetTime(g_line, out)) {
      return kSetTime;
    }
    return kBad;
  }
  return kNone;
}

long DiffSeconds(const Ds1302::DateTime& a, const Ds1302::DateTime& b) {
  const long days = DaysSince2000(a) - DaysSince2000(b);
  if (days > 20000 || days < -20000) {
    return days > 0 ? 999999999L : -999999999L;  // 差好幾十年, 32 位元裝不下, 也沒意義
  }
  const long seconds_a = a.hour * 3600L + a.minute * 60L + a.second;
  const long seconds_b = b.hour * 3600L + b.minute * 60L + b.second;
  return days * 86400L + seconds_a - seconds_b;
}

void Format(const Ds1302::DateTime& t, char* buf, size_t size) {
  snprintf(buf, size, "20%02u-%02u-%02u %02u:%02u:%02u",
           t.year, t.month, t.day, t.hour, t.minute, t.second);
}

}  // namespace TimeSync
