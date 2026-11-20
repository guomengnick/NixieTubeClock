#ifndef DRIFT_COMP_H
#define DRIFT_COMP_H

#include <Arduino.h>

// DS1302 自動誤差補償
//
// DS1302 的石英每顆都有固定的快慢 (常見每天 1~4 秒), 只有溫度和老化會讓它慢慢變。
// 做法:
//   1. 學習: 每次 USB 對時都知道「時鐘快/慢了幾秒」。兩次對時隔 3 天以上
//      (差很多的話, 例如隔 20 小時以上就差了 20 秒, 不用等 3 天),
//      就能算出這顆 RTC 的速率 (ppb = 十億分之一; +23000 ppb = 每天快 2 秒)。
//      之後繼續累積到 30 天, 越久越準; 滿 30 天重新開始量, 跟上季節和老化。
//   2. 補償: 從上次對時 (錨點) 起經過的秒數 × 速率 = 應該修正的秒數,
//      累積滿 1 秒就把 RTC 往回撥 (或往前撥) 1 秒。
//      關機期間 RTC 靠電池照樣跑偏, 開機時一次補回來。
// 全部存在 Nano 的 EEPROM, 拔電也不會忘。時間都用 TimeSync::ToEpoch 的秒數。
namespace DriftComp {

void Begin();  // 從 EEPROM 讀回; 第一次用 (或格式不對) 就從頭開始

// 現在應該把時鐘往回撥幾秒 (+ = 時鐘快了, 要撥慢; - = 慢了, 要撥快); 0 = 不用動
long DueSeconds(uint32_t now);
// 真的撥完 DueSeconds 回傳的秒數後呼叫
void MarkApplied(long seconds);

struct SyncInfo {
  long raw_drift;          // 如果沒有補償, 這段期間會差幾秒 (+ = 快)
  long applied;            // 這段期間已經自動修正了幾秒
  bool rate_updated;       // 這次對時算出了新的速率
  uint32_t learn_seconds;  // 目前這一輪學習累積了幾秒 (滿 3 天才會算速率)
};
// USB 對時: drift = 對時前時鐘快 (+) / 慢 (-) 了幾秒, now = 對時後的正確時間
SyncInfo OnSync(uint32_t now, long drift);

// 手動用按鈕改了時間: 從這裡重新開始補償, 但這一輪學習作廢 (手動改的時間不準)
void OnManualSet(uint32_t now);
// RTC 電池沒電被重設: 錨點都不能用了, 等下次 USB 對時
void Forget();

bool HasRate();
long RatePpb();  // + = 時鐘偏快

}  // namespace DriftComp

#endif  // DRIFT_COMP_H
