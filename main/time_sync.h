#ifndef TIME_SYNC_H
#define TIME_SYNC_H

#include <Arduino.h>
#include <Ds1302.h>

// USB 對時: 讀序列埠 (9600 baud) 的一行指令
//   ?                      -> 回報時鐘目前的時間
//   T2026-11-18 10:00:00   -> 把時鐘設成這個時間 (電腦端腳本 tools/sync_time.ps1 會在整秒那一刻送出)
// 也可以直接在 Arduino IDE 的序列埠監控視窗手動輸入 (行尾選「換行」)
namespace TimeSync {

enum Command : uint8_t {
  kNone,     // 還沒收到完整一行
  kQuery,    // ?
  kSetTime,  // T...; 時間放在 out
  kBad,      // 格式錯誤或日期不存在
};

// 每個 loop 呼叫一次, 不會卡住
Command Poll(Ds1302::DateTime* out);

// a 比 b 晚幾秒 (差太多時回傳 +-999999999, 只拿來顯示誤差)
long DiffSeconds(const Ds1302::DateTime& a, const Ds1302::DateTime& b);

// 印成 "2026-11-18 10:00:00"
void Format(const Ds1302::DateTime& t, char* buf, size_t size);

}  // namespace TimeSync

#endif  // TIME_SYNC_H
