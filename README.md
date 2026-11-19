# 輝光管時鐘製作記錄 | Nixie Tube Clock Project


![時鐘成品](https://github.com/user-attachments/assets/4f422308-13c9-403f-8024-2359b2cda493)

用 Arduino Nano、6 支 IN-12 輝光管和 DS1302 手工做的時鐘。這份 README 前半是韌體功能與使用方式，後半是從零開始的製作記錄。  
A hand-built clock using an Arduino Nano, six IN-12 nixie tubes and a DS1302 RTC. The first half of this README covers the firmware and how to use it; the second half is the build log from scratch.

## 功能一覽 | Features at a Glance

- **換分鐘電梯效果**：有變的管子從 0 走到 9，再走回新數字。  
  **Elevator digit change**: every tube that changes climbs from 0 to 9, then steps back down to the new digit.
- **四種清管動畫隨機輪播**：防止陰極中毒，平常不會亮的數字會多亮一下。  
  **Four random anti-poisoning animations**: digits the clock never normally shows get extra glow time.
- **淡入淡出換字、冒號呼吸燈、開機解碼秀**。  
  **Crossfading digits, a breathing colon and a boot "decode" animation.**
- **短按看日期、可調亮度**（存在 EEPROM，降低亮度可以延長管子壽命）。  
  **Tap to show the date, adjustable brightness** (saved to EEPROM; lower brightness extends tube life).
- **USB 一鍵對時，並自動學習、補償 DS1302 的快慢**。  
  **One-click USB time sync, with automatic learning and correction of DS1302 drift.**

<!-- GIF 放這裡 | GIFs go here
| 換分鐘電梯 Elevator | 清管接力 Relay cleaning |
|---|---|
| ![elevator](GIF連結) | ![relay](GIF連結) |
| 開機解碼秀 Boot decode | 看日期 Date |
| ![boot](GIF連結) | ![date](GIF連結) |
-->

## 目錄 | Contents

- [功能與操作 | Features and Controls](#功能與操作--features-and-controls)
- [硬體需求 | Hardware Requirements](#硬體需求--hardware-requirements)
- [軟體需求 | Software Requirements](#軟體需求--software-requirements)
- [開發與上傳流程 | Development and Upload Process](#開發與上傳流程--development-and-upload-process)
- [製作記錄 | Build Log](#前言--introduction)
- [總材料 | Materials List](#總材料--materials-list)
- [總配線流程 | Wiring Overview](#總配線流程--wiring-overview)
- [做完後發現可以改善的地方 | Lessons Learned](#做完後發現可以改善的地方--lessons-learned)

## 硬體需求 | Hardware Requirements
- **Arduino Nano**：作為控制輝光管顯示的微控制器。  
  **Arduino Nano**: Used as the microcontroller to control the nixie tube display.
- **輝光管（IN-12 型號）**：作為時鐘顯示的主要元件，共需 6 支。  
  **Nixie Tube (IN-12 model)**: Main component for the clock display, requires 6 tubes.
- **杜邦線**：電路板到輝光管座的連接，共需 60 條。  
  **Dupont Wire**: connections from the circuit board to the nixie tube sockets, 60 pieces in total.
- **K155ID1 驅動晶片**：輝光管專用驅動晶片，共需 6 顆。  
  **K155ID1 Driver Chip**: A driver chip for nixie tubes, requires 6 chips.
- **74HC595 移位寄存器**：擴展 Arduino 的 IO 腳位，共需 3 顆。  
  **74HC595 Shift Register**: Expands Arduino’s IO pins, requires 3 chips.
- **DS1302 實時時鐘模組**：用於提供穩定的時間信息。  
  **DS1302 Real-Time Clock Module**: Provides stable time information.
- **5V-9V 升壓模組**：將輸入的 5V 電壓升壓至 170V，供應輝光管需求。  
  **5V-9V Boost Module**: Boosts input voltage from 5V to 170V to power nixie tubes.
- **LED 及電阻**：用於冒號顯示（4 顆 LED，配 470 歐姆電阻）。  
  **LEDs and Resistors**: Used for colon display (4 LEDs with 470-ohm resistors).
- **各種電容及電阻**：包含 10K 歐姆電阻、電容（根據需求選擇）。  
  **Capacitors and Resistors**: Includes 10K-ohm resistors and capacitors as needed.


## 軟體需求 | Software Requirements

- **Arduino IDE**：用於撰寫和上傳程式碼到 Arduino Nano。下載：[Arduino 官方網站](https://www.arduino.cc/en/software)  
  **Arduino IDE**: for editing and uploading the code to the Nano. Download: [Arduino website](https://www.arduino.cc/en/software)
- **時鐘韌體**：都在 `main/` 資料夾，分成好幾個檔案（見下方的[程式結構](#程式結構--code-layout)）。用 Arduino IDE 開啟 `main/main.ino`，同資料夾的檔案會一起編譯、上傳。  
  **Clock firmware**: everything is in the `main/` folder, split into several files (see [Code Layout](#程式結構--code-layout)). Open `main/main.ino` in the Arduino IDE and the other files in that folder are compiled and uploaded with it.
- **DS1302 RTC 程式庫**：用來讀寫 DS1302 實時時鐘模組，已經放在本專案的 `libraries/Ds1302`。  
  **DS1302 RTC library**: reads and writes the DS1302 module; already included in `libraries/Ds1302`.
- **USB 對時腳本**：`tools/sync_time.bat`（Windows，用內建的 PowerShell，不用另外安裝）。  
  **USB sync script**: `tools/sync_time.bat` (Windows; uses the built-in PowerShell, nothing extra to install).



---

## 功能與操作 | Features and Controls

### 按鈕 | Buttons

| 操作 Action | 功能 Function |
|---|---|
| 長按**後面**按鈕 2 秒<br>Hold **back** button 2 s | 設定時間：後鍵切換 秒→分→時，前鍵 +1（秒是歸零），按住會越加越快；8 秒沒按自動離開<br>Set time: back cycles second → minute → hour, front adds 1 (second resets to 0), hold to repeat faster; exits after 8 s idle |
| 長按**前面**按鈕 2 秒<br>Hold **front** button 2 s | 設定亮度：前鍵 +5%、後鍵 -5%（20%~100%，預設 80%），存進 EEPROM；5 秒沒按自動離開<br>Set brightness: front +5 %, back −5 % (20–100 %, default 80 %), saved to EEPROM; exits after 5 s idle |
| 短按**前面**按鈕<br>Tap **front** button | 看日期 YY MM DD，3 秒後淡回時間（再按一下提早回去）<br>Show the date YY MM DD for 3 s (tap again to go back early) |
| 兩顆一起按 1 秒<br>Hold both 1 s | 手動清管<br>Clean the tubes manually |

### 顯示效果 | Display

- **開機解碼秀**：0~9 掃一次（順便檢查每個數字有沒有亮），亂數狂跳後由左到右鎖定成現在時間。  
  **Boot animation**: sweeps 0–9 (a quick self-test), then scrambles and locks onto the current time left to right.
- **淡入淡出換字**、**冒號呼吸燈**：都是用 Timer2 中斷做的軟體 PWM。  
  **Crossfading digits** and a **breathing colon**, both done with software PWM on a Timer2 interrupt.
- **換字掃描（電梯）**：換分鐘時，時、分有變的管子先跳到 0，一階一階往上走到 9，再往下走回新數字停住（例如 5 → 6：0 1 2 3 4 5 6 7 8 9 8 7 6），每個數字都亮過、順便清管。  
  **Digit sweep (elevator)**: on each new minute, every hour/minute tube that changes jumps to 0, climbs to 9, then steps back down to its new digit (e.g. 5 → 6: 0 1 2 … 9 8 7 6).
- **亮度**：電流不變，只縮短每輪亮的時間（每秒 133 輪），輝光一樣包滿陰極；損耗大致跟亮度成正比。  
  **Brightness**: same current, shorter on-time per cycle (133 Hz), so the glow still covers the whole cathode; wear scales roughly with brightness.
- **自動清管**：每 1~3 分鐘讓每支管子的 0~9 都亮過，防止陰極中毒。每次隨機挑一種動畫：流動（6 支一起快速流過兩輪）、電梯（一起從 0 走到 9 再走回現在時間）、波浪（由左到右一支接一支走）、接力（一次只有一支從 0 走到 9，到頂淡回時間、下一支接著走，其他管子照常顯示時間）。平常不會亮的數字（時十位 3~9、分十位和秒十位 6~9）會多停一下，因為真正需要清的是它們。兩顆按鈕一起按 1 秒也一樣隨機。  
  **Anti-poisoning**: every 1–3 min every digit of every tube is lit, using a randomly chosen animation (flow / elevator / wave / relay); digits a 24-hour clock never shows get extra dwell time.

### USB 對時與誤差補償 | USB Time Sync and Drift Compensation

1. 時鐘用 USB 接上電腦，雙擊 `tools/sync_time.bat`（會自動找 CH340 的 COM 埠；Arduino IDE 的序列埠監控視窗要先關掉）。  
   Connect the clock over USB and double-click `tools/sync_time.bat` (it finds the CH340 port automatically; close the Arduino IDE serial monitor first).
2. 腳本在整秒那一刻送出電腦時間，時鐘先顯示日期再解碼成新時間，腳本會印出對時前快/慢了幾秒。  
   The script sends the PC time on the exact second; the clock shows the date, then decodes to the new time, and the script prints how far off it was.
3. **兩次對時隔 3 天以上**，時鐘就會算出 DS1302 每天快/慢多少，之後自己定期撥秒修正（關機期間的誤差開機時補回），資料存在 EEPROM。之後偶爾對時會繼續修正速率。  
   **Two syncs at least 3 days apart** let the clock learn how many seconds per day its DS1302 drifts; it then corrects itself (including time spent powered off) and stores the rate in EEPROM. Later syncs keep refining the rate.

也可以在序列埠監控視窗（9600 baud，行尾選「換行」）手動輸入：`?` 查詢時間、`T2026-11-18 10:00:00` 設定時間。  
You can also type in the serial monitor (9600 baud, newline): `?` to query, `T2026-11-18 10:00:00` to set.

### 程式結構 | Code Layout

| 檔案 File | 內容 Contents |
|---|---|
| `main/config.h` | 所有可調的設定與腳位 / every tunable setting and pin |
| `main/nixie_tube_clock.*` | 主邏輯：狀態機 (走時、看日期、設定時間/亮度)、USB 對時 / main logic: state machine (time, date, time/brightness setting), USB sync |
| `main/display.*` | 6 支管子要顯示什麼數字 / which digit each of the 6 tubes shows |
| `main/animations.*` | 清管、換字掃描、解碼秀 / cleaning, digit sweep, decode show |
| `main/button_input.h`, `main/button.h` | 兩顆按鈕 -> 短按/長按/一起按事件、防抖 / button gestures (tap, hold, both) and debounce |
| `main/tube_driver.*` | 中斷送 595：淡入淡出、亮度 / interrupt-driven 595 output: fades, brightness |
| `main/colon_led.*`, `main/fast_tick.*` | 冒號呼吸燈、Timer2 16 kHz 中斷 / breathing colon, Timer2 16 kHz interrupt |
| `main/time_sync.*`, `main/drift_comp.*` | USB 對時指令、DS1302 誤差補償 / USB sync commands, DS1302 drift compensation |
| `tools/sync_time.*` | 電腦端對時腳本 / PC-side sync script |

---

## 開發與上傳流程 | Development and Upload Process

1. **下載本專案**：克隆或下載 ZIP 文件。  
   **Download the project**: Clone or download the ZIP file.
   ```bash
   git clone https://github.com/guomengnick/NixieTubeClock.git
   ```
2. **安裝 DS1302 程式庫**：把 `libraries/Ds1302` 複製到 Arduino 的 libraries 資料夾。  
   **Install the DS1302 library**: copy `libraries/Ds1302` into your Arduino libraries folder.
3. **上傳**：用 Arduino IDE 開啟 `main/main.ino`，板子選 Arduino Nano 後上傳。  
   **Upload**: open `main/main.ino` in the Arduino IDE, select Arduino Nano, and upload.


## 開發注意 | Note on Development
1. 可能是因為我買的 Nano 板比較舊，編譯沒問題，但上傳一直失敗，後來發現處理器要選下圖這個模式（舊版 Bootloader）才可以，遇到同樣問題的可以參考。  
   My Nano may be an older board: compiling worked but uploads kept failing, until I selected the processor option shown below (old bootloader). Try it if you hit the same problem.
![截圖 2024-11-03 下午5 14 51](https://github.com/user-attachments/assets/8b411fa5-23a8-4aa2-9e48-a87963be53c2)





- [手搓輝光管 Arduino 時鐘 - B站影片](https://www.bilibili.com/video/BV1f31gYLEiL/?share_source=copy_web&vd_source=17521cbd088477c140b597f64646ffe3)
- [Clock video on Youtube](https://youtu.be/MlX_RdtVoo0)



目前成品放在辦公室，看著時間一分一秒流逝。為了讓正面只看得到元件、看起來比較整潔，焊接和線路都放在板子底部。  
The clock sits in my office, watching time tick by. To keep the front clean, all the wiring and soldering is hidden on the bottom of the PCB.

![orig (2)](https://github.com/user-attachments/assets/cc8a0b39-b5f0-42ad-be37-c314992d429f)

---

## 前言 | Introduction

想做個 side project，但不想只做純軟體的東西，偶然看到非常有歷史感的「輝光管」。輝光管是在玻璃管裡灌入氖氣，對裡面做成數字形狀的金屬（陰極）加上高電壓，金屬周圍就會發出橘色的光；仔細看會發現，發光的其實是金屬周圍的氣體。  
I wanted a side project that wasn't pure software, and came across the historic nixie tube. A glass tube is filled with neon gas; when a high voltage is applied to a digit-shaped metal cathode inside, the gas around it glows orange. Look closely and you'll see it's the gas around the metal that glows, not the metal itself.

看到網路上有人在做，心想應該不難、應該做得到。程式第一版大概一天就寫完了，但要控制的點位很多，連 LED 冒號都得跟別的腳位共用一支。材料一開始是在網路上買的，結果做的過程中還是陸續跑了 5 趟電子材料行才完成。  
I saw someone online building one and thought it couldn't be too hard. The first version of the code took about a day, but there are so many outputs to control that even the LED colon had to share a pin. I ordered the parts online at first, and still ended up making five trips to the electronics store before it was done.

所以順便記錄總共需要哪些東西，給也想試試看的人參考，可以少跑幾趟材料行。材料清單和原始碼都放在這裡，想復刻的可以參考。  
So I'm recording everything I used for anyone who wants to try, to save you a few trips to the store. The parts list and source code are all here for reference.

寫第一版的時候本來想把程式寫得很優雅，還替 74HC595 做了一個類別，結果寫到後來全部塞在 main 裡，當時想說「程式沒問題就不要再去動它」。後來還是動了：現在改成一個狀態機，顯示、動畫、按鈕各自拆成獨立的檔案（見[程式結構](#程式結構--code-layout)），也加了不少新功能。  
In the first version I meant to keep the code elegant and even wrote a class for the 74HC595, but in the end everything went into the main file, and I told myself "if it works, don't touch it." I touched it anyway: the code is now a state machine, with display, animations and buttons split into their own files (see [Code Layout](#程式結構--code-layout)), plus quite a few new features.



## Step 1. 製作輸入電壓 | Creating the Input Voltage

（下圖是改成電容以前的未完成品）  
(The photo below shows the unfinished build, before the regulator was replaced with capacitors.)

發生一件有趣的事：電源原本用 9V，再用 LM7805 穩壓成 5V 給後面的電路，這樣其實可以用，缺點是這顆 LM7805 時不時就在發燙。當初在材料行看到這個穩壓器覺得很酷，雖然明顯是殺雞用牛刀。  
Something funny happened here. At first I used a 9V supply and an LM7805 to regulate it down to 5V for the circuit. That worked, but the LM7805 kept getting hot. I just thought the regulator looked cool when I saw it at the store, even though it was clearly overkill.

時鐘做好放在辦公室一陣子後，剛好被老闆看到，他也是學電路的，建議我電源直接用 iPhone 豆腐頭就好，還說「我們後面很多電容，我拿一些給你」，於是穩壓器就被換掉了。補充一下：LM7805 輸入要比 5V 高大約 2V（約 7V 以上）才穩得住，所以豆腐頭的 5V 直接接 LM7805 是行不通的。最後就換成電容，畢竟老闆都給建議了，再不換就對不起老闆的電容。  
After the clock had been in the office for a while, my boss, who also studied electronics, suggested powering it straight from an iPhone charger, and offered me some capacitors from the office stock, so the regulator was replaced. Note: an LM7805 needs its input about 2V above 5V (roughly 7V or more) to regulate, so a 5V charger into an LM7805 won't work. In the end I switched to capacitors; after all, my boss had given advice and capacitors, so I had to use them~.

![orig](https://github.com/user-attachments/assets/a46b9b48-a974-49f3-a5ff-869a9af4d38f)


## Step 2. 輸入電壓處理 | Input Voltage Processing

豆腐頭進來的 5V 用電容濾波、穩住電壓。其實一顆電容就很夠了，但拿掉 LM7805 之後空間很多，就用了兩組電容。  
The 5V from the charger is filtered and steadied by capacitors. One capacitor would really be enough, but removing the LM7805 freed up plenty of space, so I used two sets.

備註：電容串聯是「耐壓」變高（但電容量會變小），並聯是「電容量」變大。例如兩顆 20V 的電容串聯，耐壓變 40V；兩顆 1000μF 的電容並聯，就是 2000μF。  
Note: capacitors in series raise the voltage rating (but lower the capacitance); in parallel they add capacitance. For example, two 20V capacitors in series are rated for 40V, and two 1000μF capacitors in parallel give 2000μF.

豆腐頭的電一進來，先經過保險絲，再經過一顆二極體（萬一豆腐頭出問題或接反時保護電路），最後才到電容。  
Power from the charger first goes through the fuse, then a diode (to protect the circuit if the charger misbehaves or is connected backwards), and only then to the capacitors.
![S__36790275](https://github.com/user-attachments/assets/7736ab03-bee6-4d40-a085-207b7d77fb3a)
![S__37027905_0](https://github.com/user-attachments/assets/0f40e4fb-0bc7-45ba-abf6-0a703e528fe6)


## Step 3. 5V 轉 170V 升壓模組 | The 5V to 170V Boost Module

豆腐頭提供的 5V 直流電，要升壓成輝光管需要的電壓（約 160V）。當時不想自己繞變壓器，板子上再加變壓器可能更難、更做不出來，所以直接買現成的升壓模組。  
The 5V DC from the phone charger has to be boosted to the voltage the nixie tubes need (around 160V). I didn't want to wind my own transformer, which would have made the board harder and riskier to build, so I bought a ready-made boost module.

模組是在淘寶買的。保險起見，照片上可以看到我把正面的接點都用絕緣膠帶貼起來：之前可能是焊點太大，一接電就冒火花，剛開始做就看到煙火秀。淘寶搜「5V-9V升170V小型升压模块」應該就找得到。  
I bought the module on Taobao. To be safe, as the photo shows, I covered the front contacts with insulating tape: a solder joint that was too large once sparked the moment I powered it on, a small firework show on day one. Search Taobao for "5V-9V升170V小型升压模块" (5V-9V to 170V small boost module).

![S__37027904_0](https://github.com/user-attachments/assets/f4a4b0fb-5be2-48bf-8bcf-9c342d86e2d4)



## Step 4. DS1302 時鐘模組 | The DS1302 RTC Module

DS1302 是很常見的「實時時鐘」（RTC, Real-Time Clock）晶片模組，能提供秒、分、時、日、月、年。靠一顆鈕扣電池就能撐好幾年，斷電後也會繼續走時；原理是用石英振盪器不斷數過了幾秒，跟以前電腦主機板上的時鐘是同樣的東西。  
The DS1302 is a common real-time clock (RTC) module that keeps seconds, minutes, hours, day, month and year. A coin cell keeps it running for years, even with the main power off. It counts time with a quartz oscillator, the same idea as the clock on an old PC motherboard.

順帶一提，預算夠的話，建議另外買好一點的 32.768 kHz 石英振盪器。我是買下圖這個模組拆下來用，結果一個月差了快十分鐘。  
If your budget allows, get a better 32.768 kHz crystal. I took the parts off the module shown below, and it drifted by almost ten minutes a month.

> **後來的改善**：韌體現在會自動補償 DS1302 的快慢。用 USB 對時兩次（隔 3 天以上），時鐘就會算出它每天快或慢幾秒，之後自己定期修正（見 [USB 對時與誤差補償](#usb-對時與誤差補償--usb-time-sync-and-drift-compensation)）。補償修的是平均的快慢；如果晶振一陣子快、一陣子慢，還是會有一些殘差，偶爾再對時一次就好。  
> **Later improvement**: the firmware now compensates for DS1302 drift. Sync over USB twice (at least 3 days apart) and the clock works out how many seconds a day it gains or loses, then corrects itself (see [USB Time Sync and Drift Compensation](#usb-對時與誤差補償--usb-time-sync-and-drift-compensation)). It corrects the average rate; if the crystal speeds up and slows down over time, some error remains, so sync again once in a while.

DS1302 的 Arduino 程式庫已經有人寫好，直接拿來用就行，也放在本專案的 `libraries/Ds1302`。  
An Arduino library for the DS1302 already exists; it's included in this repository under `libraries/Ds1302`.

![orig (3)](https://github.com/user-attachments/assets/e275c5a3-12a5-41e1-8329-d5fe9e044498)



如圖，我把買來的模組拆焊下來，再焊到自己的板子上。DS1302 模組包含鈕扣電池、32.768 kHz 石英振盪器和計時 IC 本身。  
As shown, I desoldered the parts from the module and soldered them onto my own board: the coin cell, the 32.768 kHz crystal and the timekeeping IC itself.
![S__37027915_0](https://github.com/user-attachments/assets/2c005f79-db04-4b9b-956b-f9e9ac6be0c6)


## Step 5. 保險絲 | Adding a Fuse

這一步其實應該排在最前面：5V 豆腐頭一進來就要先經過保險絲。因為時鐘做完半年才想到要寫文章，當初的配線順序都忘了，所以最後另外整理了[總配線流程](#總配線流程--wiring-overview)。至於保險絲盒為什麼放在正中間，單純是覺得放中間有種和諧感。  
This step really belongs first: the 5V from the charger should go through the fuse right away. I only wrote this up six months after finishing the clock and had forgotten the wiring order, so there's a separate [Wiring Overview](#總配線流程--wiring-overview) at the end. As for why the fuse holder sits in the middle: it just looked balanced there.

下圖有學過電路的應該不難理解：保險絲跟 LED 並聯。保險絲沒斷時，電都走保險絲；保險絲燒斷後，電改走 LED，LED 亮起來就是在告訴你短路了。  
The circuit below should be easy to follow if you've studied electronics: the fuse and an LED are in parallel. While the fuse is intact, current flows through it; once it blows, current flows through the LED instead, and the lit LED tells you something shorted.

保險絲要配多大，可以查資料或問 AI，但盡量不要配太小。我原本照每支管子的耗電算，用了 0.2A 的保險絲，結果保險絲本身的電阻造成壓降，讓 Arduino、升壓模組出了一連串問題，後來改成 2A 就正常了。實際該配多少我也沒什麼經驗，懂電路的朋友應該更清楚。  
For the fuse rating, look it up or ask an AI, but don't go too small. I first used a 0.2A fuse based on each tube's current, and the fuse's own resistance caused a voltage drop that led to a string of problems with the Arduino and the boost module. Switching to 2A fixed it. I don't have much experience here; people who know circuits will know better.
![S__37027906_0](https://github.com/user-attachments/assets/8df6d887-f09f-449d-8ed6-3e75ffbd98db)




## Step 6. 輝光管的 IC 控制 | IC Control for Nixie Tubes

每支管子有 0~9 共 10 個數字要控制，6 支就是 60 個腳位，Nano 根本沒有這麼多腳。解法是搭配下面的 K155ID1 和「74HC595 移位暫存器」：一顆 74HC595 只用 Arduino 的 3 支腳，就能輸出 8 個 bit；而 K155ID1 只要 4 個 bit 就能選數字（0000 是 0、0111 是 7、1000 是 8、1001 是 9）。所以一顆 74HC595 加兩顆 K155ID1，就能控制 2 支管子共 20 個腳位。  
Each tube has 10 digits (0–9), so six tubes need 60 outputs, far more than the Nano has. The fix is the K155ID1 driver plus a 74HC595 shift register: one 74HC595 gives 8 output bits from just 3 Arduino pins, and a K155ID1 selects a digit from only 4 bits (0000 = 0, 0111 = 7, 1000 = 8, 1001 = 9). So one 74HC595 with two K155ID1s drives 2 tubes, 20 pins in total.

一顆 74HC595 的 8 個 bit 剛好分給兩支管子。在我的接線裡，**高 4 位是個位、低 4 位是十位**，所以顯示 88 送出 `10001000`，顯示 24 則是個位 4（`0100`）在前、十位 2（`0010`）在後，送出 `01000010`。這部分寫在 `main/display.cpp`，如果你的接線順序不一樣，改那裡就好。  
The 8 bits of one 74HC595 are split between two tubes. In my wiring the **high 4 bits are the ones digit and the low 4 bits the tens digit**, so "88" is `10001000` and "24" is the ones digit 4 (`0100`) followed by the tens digit 2 (`0010`): `01000010`. This lives in `main/display.cpp`; if your wiring is different, change it there.

每支輝光管也要記得配一個 10K 電阻。下面是我找到國外有人用這兩種 IC 搭配的接線圖：左邊紅框是 1 個 10K 電阻 + 1 支管子 + 1 顆 K155ID1 + 半顆 74HC595，可以控制一支管子（一個數字）。Arduino 控制的是 74HC595，74HC595 再去控制 K155ID1。  
Each nixie tube also needs a 10K resistor. Below is a schematic I found from another builder using these two ICs: the red box on the left is one 10K resistor + one tube + one K155ID1 + half a 74HC595, which drives one tube (one digit). The Arduino talks to the 74HC595, which in turn drives the K155ID1.

![15611377040_c15f4eb0b9_o-2](https://github.com/user-attachments/assets/9ca2292b-cf2c-4ac0-84a6-3bf781130575)
- [圖片來源 | Image source](https://www.plingboot.com/arduino-nixie-gps-clock-part-2/)

下圖可以看到，一顆 74HC595 控制 2 顆 K155ID1，進而控制兩支管子。  
In the photo below, one 74HC595 drives two K155ID1 chips, which drive two tubes.
  ![S__37027910_0](https://github.com/user-attachments/assets/801da5ae-00f2-4caa-9962-8466912223fb)

74HC595 的用法和原理，YouTube 搜「【Arduino教學】實作08-74HC595(上)」，講得蠻清楚的。  
For an English explanation, search YouTube for "how to use 74HC595".


**腳位不夠用的問題**：下圖是 Nano 的腳位，D 開頭的是數位輸出（輸出 0 或 1）。時、分、秒各一顆 74HC595，每顆要佔 3 支腳（哪 3 支看上面的教學影片），一共 9 支，再加上 RTC 模組，冒號 LED 就沒有腳可以用了。我的做法是讓冒號 LED 跟「時」那顆 74HC595 的鎖存腳（STCP，D12）共用一支腳。這樣可行的原因是：拉高拉低鎖存腳，只會把 74HC595 裡同一份資料再鎖存一次，不會改變管子顯示的數字。現在的韌體把送資料給 74HC595 和控制冒號呼吸燈放在同一個計時中斷裡，兩者不會互相干擾。另外聽說三顆 74HC595 的資料腳和時脈腳也可以共用，只要鎖存腳分開就好，這樣能省更多腳位，不過我當時沒有這樣做。  
**Running out of pins**: the diagram below shows the Nano's pins; the D pins are digital outputs (0 or 1). Hours, minutes and seconds each have a 74HC595 that takes 3 pins (see the tutorial above for which), 9 in total, and with the RTC module there's no pin left for the colon LED. My solution was to share the hour 74HC595's latch pin (STCP, D12) with the colon LED. This works because toggling the latch only re-latches the same data already in the 74HC595, so the displayed digits don't change. The current firmware drives both the 74HC595 updates and the breathing colon from the same timer interrupt, so they never interfere. I've also heard that the data and clock pins can be shared across all three 74HC595s as long as each has its own latch pin, which would save even more pins, but I didn't do that in this build.
![orig (4)](https://github.com/user-attachments/assets/f79535f7-3457-4811-bfcc-703933555540)
![S__37027910_0](https://github.com/user-attachments/assets/d4c44734-689e-4280-af34-111a5f8d37cf)


## Step 7. 輝光管支架 | Making the Nixie Tube Stand

為了讓管子好好站著，當初想了很多方法，最後用了簡單又暴力的做法（身邊剛好沒有時鐘，只好用影片截圖）：把一根螺絲固定在 PCB 上，再固定到管子的管座。難的是 6 支要保持同樣的角度、高度和彎度，而且要先把螺帽鎖上再凹，不然凹完螺帽就鎖不上去了。螺絲比較粗，PCB 要另外開孔，我當初是用美工刀鑿出來的。  
To keep the tubes upright I tried many ideas and ended up with a simple, brute-force one (I didn't have the clock with me, hence the video screenshot): a screw is fixed to the PCB and then to the tube socket. The hard part is getting all six to the same angle, height and bend, and the nuts have to go on before bending, or they won't fit afterwards. The screws are thick, so the PCB needs bigger holes; I carved mine with a utility knife.

這就很考驗凹螺絲的功力了，我當初凹了 8 根，挑 6 根比較像的，總之就是歪歪扭扭地凹好了。  
This really tested my bending skills: I bent eight and picked the six that matched best.

至於為什麼要用管座：一方面輝光管有壽命，用管座以後換管子比較方便；另一方面管座上才有孔可以固定管子。  
Why use sockets: tubes wear out, and sockets make them easy to replace; the sockets also provide holes to mount the tubes.

![orig (4)](https://github.com/user-attachments/assets/57d29c63-13c2-4e1f-84be-befa49889f07)

可以看到我用鉗子凹螺絲留下的痕跡；管座前後各用一個螺帽固定。  
You can see the plier marks from bending the screws; each socket is held by two nuts, one in front and one behind.
![S__37027903_0](https://github.com/user-attachments/assets/593793f8-9f2b-4d3b-b2d3-ba11920a0775)


螺絲頭在 PCB 底部的樣子  
The screw heads on the bottom of the PCB
![S__37027922_0](https://github.com/user-attachments/assets/a58173db-1d54-4977-8db7-7ef2be759b53)



## Step 8. 配線 | Wiring

接下來就是燒腦的配線。配完才發現單芯線應該買耐熱的，不然烙鐵碰久一點，白色外皮就會燒黑、甚至脫落。焊接記得用助焊劑，元件針腳和單芯線才會融得比較好。  
Next came the brain-burning part: wiring. Afterwards I realized I should have bought heat-resistant solid-core wire; with ordinary wire, the white insulation blackens or even falls off if the iron stays on too long. Use flux so the component pins and wires solder together properly.

其實第一版配線沒有這條彎線：後來把 LM7805 換成電容，需要一條比較長的線，就任性地把最後一條線繞成「黃金比例」。  
The first version of the wiring didn't have this curly wire. After swapping the LM7805 for capacitors I needed a longer wire, so I indulged myself and coiled it into a "golden ratio" spiral.

![orig (2)](https://github.com/user-attachments/assets/ebab3a19-8043-4458-a61b-5f92bc82b986)

![S__37027921_0](https://github.com/user-attachments/assets/69445090-58d3-4d9c-9328-566a04181443)

從底板到輝光管座的連接線，兩端的接法如下：  
How I connected both ends of the wires from the base board to the tube sockets:
- 底板端：杜邦線剪掉一端、上錫，焊在 PCB 上。  
  Base board side: cut off one end of the Dupont wire, tin it, and solder it to the PCB.
- 管座端：拆掉杜邦線的塑膠外殼，套上熱縮管，就可以插到管座上。  
  Socket side: remove the Dupont wire's plastic housing, cover it with heat-shrink tubing, and plug it into the tube socket.
<img width="862" height="565" alt="eb223592-8ba8-457f-9854-c33d29ecfc9a" src="https://github.com/user-attachments/assets/4f695c3f-baf9-43b5-b7eb-2124bc441480" />
<img width="948" height="684" alt="1f7fa4b1-5d7b-4cc7-94fa-6bda2b46f75b" src="https://github.com/user-attachments/assets/5baf1146-16d0-439f-b30d-fbfdce6325a4" />

- 杜邦線顏色和數字的對應：我照電阻色碼來配，數字顯示錯了比較好查。這點看個人喜好，也可以全部用單色線，跟底板風格一致。  
  杜邦線「黑棕紅橙黃綠藍紫黑白」依序對應管座的數字「1234567890」。建議多買幾條，焊壞或剪太短時有得換，我買了大約 70 條。  
  Wire colors: I followed the resistor color code so wrong digits are easier to debug. That's just my preference; single-color wires that match the base board work too.  
  The colors black, brown, red, orange, yellow, green, blue, purple, black, white map to socket digits 1 2 3 4 5 6 7 8 9 0. Buy a few spares in case some get damaged or cut too short; I bought about 70.
<img width="1078" height="239" alt="80073179-0508-4deb-8b4e-f3b38d098fb8" src="https://github.com/user-attachments/assets/2d8e2137-b60d-486c-8389-3f08c4eaf1a1" />




## Step 9. 按鍵 | Making the Buttons

要設定時間，也想要能手動清管，所以挑了按起來比較舒服的微動開關。這種開關的針腳比較大，只好在 PCB 上開孔。（圖片左下角就是按鈕）  
I needed buttons to set the time and to clean the tubes manually, so I picked micro switches that feel nice to press. Their pins are large, so I had to enlarge the holes in the PCB. (The button is in the lower left of the photo.)

微動開關要配一個電阻。雖然 Nano 本身就有內建的上拉電阻可以用，我還是另外加了。接法是一條線接到 Nano 的類比輸入腳，偵測現在是 5V 還是接近 0V。按鈕的款式可以去材料行看個人喜好，我是挑跟時鐘大小搭配的，缺點是針腳很大，得修改板子，我就直接拿刀在 PCB 上挖洞。  
Each micro switch needs a resistor. The Nano has built-in pull-up resistors, but I added external ones anyway. One wire goes to a Nano analog input, which reads whether it's at 5V or near 0V. Pick whatever button you like at the store; I chose one that matched the clock's size, but its pins were large, so I cut the holes in the PCB with a knife.

![81+GMLjeZYL](https://github.com/user-attachments/assets/fd728390-f505-4064-a8bc-03a3b1911e8c)

兩顆按鈕接在 Nano 的 A1（後面）和 A2（前面），腳位定義在 `main/config.h`。韌體會做防抖，再把按法轉成「短按、長按、兩顆一起按」，各自的功能見[按鈕](#按鈕--buttons)。  
The two buttons go to A1 (back) and A2 (front) on the Nano; the pins are defined in `main/config.h`. The firmware debounces them and turns presses into "tap", "hold" and "both held" gestures; see [Buttons](#按鈕--buttons) for what each one does.
![S__37027908_0](https://github.com/user-attachments/assets/83b2ab4c-82e3-40d7-a431-e7bb22f7bb4d)
![S__37027920_0](https://github.com/user-attachments/assets/b84577bd-d403-4861-8f01-2661e87636f7)



## Step 10. 總開關 | Creating the Main Power Switch

輝光管的壽命通常是幾萬小時。韌體裡已經有「清管」：每隔 1～3 分鐘讓每支管子的 0～9 都亮一遍。這是為了防止**陰極中毒**：一直亮著的數字會把金屬濺鍍到旁邊很少亮的陰極上，久了那些數字就會亮不全。對時鐘來說，最容易中毒的是平常不會出現的數字，例如時的十位只會顯示 0～2，分和秒的十位只會顯示 0～5，所以清管時會讓這些數字多亮一下。另外調低亮度也能延長壽命。  
Nixie tubes typically last tens of thousands of hours. The firmware already "cleans" the tubes, lighting every digit of every tube every 1–3 minutes. This prevents **cathode poisoning**: the digit that's lit sputters metal onto nearby cathodes that are rarely lit, and over time those digits stop lighting fully. On a clock, the digits most at risk are the ones that never come up, e.g. the hour tens only shows 0–2 and the minute/second tens only 0–5, so the cleaning gives those digits extra time. Lowering the brightness also extends tube life.

但我希望這個看起來很老的東西能比我還長壽，所以又加了一個總開關，每天上班打開、下班關掉，讓它變成一台「手動檔時鐘」。圖片右下角的紅色開關就是總開關。  
But I'd like this old-looking thing to outlive me, so I added a main power switch as well. I turn it on when I get to work and off when I leave, making it a "manual" clock. The red switch in the lower right of the photo is the main switch.

![orig (6)](https://github.com/user-attachments/assets/504dc060-816f-4906-b049-9609df8b8f4f)

## Step 11. 最後清潔 | Final Cleaning

配完線一定要用酒精把整塊板子刷一刷。我第一次上電時，明明只導通一個數字，管子裡卻有好幾個數字一起亮，或是某幾個數字微微亮著。查了很久配線，最後發現只是殘留的助焊劑造成短路。任何一顆 IC、一支管子或一個接點有短路，都會出現這種現象。  
After wiring, scrub the whole board with alcohol. The first time I powered it on, selecting one digit lit several at once, or left some digits faintly glowing. I spent ages checking the wiring before finding it was just leftover flux causing shorts. A short at any IC, tube or joint can cause this.

下圖是把管子、IC、控制板都拆掉清洗的樣子，洗完曬乾再裝回去比較安全。  
Below, the tubes, ICs and control board are removed for cleaning; let everything dry completely before reassembling.

冒號是紅色 LED 配 470 歐姆電阻。照片上可以看到，我用比較粗的電線把 LED 撐起來，看起來像根「電線桿」。這樣固定其實不太穩，但管子之間比較緊湊，冒號也剛好藏在管子中間，做好到現在都沒歪掉，就沒再改了。  
The colon is a red LED with a 470-ohm resistor. As the photo shows, I held the LED up with thick wire, like a little "utility pole". It's not the sturdiest mount, but it keeps the tubes close together with the colon tucked between them, and it hasn't bent since, so I left it.
![S__37027909_0](https://github.com/user-attachments/assets/f0e744b9-b5e7-439d-9d09-edcf142c7ae5)



---
最後，欣賞一下剩下的照片。  
Finally, the rest of the photos.
![orig (7)](https://github.com/user-attachments/assets/43951a69-dd6f-454e-8b76-edd13eeae783)
![orig (8)](https://github.com/user-attachments/assets/99d5db41-af29-4fd4-ad12-e53c4bb0fe53)
![orig (9)](https://github.com/user-attachments/assets/9513bffe-bc09-48ad-8181-aca6e6846dec)
![S__37027900_0](https://github.com/user-attachments/assets/f58ccdb2-9ffd-4d37-918c-a52e1db7893a)
![S__37027914_0](https://github.com/user-attachments/assets/66887930-8216-4a21-b4b1-47fd6b121eb7)
![S__37027902_0](https://github.com/user-attachments/assets/ca32a1c0-396e-41c4-b617-aeea5d7420d2)
![S__37027911_0](https://github.com/user-attachments/assets/f477af30-1540-4c95-ae09-1a0e7836c39f)
![S__37027913_0](https://github.com/user-attachments/assets/d60680cf-4dc5-4391-a5e8-bfa0594e65b3)

---
## 總材料 | Materials List

- **PCB 板**：記得先買好管子、IC、Arduino 這些主要元件，再估算 PCB 要多大，或乾脆買可以裁切的 PCB。想用大支的管子也可以，看起來更復古。我買的板子焊點銅箔比較薄，不熟烙鐵的人容易把焊點弄翹起來，可以買下圖這種厚一點的 PCB，或去電子材料行問問看。
![截圖 2024-11-03 下午4 21 00](https://github.com/user-attachments/assets/ab93bfe6-eb22-4048-8389-c9adfc8a934d)
- 輝光管 ×6：我用 IN-12，一支就要 60 人民幣，台灣買更貴，所以在淘寶買。大支的管子看個人預算。
- 輝光管座 ×6
- 杜邦線 ×60：建議多買幾條，線可能焊壞或剪太短，我買了 70～80 條。
- K155ID1 輝光管專用驅動 IC ×6
- 74HC595 ×3
- 微動開關 ×2 + 開關用的電阻 ×2
- 總開關 ×1
- Arduino Nano ×1
- 5V-9V 升 170V 小型升壓模組 ×1
- IC 腳座 ×9
- DS1302 模組 ×1
- 電容 ×1（用多個也可以）
- LED ×4：冒號的 4 個點
- 470 歐姆電阻 ×4
- 10K 歐姆電阻 ×6
- 二極體 ×1
- Nano 的排母 ×1：沒有 Nano 專用的，我買了比較長的排母
- 幾根螺絲、螺帽


- **PCB board**: buy the tubes, ICs and Arduino first, then work out how big the PCB needs to be, or get a PCB you can cut to size. Bigger tubes work too and look even more retro. The copper pads on my board were thin and easy to lift if you're new to soldering, so consider a thicker board like the one shown above, or ask at an electronics store.
- **Nixie tubes ×6**: I used IN-12s. Each cost about 60 RMB (more in Taiwan), so I bought them on Taobao. Bigger tubes if your budget allows.
- **Nixie tube sockets ×6**
- **Dupont wires ×60**: buy a few spares (I bought 70–80) in case some get damaged or cut too short.
- **K155ID1 Nixie Tube Driver x6**
- **74HC595 Shift Registers x3**
- **Micro Switches x2** with **Resistors for Switches x2**
- **Main Power Switch x1**
- **Arduino Nano x1**
- **5V-9V to 170V Boost Module x1**
- **IC Sockets x9**
- **DS1302 RTC Module x1**
- **Capacitors x1** (or more if needed)
- **LEDs x4** for the colon indicator
- **470-ohm Resistors x4**
- **10K-ohm Resistors x6**
- **Diode x1**
- **Pin Headers for Nano Board x1** (I bought a larger pack since there is no Nano-specific set)
- **Various Screws and Nuts**

## 總配線流程 | Wiring Overview

- 豆腐頭 DC 5V → 保險絲 → 二極體 → 電容（電容是並聯的，就算拿掉電容，電路也照樣通）
  - → 5V 供給以下：
    - → 升壓模組 → 輝光管
    - → Nano 主板
    - → LED、DS1302、74HC595、按鈕等
    - → 所有 IC 需要的 5V
    - → 微動開關

大部分都可以對照 `main/config.h` 裡的腳位定義。

- Charger (DC 5V) → Fuse → Diode → Capacitors (in parallel, so the circuit still works without them)
  - → 5V supplies:
    - → Boost module → nixie tubes
    - → Nano board
    - → LEDs, DS1302, 74HC595s, buttons, etc.
    - → 5V for all ICs
    - → Micro switches

Most of this can be cross-checked against the pin definitions in `main/config.h`.


---
## 做完後發現可以改善的地方 | Lessons Learned

1. **LED 冒號的固定**：我是用比較粗的鐵絲撐著，正負極用熱縮管固定，目前還沒想到更好的方式。  
   **Mounting the LED colon**: I held it up with thick wire and heat-shrink on the leads; I still haven't found a better way.
2. **電源接頭**：從豆腐頭進來的電源可以改用快速接頭，會更漂亮。  
   **Power connector**: a proper quick-connect jack for the charger input would look neater.

![orig (10)](https://github.com/user-attachments/assets/d457a8c3-2564-4103-b474-2b848ec1024b)

3. **冒號呼吸燈（已完成）**：當初寫這裡時還在想要怎麼做，現在已經用韌體做出來了。冒號接在 D12，那支腳沒有硬體 PWM，所以是用 Timer2 每秒 16000 次的中斷自己切換亮暗（軟體 PWM），一次呼吸 2 秒，亮度也會跟著管子的亮度設定一起調。如果想要更復古的做法，也可以用電容、電阻、二極體：先讓 Nano 對 LED 和電容供電，斷電後 LED 靠電容裡剩下的電慢慢變暗。這需要比較多電容充放電的知識，下圖是示意。  
   **Breathing colon (done)**: when I first wrote this it was just an idea; the firmware now does it. The colon is on D12, which has no hardware PWM, so a 16 kHz Timer2 interrupt switches it on and off in software (software PWM). One breath takes 2 seconds, and the brightness follows the tube brightness setting. For a more vintage approach you could use a capacitor, resistor and diode instead: power the LED and capacitor from the Nano, then cut the power so the LED fades as the capacitor discharges. That takes more knowledge of capacitor charging and discharging; see the sketch below.

![9009d5d6df61842cbda972c500350613](https://github.com/user-attachments/assets/cab8370c-bb8c-4c12-8f67-55c1c78919ad)

4. **使用後發現的按鈕問題**  
   **Button issues found after use**

   我原本用 Nano 板上的 5V 當按鈕（A1、A2）的參考電壓，可能因此出現下面兩個問題：  
   I originally used the Nano's own 5V as the reference for the buttons (A1, A2), which may have caused two problems:

   - 按鈕有時候按了沒反應。用序列埠監控看，其實有電壓變化，但有時候降不到接近 0V。參考電壓改用外部穩定的電源，可能會比較穩。  
     Sometimes a press didn't register. The serial monitor showed the voltage changing, but it sometimes didn't drop close to 0V. A stable external reference would probably help.
   - 在接觸不良等情況下，按鈕可能讓 Nano 多耗電。時鐘連續開超過半天時，偶爾會出現分或秒的數字不亮或亂跳；改用 USB 直接供電給 Nano 後就恢復正常。所以我推測，按鈕的參考電壓不應該直接接在 Nano 板上。  
     With a poor contact, the buttons may have drawn extra current from the Nano. After running for more than half a day, the minute or second digits sometimes went blank or jumped around; powering the Nano straight from USB fixed it. My guess is that the buttons' reference voltage shouldn't come directly from the Nano.

   總結：Nano 的電源最好只負責核心工作（讀時間、控制 74HC595、讀按鈕），不要再掛額外的負載，系統才會穩定。現在的韌體也會對按鈕做防抖，偶爾的雜訊不會被當成按下。  
   In short: keep the Nano's supply for the core work (reading the time, driving the 74HC595s, reading buttons) and avoid extra loads. The firmware now also debounces the buttons, so occasional noise isn't treated as a press.



---

感謝收看！內容有錯，或想問什麼，都歡迎留言或開 issue。  
Thanks for reading! If anything here is wrong or you have questions, feel free to leave a comment or open an issue.
