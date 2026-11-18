#ifndef BUTTON_INPUT_H
#define BUTTON_INPUT_H

#include <Arduino.h>

#include "button.h"

// 兩顆按鈕 -> 「事件」
//
// 原本主程式自己盯著按鈕狀態, 還要記「這次按住是不是已經跟另一顆一起按過」之類的旗標,
// 每加一個功能就要多一組旗標、多幾個 if。現在由這裡統一判斷手勢, 主程式只看事件:
//   kFrontTap  短按 front: 放開時沒有長按、也沒有跟 back 一起按
//   kFrontHold 單獨按住 front 超過 hold_ms (每次按住只觸發一次)
//   kBackHold  單獨按住 back 超過 hold_ms (每次按住只觸發一次)
//   kBothHold  兩顆一起按住超過 both_ms (放開前只觸發一次)
//   kFrontPress / kBackPress 剛按下的那一刻 (設定模式 +1、切換用)
// 一次按住只要另一顆也按過, 就算「一起按」, 不會再觸發單獨的短按/長按。
class ButtonInput {
 public:
  enum Event : uint8_t {
    kFrontPress = 1 << 0,
    kBackPress = 1 << 1,
    kFrontTap = 1 << 2,
    kFrontHold = 1 << 3,
    kBackHold = 1 << 4,
    kBothHold = 1 << 5,
  };

  ButtonInput(uint8_t back_pin, uint8_t front_pin, unsigned long front_hold_ms,
              unsigned long back_hold_ms, unsigned long both_hold_ms)
    : back_(back_pin), front_(front_pin), front_hold_ms_(front_hold_ms),
      back_hold_ms_(back_hold_ms), both_hold_ms_(both_hold_ms) {}

  void Begin() {
    back_.Begin();
    front_.Begin();
  }

  // 每個 loop 呼叫一次; 這一輪發生的事件之後用 Has() 查
  void Update() {
    back_.Update();
    front_.Update();
    events_ = 0;
    const bool back = back_.IsPressed();
    const bool front = front_.IsPressed();

    if (back_.WasPressed()) {
      events_ |= kBackPress;
      back_.combo = false;
      back_.hold_fired = false;
    }
    if (front_.WasPressed()) {
      events_ |= kFrontPress;
      front_.combo = false;
      front_.hold_fired = false;
    }
    if (back && front) {
      back_.combo = front_.combo = true;
    }

    if (front_.WasReleased() && !front_.combo && !front_.hold_fired) {
      events_ |= kFrontTap;
    }
    CheckHold(front_, front_hold_ms_, kFrontHold);
    CheckHold(back_, back_hold_ms_, kBackHold);

    if (!(back && front)) {
      both_fired_ = false;
    } else if (!both_fired_ && back_.PressedDurationMs() > both_hold_ms_ &&
               front_.PressedDurationMs() > both_hold_ms_) {
      both_fired_ = true;
      events_ |= kBothHold;
    }
  }

  bool Has(Event event) const { return events_ & event; }

  // 設定模式按住連加要看「按住多久」
  const Button& front() const { return front_; }
  const Button& back() const { return back_; }

 private:
  struct TrackedButton : Button {
    explicit TrackedButton(uint8_t pin) : Button(pin) {}
    bool combo = false;       // 這次按住期間另一顆也按過
    bool hold_fired = false;  // 這次按住已經觸發過長按
  };

  void CheckHold(TrackedButton& button, unsigned long hold_ms, Event event) {
    if (button.IsPressed() && !button.combo && !button.hold_fired &&
        button.PressedDurationMs() > hold_ms) {
      button.hold_fired = true;
      events_ |= event;
    }
  }

  TrackedButton back_;
  TrackedButton front_;
  const unsigned long front_hold_ms_;
  const unsigned long back_hold_ms_;
  const unsigned long both_hold_ms_;
  uint8_t events_ = 0;
  bool both_fired_ = false;
};

#endif  // BUTTON_INPUT_H
