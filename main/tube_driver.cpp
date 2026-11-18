#include "tube_driver.h"

namespace {
  // 中斷每秒 16000 次, 3 顆 595 輪流處理 (每 8 次裡的前 3 次), 每顆每秒更新 2000 次
  const uint8_t kSlots = 16;          // 一個切換週期 16 格 = 每秒 125 輪, 看不出閃爍

  struct Register {
    volatile uint8_t* latch_port;
    volatile uint8_t* clock_port;
    volatile uint8_t* data_port;
    uint8_t latch_mask;
    uint8_t clock_mask;
    uint8_t data_mask;
    uint8_t from;          // 淡出中的舊資料
    uint8_t to;            // 淡入中的新資料
    uint8_t level;         // 一個週期裡有幾格送新資料; kSlots = 換完了
    uint8_t level_ticks;
    uint8_t ticks_per_level;  // 這次淡入的速度
    uint8_t slot;          // 目前在週期的第幾格
    int16_t sent;          // 595 裡現在鎖存的資料, -1 = 還沒送過
  };

  Register g_registers[TubeDriver::kRegisters];
  bool g_running = false;

  // 中斷裡直接寫暫存器送 8 個 bit (約 6 微秒); 用 shiftOut()/digitalWrite() 要 100 多微秒, 太慢
  void Shift(Register& r, uint8_t value) {
    *r.latch_port &= ~r.latch_mask;
    for (uint8_t bit = 0; bit < 8; ++bit) {  // MSB 先送, 跟 shiftOut(MSBFIRST) 一樣
      if (value & 0x80) {
        *r.data_port |= r.data_mask;
      } else {
        *r.data_port &= ~r.data_mask;
      }
      *r.clock_port |= r.clock_mask;
      *r.clock_port &= ~r.clock_mask;
      value <<= 1;
    }
    *r.latch_port |= r.latch_mask;
  }
}

namespace TubeDriver {

void Begin(const uint8_t latch_pins[kRegisters], const uint8_t clock_pins[kRegisters],
           const uint8_t data_pins[kRegisters]) {
  noInterrupts();
  for (uint8_t i = 0; i < kRegisters; ++i) {
    Register& r = g_registers[i];
    r.latch_port = portOutputRegister(digitalPinToPort(latch_pins[i]));
    r.clock_port = portOutputRegister(digitalPinToPort(clock_pins[i]));
    r.data_port = portOutputRegister(digitalPinToPort(data_pins[i]));
    r.latch_mask = digitalPinToBitMask(latch_pins[i]);
    r.clock_mask = digitalPinToBitMask(clock_pins[i]);
    r.data_mask = digitalPinToBitMask(data_pins[i]);
    r.from = r.to = 0xFF;  // 全部不亮, 等主程式送第一筆資料
    r.level = kSlots;
    r.level_ticks = 0;
    r.slot = 0;
    r.sent = -1;
  }
  g_running = true;
  interrupts();
}

bool Running() { return g_running; }

void Show(uint8_t id, uint8_t data, uint8_t ticks_per_level) {
  if (id >= kRegisters) {
    return;
  }
  noInterrupts();  // 跟中斷共用這些變數, 一次改完
  Register& r = g_registers[id];
  if (ticks_per_level != kFadeInstant) {
    // 從「目前看起來比較亮的那個」淡入新資料; 上一次還沒換完就再換, 也不會跳回更舊的數字
    r.from = r.level >= kSlots / 2 ? r.to : r.from;
    r.to = data;
    r.level = 0;
    r.level_ticks = 0;
    r.ticks_per_level = ticks_per_level;
  } else {
    r.from = r.to = data;
    r.level = kSlots;
  }
  interrupts();
}

void Tick() {
  static uint8_t tick = 0;
  tick = (tick + 1) & 7;
  if (!g_running || tick >= kRegisters) {
    return;
  }
  Register& r = g_registers[tick];
  if (r.level < kSlots && ++r.level_ticks >= r.ticks_per_level) {
    r.level_ticks = 0;
    ++r.level;
  }
  r.slot = (r.slot + 1) & (kSlots - 1);
  const uint8_t out = r.slot < r.level ? r.to : r.from;
  if (out != r.sent) {
    Shift(r, out);
    r.sent = out;
  }
}

}  // namespace TubeDriver
