#ifndef SHIFT_REGISTER_H
#define SHIFT_REGISTER_H
class ShiftRegister {
 public:
  ShiftRegister(int latch_pin, int clock_pin, int data_pin) :
    latch_pin_(latch_pin), clock_pin_(clock_pin), data_pin_(data_pin),
    numbers_({0b0000, 0b0001, 0b0010, 0b0011, 
      0b0100, 0b0101, 0b0110, 0b0111, 0b1000, 0b1001, 0b1111}),
    last_time_(-1){
      pinMode(latch_pin_, OUTPUT);
      pinMode(clock_pin_, OUTPUT);
      pinMode(data_pin_, OUTPUT);
  }

  // 设置时钟显示，接收一个整数，左边的輝光管显示十位数，右边的輝光管显示个位数
  void SetClockTime(int time) {
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
    SendBinaryData(binary_time);
  }
  // 最後一次送進 74HC595 的 byte (Q7..Q0), 給診斷日誌用; 還沒送過是 -1
  int LastData() const { return last_time_; }
 private:
  void SendBinaryData(int data) {
    if(last_time_ == data){
      return;
    }
    last_time_ = data;
    // 「時」的 latch (pin 12) 也是冒號 LED, 呼吸燈的中斷會一直拉高拉低它;
    // 送資料這約 0.1ms 先暫停中斷, 免得中途被拉高、把送到一半的資料鎖存到管子上
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

  const int latch_pin_; // Latch pin STCP 大平台
  const int clock_pin_; // Clock pin SHCP 活塞
  const int data_pin_;  // Data pin  DS 資料
  int last_time_;
};

#endif // SHIFT_REGISTER_H

