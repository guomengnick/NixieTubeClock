#ifndef BUTTON_H
#define BUTTON_H

#include <Arduino.h>

// 非阻塞的按鈕防抖 (millis() based debounce)
// 按鈕一端接腳位、一端接 GND，按下時為 LOW。
// 每個 loop 只呼叫一次 Update()，之後用 IsPressed()/WasPressed() 讀取穩定後的狀態。
class Button {
 public:
  // debounce_ms: 讀值要連續穩定這麼久才算數
  // min_release_ms: 放開後至少要這麼久才接受下一次按下 (擋掉微動開關放開時的長彈跳)
  explicit Button(uint8_t pin, unsigned long debounce_ms = 50,
                  unsigned long min_release_ms = 120)
    : pin_(pin), debounce_ms_(debounce_ms), min_release_ms_(min_release_ms) {}

  void Begin() {
    // 開啟內建上拉 (約 20k~50k)，就算外部上拉電阻沒接好，腳位也不會浮空
    pinMode(pin_, INPUT_PULLUP);
    raw_ = stable_ = ReadRaw();
    raw_changed_ms_ = stable_since_ms_ = millis();
  }

  void Update() {
    const unsigned long now = millis();
    const bool reading = ReadRaw();
    pressed_edge_ = false;
    released_edge_ = false;

    if (reading != raw_) {  // 讀值有變化 (可能是彈跳)，重新計時
      raw_ = reading;
      raw_changed_ms_ = now;
      return;
    }
    // 讀值連續穩定 debounce_ms_ 以上，才承認狀態改變
    if (reading != stable_ && now - raw_changed_ms_ >= debounce_ms_) {
      // 剛放開沒多久又「按下」, 視為彈跳而不是新的一下
      if (reading && now - stable_since_ms_ < min_release_ms_) {
        return;
      }
      stable_ = reading;
      stable_since_ms_ = now;
      if (stable_) {
        pressed_edge_ = true;
      } else {
        released_edge_ = true;
      }
    }
  }

  bool IsPressed() const { return stable_; }
  bool WasPressed() const { return pressed_edge_; }    // 這一輪剛按下
  bool WasReleased() const { return released_edge_; }  // 這一輪剛放開
  unsigned long PressedDurationMs() const {
    return stable_ ? millis() - stable_since_ms_ : 0;
  }

 private:
  // digitalRead 的 LOW 門檻約 1.5V (5V 供電)，比 analogRead() < 50 (約 0.24V) 寬鬆很多
  bool ReadRaw() const { return digitalRead(pin_) == LOW; }

  const uint8_t pin_;
  const unsigned long debounce_ms_;
  const unsigned long min_release_ms_;
  bool raw_ = false;
  bool stable_ = false;
  bool pressed_edge_ = false;
  bool released_edge_ = false;
  unsigned long raw_changed_ms_ = 0;
  unsigned long stable_since_ms_ = 0;
};

#endif  // BUTTON_H
