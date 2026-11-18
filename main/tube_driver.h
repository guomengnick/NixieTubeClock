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

// 要顯示的資料 (Q7..Q0); fade = true 從目前的數字淡入過去, false 立刻切換
void Show(uint8_t id, uint8_t data, bool fade);

// 由 FastTick 的中斷每秒呼叫 16000 次, 不要在別的地方呼叫
void Tick();

}  // namespace TubeDriver

#endif  // TUBE_DRIVER_H
