#ifndef TUBE_DRIVER_H
#define TUBE_DRIVER_H

#include <Arduino.h>

// 輝光管淡入淡出換字
//
// 原理跟冒號呼吸燈一樣是「快速切換」: 換字的 0.3 秒內, 讓 595 在舊數字和新數字之間每秒切換 2000 次,
// 一開始幾乎都送舊的, 慢慢變成幾乎都送新的, 人眼看起來就是舊數字暗下去、新數字亮起來。
//
// 切換要準時又不能卡住按鈕, 所以 Begin() 之後所有送給 595 的資料都改由 Timer2 中斷 (FastTick) 送出,
// 主程式只負責說「這組要顯示什麼、要不要淡入」。一個 595 管兩支管子, 沒變的那支兩邊資料相同, 會一直亮著。
namespace TubeDriver {

const uint8_t kRegisters = 3;

// 設定 3 顆 595 的腳位 (順序 = ShiftRegister 的 id), 之後由中斷接手送資料
void Begin(const uint8_t latch_pins[kRegisters], const uint8_t clock_pins[kRegisters],
           const uint8_t data_pins[kRegisters]);

bool Running();

// 淡入速度: 新數字每幾次中斷 (1/2000 秒) 多佔一格, 16 格換完; 0 = 立刻切換
const uint8_t kFadeInstant = 0;
const uint8_t kFadeNormal = 31;  // 約 0.25 秒, 正常走時用
const uint8_t kFadeQuick = 10;   // 約 0.08 秒, 清管動畫用 (每步約 0.1 秒, 剛好一步接一步流過去)

// 要顯示的資料 (Q7..Q0), 從目前的數字以 ticks_per_level 的速度淡入過去
void Show(uint8_t id, uint8_t data, uint8_t ticks_per_level);

// 由 FastTick 的中斷每秒呼叫 16000 次, 不要在別的地方呼叫
void Tick();

}  // namespace TubeDriver

#endif  // TUBE_DRIVER_H
