#include "drift_comp.h"

#include <EEPROM.h>

namespace {
  // EEPROM 沒寫過是 0xFF, 對不上就當第一次用。
  // 0xD7 -> 0xD8: 改成少讀 RTC 之後 DS1302 的快慢可能不一樣了, 舊的量測作廢, 重新學
  const uint8_t kMagic = 0xD8;
  const int kEepromAddress = 0;  // 目前 EEPROM 只有這裡在用
  const uint32_t kNone = 0;      // 錨點 = 0 表示沒有

  const uint32_t kMinLearnSeconds = 3UL * 86400UL;    // 至少隔 3 天才算速率 (對時本身有約 1 秒誤差)
  const uint32_t kQuickLearnSeconds = 20UL * 3600UL;  // 誤差大的話隔 20 小時就先算 (之後越學越準)
  const long kQuickLearnDrift = 20;                   // ...累積差到這麼多秒才算大 (1 秒誤差 = 5%)
  const uint32_t kMaxLearnSeconds = 30UL * 86400UL;   // 累積滿 30 天重新量
  const uint32_t kMaxCorrectSpan = 400UL * 86400UL;   // 錨點超過這麼久 (或在未來) 就不補, 多半是 RTC 被重設
  const long kMaxRatePpb = 2000000L;                  // 2000 ppm = 每天 173 秒; 超過一定是量錯
  const long kApplyThresholdMs = 600;                 // 誤差超過 0.6 秒才撥, 撥完剩不到 0.5 秒, 不會來回撥

  struct Stored {
    uint8_t magic;
    uint8_t has_rate;
    int32_t rate_ppb;       // + = 時鐘偏快
    uint32_t anchor;        // 上次對時 (或手動設定) 的時間; 補償從這裡起算
    int32_t applied;        // 錨點之後已經往回撥了幾秒
    uint32_t learn_anchor;  // 這一輪學習從哪次對時開始
    int32_t learn_raw;      // 這一輪學習累積的「沒補償的話會差幾秒」
  };
  Stored g;

  void Save() {
    EEPROM.put(kEepromAddress, g);  // 只會寫有變的 byte
  }

  int64_t RoundDiv(int64_t a, int64_t b) {
    return a >= 0 ? (a + b / 2) / b : -((-a + b / 2) / b);
  }

  bool AnchorUsable(uint32_t anchor, uint32_t now) {
    return anchor != kNone && now >= anchor && now - anchor <= kMaxCorrectSpan;
  }
}

namespace DriftComp {

void Begin() {
  EEPROM.get(kEepromAddress, g);
  if (g.magic != kMagic) {
    g = Stored();
    g.magic = kMagic;
    Save();
  }
}

long DueSeconds(uint32_t now) {
  if (!g.has_rate || !AnchorUsable(g.anchor, now)) {
    return 0;
  }
  // 經過秒數 × ppb / 1e6 = 應該修正的毫秒數
  const int64_t should_ms = RoundDiv((int64_t)(now - g.anchor) * g.rate_ppb, 1000000LL);
  const int64_t error_ms = should_ms - (int64_t)g.applied * 1000;
  if (error_ms > -kApplyThresholdMs && error_ms < kApplyThresholdMs) {
    return 0;
  }
  return (long)RoundDiv(error_ms, 1000);
}

void MarkApplied(long seconds) {
  g.applied += seconds;
  Save();
}

SyncInfo OnSync(uint32_t now, long drift) {
  SyncInfo info = {};
  info.applied = g.applied;
  const bool anchor_ok = AnchorUsable(g.anchor, now);
  // 這段期間如果沒有補償會差幾秒: 量到的誤差 + 已經撥掉的
  info.raw_drift = drift + g.applied;

  // 合理性檢查: 差太多 (例如 RTC 電池沒電被重設) 就不拿來學
  const uint32_t span = anchor_ok ? now - g.anchor : 0;
  const long limit = (long)((int64_t)span * kMaxRatePpb / 1000000000LL) + 3;
  const bool plausible = anchor_ok && labs(info.raw_drift) <= limit;

  if (!plausible || !AnchorUsable(g.learn_anchor, now)) {
    g.learn_anchor = now;  // 從這次對時開始新的一輪
    g.learn_raw = 0;
  } else {
    g.learn_raw += info.raw_drift;
    const uint32_t learn_span = now - g.learn_anchor;
    if (learn_span >= kMinLearnSeconds ||
        (learn_span >= kQuickLearnSeconds && labs(g.learn_raw) >= kQuickLearnDrift)) {
      long rate = (long)RoundDiv((int64_t)g.learn_raw * 1000000000LL, learn_span);
      g.rate_ppb = constrain(rate, -kMaxRatePpb, kMaxRatePpb);
      g.has_rate = 1;
      info.rate_updated = true;
    }
    if (learn_span >= kMaxLearnSeconds) {
      g.learn_anchor = now;  // 速率先沿用這次算的, 重新累積下一輪
      g.learn_raw = 0;
    }
  }
  info.learn_seconds = now - g.learn_anchor;

  g.anchor = now;  // 時間剛對準, 補償從現在重新起算
  g.applied = 0;
  Save();
  return info;
}

void OnManualSet(uint32_t now) {
  g.anchor = now;
  g.applied = 0;
  g.learn_anchor = kNone;
  g.learn_raw = 0;
  Save();
}

void Forget() {
  g.anchor = kNone;
  g.applied = 0;
  g.learn_anchor = kNone;
  g.learn_raw = 0;
  Save();
}

bool HasRate() {
  return g.has_rate;
}

long RatePpb() {
  return g.rate_ppb;
}

}  // namespace DriftComp
