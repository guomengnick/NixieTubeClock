#ifndef FAST_TICK_H
#define FAST_TICK_H

// Timer2 每秒中斷 16000 次, 給需要準時快速切換的東西用:
//   TubeDriver: 輝光管淡入淡出換字
//   ColonLed:   冒號呼吸燈的軟體 PWM
// Timer0 是 millis()/delay() 在用, Timer1 留給 Servo 之類的函式庫, Timer2 原本只有 tone() 會用 (本專案沒用)
namespace FastTick {

void Begin();

}  // namespace FastTick

#endif  // FAST_TICK_H
