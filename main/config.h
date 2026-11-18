#ifndef CONFIG_H
#define CONFIG_H

// 所有可以調的設定都在這裡; 改完重新燒錄即可。
// 只給 nixie_tube_clock.cpp include (const 只在 include 它的檔案裡看得到)

#include <Arduino.h>

#include "tube_driver.h"

//******************************** 腳位 ********************************
const int kBackButtonPin = A1;     // 後面按鈕: 長按設定時間 / 設定模式下切換時分秒 (不能只寫 1)
const int kFrontButtonPin = A2;    // 前面按鈕: 長按設定亮度 / 設定模式下 +1
const int kColonLedPin = 12;       // 冒號 LED, 與「時」那顆 74HC595 的 STCP (大平台) 共用
const int kRandomSeedPin = A7;     // Nano 的 A7 沒接東西, 讀到的雜訊拿來當亂數種子

const int kRtcEnablePin = 4;       // DS1302
const int kRtcClockPin = 2;
const int kRtcDataPin = 3;

// 74HC595: STCP (大平台) / SHCP (活塞) / DS (資料)
const int kSecondLatchPin = 6, kSecondClockPin = 7, kSecondDataPin = 5;
const int kMinuteLatchPin = 9, kMinuteClockPin = 10, kMinuteDataPin = 8;
const int kHourLatchPin = 12, kHourClockPin = 13, kHourDataPin = 11;

//******************************** 按鈕 ********************************
const unsigned long kEnterSettingHoldMs = 2000;   // 長按 back 2 秒進入設定時間
const unsigned long kManualCleanHoldMs = 1000;    // 兩顆一起長按 1 秒清理管子
const unsigned long kSettingTimeoutMs = 8000;     // 設定時間 8 秒沒按鍵就回到正常模式

// 設定時間按住 front 連續加 (跟鍵盤按住連打一樣): 按下先 +1, 按住 0.5 秒開始連加,
// 一開始每 200ms 加 1, 之後每加一次間隔縮短 15%, 按住約 1.3 秒後到最快每 50ms 加 1 (每秒 20 下), 放開就停
const unsigned long kRepeatDelayMs = 500;
const unsigned long kRepeatStartIntervalMs = 200;
const unsigned long kRepeatMinIntervalMs = 50;
const uint8_t kRepeatSpeedUpPercent = 85;  // 下一次間隔 = 這一次 x 85%

// 設定時間時正在設定的管子閃爍: 一個週期 400ms, 亮 240ms 暗 160ms (約每秒閃 2.5 次)
const unsigned long kBlinkPeriodMs = 400;
const unsigned long kBlinkOnMs = 240;

//******************************** 亮度 ********************************
// 正常模式下長按 front 2 秒進入, 管子顯示目前亮度 (例如 "80");
// front 亮一階、back 暗一階, 5 秒沒按就回到時間。每次調整馬上存進 EEPROM。
// 亮度是用「縮短亮的時間」做的 (電流不變, 見 tube_driver.h), 損耗大致跟亮度成正比
const unsigned long kEnterBrightnessHoldMs = 2000;
const unsigned long kBrightnessTimeoutMs = 5000;
const uint8_t kBrightnessDefault = 80;  // 第一次開機的亮度: 肉眼幾乎看不出比 100% 暗
const uint8_t kBrightnessMin = 20;      // 每輪亮 1.5ms; 再暗白天會看不清楚
const uint8_t kBrightnessMax = 100;
const uint8_t kBrightnessStep = 5;      // = TubeDriver 一格, 剛好不用四捨五入
const int kBrightnessEepromAddress = 64;  // EEPROM 0 開始的那段給 drift_comp 用

//******************************** 清管 ********************************
// 自動清管 (防陰極中毒): 每隔 15~60 秒 (隨機) 把每個數字快速亮過兩輪
// 每步 70ms 起、每步快 1ms -> 一次約 1.2 秒
const unsigned long kAutoCleanMinSeconds = 15;
const unsigned long kAutoCleanMaxSeconds = 60;
const uint8_t kTubeCleanSteps = 20;     // 10 個數字 x 2 輪, 每個數字亮的時間一樣
const float kCleanFirstStepMs = 70;
const float kCleanStepSpeedUpMs = 1.0;
// 清管時每一步顯示的數字 (時分秒都一樣), 由管子的後到前
const byte kTubeShowNumbers[10] = {11, 66, 22, 55, 0, 44, 77, 99, 88, 33};

//******************************** 解碼秀 ********************************
// 開機、USB 對時後各跑一次 (要的話也可以整點跑, 見 kChime), 兼開機自我檢查與清管:
//  1. 掃描: 6 支管子一起把 0~9 每個數字亮一次 (開機時方便看哪個數字不亮)
//  2. 解碼: 亂數狂跳, 再從左到右一支一支鎖定成現在時間
//  3. 閃斷: 整排熄滅/亮起兩下, 像舊螢幕開機
// 要慢慢檢查每個數字, 把 kBootSweepStepMs 改成 1500 即可
const bool kBootAnimation = true;
const unsigned long kBootSweepStepMs = 105;   // 開機掃描: 每個數字停留時間 (0 -> 9 順序, 方便除錯)
const uint8_t kBootSweepDigits[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
const unsigned long kBootScrambleMs = 35;     // 解碼: 亂數每幾 ms 換一次
const uint8_t kBootFramesPerLock = 4;         // 解碼: 亂跳幾次後鎖定下一支管子
const unsigned long kBootGlitchMs = 50;       // 閃斷: 每次熄滅/亮起的時間

// 整點報時: 每隔這麼多分鐘 (在 0 秒那一刻) 跑一次解碼秀, 60 = 只在整點
const bool kChime = false;  // 已經有每 15~60 秒的自動清管, 預設關閉; 想要整點報時改 true
const uint8_t kChimeEveryMinutes = 60;
const unsigned long kChimeSweepStepMs = 80;
const uint8_t kChimeSweepDigits[10] = {1, 6, 2, 5, 0, 4, 7, 9, 8, 3};  // 由管子後到前, 像一層層往前推

//******************************** 顯示效果 ********************************
// 冒號 LED 呼吸燈 (軟體 PWM, 見 colon_led.h); false = 回到原本亮 1 秒暗 1 秒
const bool kColonBreathing = true;
// 換數字時淡入淡出 (見 tube_driver.h, 速度調 kFadeNormal / kFadeQuick); false = 立刻切換
const bool kDigitFade = true;
const uint8_t kTimeFade = kDigitFade ? TubeDriver::kFadeNormal : TubeDriver::kFadeInstant;
const uint8_t kCleanFade = kDigitFade ? TubeDriver::kFadeQuick : TubeDriver::kFadeInstant;

//******************************** 時間 ********************************
const uint8_t kRtcWriteTries = 3;            // 寫 RTC 後讀回不一致就重寫, 最多寫這麼多次
const unsigned long kSyncDateShowMs = 1200;  // USB 對時成功: 先顯示日期 (YY MM DD) 這麼久, 再解碼成時間
// 誤差補償 (見 drift_comp.h): 每分鐘的這一秒檢查要不要撥 1 秒; 避開 59->00, 不會牽動分鐘
const bool kDriftCompensation = true;
const uint8_t kDriftFixSecond = 30;

//******************************** 除錯 ********************************
// 詳細診斷日誌 ([ADD]/[SHOW] 印出 RTC 讀回值與送進 595 的位元)。
// 9600 baud 一秒只能送約 960 個字, 打開時按住快速連加會被序列埠拖慢, 平常保持 false
const bool kVerboseLog = false;

#endif  // CONFIG_H
