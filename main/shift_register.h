#ifndef SHIFT_REGISTER_H
#define SHIFT_REGISTER_H

#include <Arduino.h>

#include "tube_driver.h"

class ShiftRegister {
 public:
  // id: 這顆 595 在 TubeDriver 裡的編號 (0 秒 / 1 分 / 2 時)
  ShiftRegister(uint8_t id, int latch_pin, int clock_pin, int data_pin) :
    id_(id), latch_pin_(latch_pin), clock_pin_(clock_pin), data_pin_(data_pin),
    numbers_({0b0000, 0b0001, 0b0010, 0b0011, 
      0b0100, 0b0101, 0b0110, 0b0111, 0b1000, 0b1001, 0b1111}),
    last_time_(-1){
      pinMode(latch_pin_, OUTPUT);
      pinMode(clock_pin_, OUTPUT);
      pinMode(data_pin_, OUTPUT);
  }

  // 设置时钟显示，接收一个整数，左边的輝光管显示十位数，右边的輝光管显示个位数
  // fade_ticks: 淡入速度 (TubeDriver::kFadeNormal 等), 預設立刻切換; TubeDriver 還沒啟動時一律直接切換
  void SetClockTime(int time, uint8_t fade_ticks = TubeDriver::kFadeInstant) {
    int binary_time;
    if(time == 255){//個位數、十位數都on 代表不顯示
      binary_time = 255;
    }
    else{
      const int tens = time / 10; // 计算十位数
      const int ones = time % 10; // 计算个位数
      binary_time = 
          (DecimalToBinary(ones) << 4) | DecimalToBinary(tens); 
    }
    SendBinaryData(binary_time, fade_ticks);
  }
  // 最後一次要求顯示的 byte (Q7..Q0), 給診斷日誌用; 還沒送過是 -1
  int LastData() const { return last_time_; }
  int latch_pin() const { return latch_pin_; }
  int clock_pin() const { return clock_pin_; }
  int data_pin() const { return data_pin_; }
 private:
  void SendBinaryData(int data, uint8_t fade_ticks) {
    if(last_time_ == data){
      return;
    }
    last_time_ = data;
    if (TubeDriver::Running()) {  // 交給中斷送, 才能淡入淡出
      TubeDriver::Show(id_, data, fade_ticks);
      return;
    }
    // TubeDriver 還沒啟動時直接送; 暫停中斷免得被別的中斷打斷到一半
    noInterrupts();
    digitalWrite(latch_pin_, LOW); // 开始数据传输
    shiftOut(data_pin_, clock_pin_, MSBFIRST, data); // 发送数据
    digitalWrite(latch_pin_, HIGH); // 完成数据传输
    interrupts();
  }
  byte numbers_[11];

  byte DecimalToBinary(int number) {
    return numbers_[number];
  }

  const uint8_t id_;
  const int latch_pin_; // Latch pin STCP 大平台
  const int clock_pin_; // Clock pin SHCP 活塞
  const int data_pin_;  // Data pin  DS 資料
  int last_time_;
};

#endif // SHIFT_REGISTER_H

