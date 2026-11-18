# 輝光管時鐘製作記錄 | Nixie Tube Clock Project


![時鐘成品](https://github.com/user-attachments/assets/4f422308-13c9-403f-8024-2359b2cda493)
## 硬體需求 | Hardware Requirements
- **Arduino Nano**：作為控制輝光管顯示的微控制器。  
  **Arduino Nano**: Used as the microcontroller to control the nixie tube display.
- **輝光管（IN-12 型號）**：作為時鐘顯示的主要元件，共需 6 支。  
  **Nixie Tube (IN-12 model)**: Main component for the clock display, requires 6 tubes.
- **杜邦線 **：電路板到輝光管座的連接，共需 60條。  
  **Dupont Wire**: connections from the circuit board to the Nixie tube sockets, a total of 60 pieces required.
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


## 軟體需求

- **Arduino IDE**：用於撰寫和上傳程式碼到 Arduino Nano。  
  **Arduino IDE**: Used for writing and uploading code to Arduino Nano.
- 下載連結：[Arduino 官方網站](https://www.arduino.cc/en/software)  
  Download link: [Arduino Official Website](https://www.arduino.cc/en/software)
- **Nixie Tube Control Library**：專案已包含自製的控制輝光管的程式庫，需將相關 `.ino` 文件上傳至 Arduino。  
  **Nixie Tube Control Library**: Custom library included in the project, upload related `.ino` files to Arduino.
- **DS1302 RTC Library**：用於控制 DS1302 實時時鐘模組，已在 此專案的 `NixieTubeClock/libraries/Ds1302` 內。  
  **DS1302 RTC Library**: DS1302 real-time clock module, it's in `NixieTubeClock/libraries/Ds1302` .



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
- **滾輪換字**：換分鐘時，時、分有變的管子像吃角子老虎一樣往上滾至少一整圈，最後放慢停在新數字，多支時由左到右一支一支停。  
  **Slot-machine roll**: on each new minute, the hour/minute tubes that change spin through at least one full cycle, slow down and land on the new digit, left to right.
- **亮度**：電流不變，只縮短每輪亮的時間（每秒 133 輪），輝光一樣包滿陰極；損耗大致跟亮度成正比。  
  **Brightness**: same current, shorter on-time per cycle (133 Hz), so the glow still covers the whole cathode; wear scales roughly with brightness.
- **自動清管**：每 15~60 秒把每個數字快速亮過兩輪（約 1.2 秒），防止陰極中毒。  
  **Anti-poisoning**: every 15–60 s each digit is cycled twice (≈1.2 s).

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
| `main/animations.*` | 清管、滾輪換字、解碼秀 / cleaning, slot-machine roll, decode show |
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
1. 可能是因為我買的nano板比較舊, 我編譯沒問題,但上傳都會遇到上傳失敗, 後來發現需要選用以下模式才可以, 如果有遇到該問題的可以參考
   My Arduino Nano board might be an older model. I had no issues with compiling, but uploads failed. I found that selecting the following mode solved the problem. If you encounter similar issues, try this setting.
![截圖 2024-11-03 下午5 14 51](https://github.com/user-attachments/assets/8b411fa5-23a8-4aa2-9e48-a87963be53c2)





- [手搓輝光管 Arduino 時鐘 - B站影片](https://www.bilibili.com/video/BV1f31gYLEiL/?share_source=copy_web&vd_source=17521cbd088477c140b597f64646ffe3)
- [Clock video on Youtube](https://youtu.be/MlX_RdtVoo0)



目前成品放在辦公室，看著時間一分一秒流逝。時鐘的背面為了讓正面只看得到元件，看起來比較整潔，所以將焊接、線路都放到底部。
The clock is placed in my office, watching time tick by every second. To keep the front looking clean, all wiring and soldering is hidden on the buttom of pcb.

![orig (2)](https://github.com/user-attachments/assets/cc8a0b39-b5f0-42ad-be37-c314992d429f)

---

## 前言 | Introduction：

想做個side project, 但不想單純做純軟體的東西, 偶然看到非常具有歷史的「輝光管」,輝光管是透過將玻璃管內部灌入氖氣, 並在氣體中將金屬導高電壓, 就會在金屬附近產生橘色的顏色, 仔細看會發現發光的地方是金屬周圍的氣體發光。
I wanted to create a side project, but I didn't want it to do coding only. I came across the historic "nixie tube," which works by filling a glass tube with neon gas. When a high voltage is applied to a metal component inside, the gas around it glows orange. If you look closely, you'll notice that the glowing area is actually the gas around the metal.


偶然看網路上有人在做這個, 心裡想應該不是很難, 應該做得到, 確實寫code大概一天就完成了, 因為點位較多, 甚至因為有LED燈當作冒號, 所以有一個點位還是共用的, 因為自己先在網路上買需要的東西, 結果做的過程中, 陸續去了電子材料行5 次才完成。
I saw someone online making one, and I thought it shouldn’t be too difficult. Writing the code took about a day, but because there are many points to control, including an LED light used as the colon, I had to share one pin for that. I initially ordered the necessary parts online, but ended up going to the electronics store five times before I could finish the project.

順便記錄一下總共需要哪些東西, 給也想試看看的人分享一下, 可以不用走那麼多趟材料行, 我會把需要買的材料列出, 也把source code貼上來, 想復刻的可以當個參考（寫code的時候還想說要把程式寫的很優雅，就把關於74HC595方面的還弄出一個類，結果寫到後來直接把程式都放在main了，之後有時間在把main的程式搬到nixie tube.cpp 內放，應該將設定時間、清理管子、等等的都放進去一個NixieTube class，看起來比較整潔，不過就看之後有心力吧，畢竟程式只要沒有問題，就不要再去改祂。
I keep a record of all the components I used for anyone who want to try this project. This way, you won’t need to make as many trips for parts. I'll list all the required materials and source code here. (While writing the code, I originally thought about making it more elegant by creating a class for the 74HC595 control. But eventually, I put everything in the main file. If I have time, I might move the functions for setting the time, cleaning the tubes, etc., into a separate `NixieTube` class in `nixie_tube.cpp` for a cleaner structure. But as long as the code works, Don't touch the code.)



## step1.製作輸入電壓（下圖是改成電容以前的未成品圖）
## Step 1: Creating the Input Voltage (The image below shows the unfinished setup before replacing with capacitors)

發生一個有趣的事, 因為原本電源是使用9V, 透過LM7805將電壓穩壓成 5V, 當做後面電路的穩壓其實是可以的, 但缺點就是右上角這顆LM7805時不時就在發燙, 當初在材料行看到這個穩壓覺得很酷, 雖然明顯是殺雞用牛刀,

An interesting thing happened here. at beginning, I used a 9V power source and regulated it down to 5V using LM7805. This setup worked for stabilizing the voltage for the circuits, but the LM7805 is pretty hot. I thought this regulator was so cool when I first saw it at the electronics store, even though it was clearly overpowered for this peoject.

在製作完成時鐘後, 放在辦公室一陣子, 剛好被老闆看到這東西, 他剛好也是學電路的, 就推薦我直接將電源使用IPhone豆腐頭即可, 還直接跟我說, 我們後面很多電容, 我拿一些給你, 於是左下角的穩壓器就被換掉了, 這邊補充一下, 因為LM7805是穩壓成5V, 所以如果豆腐頭的5V直接讓LM7805穩壓是行不通的, 應該至少要高於0.7V, 也就是需要一個5.7v才能讓後續電力正常，於是最終就換成電容, 畢竟老闆給建議了, 再不換就對不起老闆的電容。
After placing the clock in my office for a while, my boss, who also has an electronics background, saw it. He recommended using an iPhone charger instead. He even give me some capacitors from the office stock, so I replaced the regulator with capacitors. 
by the way, if you connect a 5V charger directly to LM7805, it won’t work. You need at least 5.7V to ensure stable power. In the end, I swapped it for capacitors, mainly because I wanted to follow my boss’s advice and make use of his capacitors~.

![orig](https://github.com/user-attachments/assets/a46b9b48-a974-49f3-a5ff-869a9af4d38f)


## step2.輸入電壓處理 | Input Voltage Processing

透過電容濾波後的電壓, 將豆腐頭輸入進來的5V透過電容穩壓, 其實一個電容就非常夠, 但因為將LM7805改成電容後, 空間非常夠, 所以就用2組串聯的電容,
After filtering the voltage through a capacitor, I fed the 5V from the charger through additional capacitors for stabilization. A single capacitor could work, but since there was extra space after removing the LM7805, I decided to use two capacitors in series.

備註:電容串聯應該是「耐壓」升高, 並聯應該是容儲存的「電容量」增加, 比如我用2個20V的電容串連的話, 耐壓就變40V, 再將兩組的電容為1000μF並聯, 則會得到2000μF。
Note: Capacitors in series increase the voltage capacity, parallel increase capacitance. For example, two 20V capacitors in series can handle up to 40V. If I use two 1000μF capacitors in parallel, I get a combined capacitance of 2000μF.

在豆腐頭剛進入時，先經過保險絲後，接著就要加個二極體, 防止豆腐頭出什麼毛病，後續才經過電容。
When the power first comes from the charger, it goes through a fuse, followed by a diode to protect against any issues from the charger, and then through the capacitors.
![S__36790275](https://github.com/user-attachments/assets/7736ab03-bee6-4d40-a085-207b7d77fb3a)
![S__37027905_0](https://github.com/user-attachments/assets/0f40e4fb-0bc7-45ba-abf6-0a703e528fe6)


## step3. 製作 5V轉170V升壓模組｜Creating the 5V to 170V voltage converter Module

首先輸入的5V直流電（豆腐頭提供）轉成輝光管所需的電壓, 約160V, 當時因為不想要再自己做變壓器, 買的板子如果再加上變壓器可能會更難, 也可能更做不出來..., 所以就用這個直接轉成高壓
First, the 5V DC input (provided by the phone charger) is converted to the voltage needed by the nixie tubes, around 160V. Since I didn’t want to build a transformer myself.


去淘寶買的升壓模組,保險起見, 可以看到照片，我把正面的接點都用絕緣膠帶貼起來, 之前就是可能焊接的接點比較大, 接電後, 就有火花, 剛開始做就看到煙火秀, 淘寶搜一下「5V-9V升170V小型升压模块」應該就找得到。
I buy this module from Taobao. To be safe, as shown in the picture, I covered the front contacts with tape. Previously, one of the solder joints was too large, which caused sparks and a mini firework show the first time I powered it on. If you’re interested, you can search for “5V-9V to 170V small boost module” on Taobao to find it.

![S__37027904_0](https://github.com/user-attachments/assets/f4a4b0fb-5be2-48bf-8bcf-9c342d86e2d4)



## step 4 .製作DS1302模組 | Building the DS1302 Module

DS1302模組是一款常見的「實時時鐘」（RTC, Real-Time Clock）芯片模組，能夠提供實時的秒、分、時、日、月、年等信息。
用紐扣電池應該就能保持幾年的時間讓改模組一直計算時間，在斷電後繼續保持時間的計算, 也就是透過「石英振蕩器」 不斷計算現在過了幾秒, 跟以前電腦內也會有一個這種模組應該是同一個功用。
The DS1302 module is a commonly used Real-Time Clock (RTC) chip module, which provides real-time information for seconds, minutes, hours, days, months, and years. A button battery can keep the module running for several years, allowing it to keep time even when the main power is off. It uses a oscillator to keep time, similar to the RTC modules in older computers.

順帶一提, 如果有預算的話, 建議另外買好一點的32.768 kHz 石英振蕩器, 我是買下圖這個, 然後拆下來用的, 結果發現一個月差了快十分鐘, 而且還是一陣子比較快、一陣子比較慢，連我在程式內想直接一天加多少秒都無法。
If your budget allows, I recommend getting a high-quality 32.768 kHz quartz oscillator. I bought the one shown below and tear it dowm, but I found that the clock was not accurate over a month. It would sometimes run faster and sometimes slower, making it difficult to compensate by adding seconds in the code.


而這個計算的模組, 在arduino 內也有人有寫好的模組, 直接拿來用就行，也在我的git內，直接下載下來就在那個包內了。
For Arduino users, It’s included in my GitHub repository.

![orig (3)](https://github.com/user-attachments/assets/e275c5a3-12a5-41e1-8329-d5fe9e044498)



如圖，我將買好的模組解焊後，焊在板子上，DS1302的模組有 鈕扣電池、32767Hz的石英振蕩器、計算時間的IC 本身。
As shown in picture, I desoldered the purchased module and soldered it on the board. The DS1302 module includes a button cell battery, a 32,767 Hz quartz oscillator, and a time-keeping IC.
![S__37027915_0](https://github.com/user-attachments/assets/2c005f79-db04-4b9b-956b-f9e9ac6be0c6)


## step5. 做個保險絲 | Adding a Fuse

這步應該是在最前面才對, 就是在5Ｖ豆腐頭進入後就應該經過這個保險絲, 因為做完這個時鐘，半年了才想說寫個文章，當初怎麼個配線流程都忘了，最後會再寫個總配線流程, 但我當初覺得保險絲盒放在正中間有種和諧感, 就放在中間了。
This step should be done first step, right after the 5V charger input. I only decided to document the project about six months after completing the clock, so I forgot the exact wiring sequence. I’ll include a full wiring diagram later, but I placed the fuse holder in the center simply because I thought it looks nice.

下圖如果有學過電路的應該不難理解, 保險絲跟LED燈是並聯, 如果保險絲沒有斷的話, 所有的電都會走保險絲, 如果保險絲燒斷, 所有的電會走LED, 進而達到提示的效果，告訴你短路了。
For anyone with a background in electronics, the setup in the image below should be easy to understand. The fuse and LED are connected in parallel. If the fuse is work, all current flows through it. If the fuse broken, the current will flow through the LED instead, providing a visual indicator of a short circuit.

而保險絲該配多大的問題，問gpt吧，只是盡量不要配太細的，比如我之前是計算每個管子耗多少電流算出來的，但保險絲比如買個0.2A的反而可能讓改保險絲產生電阻，進而這邊產生壓降，導致arduino板、升壓模組等造成一系列問題，所以這裡就改用成2A，實際上多少的話，我就沒經驗了，做韌體的應該就很懂了。
As for choosing the right fuse rating, it’s best to consult resources like GPT for advice, but avoid using one that’s too low. I originally calculated the current each tube would consume and tried a 0.2A fuse, but it caused some issues. The fuse generated resistance, which led to a voltage drop and caused problems with the Arduino board and boost module. So, I replaced it with a 2A fuse. I don’t have much experience here, but those working in firmware or electronics likely know better.
![S__37027906_0](https://github.com/user-attachments/assets/8df6d887-f09f-449d-8ed6-3e75ffbd98db)




## step6.輝光管的IC控制| IC Control for Nixie Tubes

因為每個時鐘有0-9共10個點位要控制, 總共就有60個腳位, 但arduino nano板的腳位無法提供這麼多腳位, 這時候就透過以下的IC再搭配「74HC595 移位寄存器」, 就可以達到只透過３隻腳位 控制2個輝光管共20個腳位的功能, 也就是一個74HC595, 可以有8個IO, 而以下的K155ID1 只需要4個bit 就能控制, 也就是0000是0, 0111的話就是7，1000是8, 1001是9
Each tube requires controlling 10 positions (0-9), which means a total of 60 pins. However, the Arduino Nano board doesn’t have enough pins for this. To solve this, I used the following ICs along with the "74HC595 shift register," which allows controlling 2 nixie tubes (20 pins) with only 3 Arduino pins. Each 74HC595 provides 8 I/O pins, and the K155ID1 driver chip requires only 4 bits to control each digit (e.g., 0000 for 0, 0111 for 7, 1000 for 8, and 1001 for 9).
而因為74HC595可以控制兩個管子，比如想顯示88，就是「10001000」，在比如想顯示24，就是「00100100」，這我應該是在暫存器的code內都寫好了。
Since each 74HC595 can control two tubes, for example, to display "88," the command would be "10001000." To display "24," it would be "00100100." I have written these commands into the code for the shift registers.

輝光管也要記得要配一個10K的電阻，以下我有找到其他國外有人用這兩個IC搭配的圖片,左邊紅框就是1*10K 電阻 + 1* 管子 + 1* K155ID1*1 + 半邊的 74HC595 可以控制一個管子，也就是一個數字 ，而Arduino 要控制的，就是74HC595，進而再控制K155ID1。
Each nixie tube also requires a 10K resistor. Below is an image I found showing how to use these two ICs together, which was created by someone else abroad. In the red box on the left, you can see one 10K resistor, one nixie tube, one K155ID1, and half of a 74HC595. This setup can control one tube, or one digit. The Arduino controls the 74HC595, which in turn controls the K155ID1.

![15611377040_c15f4eb0b9_o-2](https://github.com/user-attachments/assets/9ca2292b-cf2c-4ac0-84a6-3bf781130575)
- [以上圖片連結](https://www.plingboot.com/arduino-nixie-gps-clock-part-2/)

可以看到圖片上的，是74HC595，而一個可以控制2個K155ID1，進而控制兩個管子。
You can see in the picture that it’s a 74HC595, which controls two K155ID1 chips, and those K155ID1 control two tubes.
  ![S__37027910_0](https://github.com/user-attachments/assets/801da5ae-00f2-4caa-9962-8466912223fb)

74HC595使用方式、原理, Youtube收尋, 講的蠻清楚的：【Arduino教學】實作08-74HC595(上)
for english video, you can search "how to use 74HC595"


***這邊有一點要注意的, 下圖是nano板的輸出腳位，d的就是digital out數位輸出，輸出的是0、1, 因為有時分秒要控制, 每一個74HC595就要佔用3隻接腳, 哪3隻可以看上述教學影片就可以了, 這樣會導致需要9隻接腳, 再加上RTC時鐘模組, 就會導致LED 冒號不夠用, 我想到的方式, 是用 「時」這邊的某一個針腳來當LED的共同針腳, 因為小時 這邊的控制都是1小時才使用一次,使用後就沒用了, 於是就用小時的 3隻控制「74HC595」 的其中一隻接腳來控制LED, 有可能這個IC的其中一隻接腳也能共用就不用佔用那麼多, 之前聽說這３隻接腳中有某一隻接腳是可以共用的,但之前製作的時候, 沒有想到在74HC595之間這樣共用, 總之我當時是這樣做的。
***Note: In the diagram below, "d" represents the digital output pins on the Nano, outputting 0 or 1. Since each 74HC595 shift register needs 3 pins, controlling hours, minutes, and seconds requires 9 pins in total. Adding the RTC module can lead to insufficient pins for the LED colon display.
To work around this, I used one of the hour control pins to share with the LED. The hour control only updates once per hour, so after it’s used, the pin can control the LED without issues. I also heard that all of the 74HC595 pins can share, but I didn’t do this in my build.
![orig (4)](https://github.com/user-attachments/assets/f79535f7-3457-4811-bfcc-703933555540)
![S__37027910_0](https://github.com/user-attachments/assets/d4c44734-689e-4280-af34-111a5f8d37cf)


## step8. 製作輝光管支架｜Making the Nixie Tube Stand

為了要讓管子正常站著, 當初也是想了很多方法, 最後用簡單又暴力的方式, 剛好身旁沒有時鐘, 只好截圖影片, 就是將一個螺絲固定在IC板後, 在固定到管子的管座, 至於六隻要怎麼樣保持同個角度、高度、彎度, 且凹彎前, 還要先把螺絲鎖上, 否則凹彎後, 螺帽會鎖不上去，那因為螺絲較粗, 需要對PCB鑽孔, 當初就是用美工刀鑿開一個洞。
To make the tubes stand upright, I tried various methods and eventually went with a simple approach. I secured a screw on the back of the PCB and attached it to the tube’s socket. To keep the six tubes at the same angle, height, and bend, I first screwed them in place. If I bent the pins first, the nuts can’t fit later. Since the screws were a bit thick, I drill holes in the PCB to make hole fit the screw.

這就考驗掰彎的能力了, 想當初也是凹了8隻, 取了6隻比較相似的, 總之就是變變扭扭的彎好了。
This setup really tested my bending skills. I bent eight tubes and chose the six that matched best.


至於輝光管的管座, 一方面是輝光管有壽命, 令一方面是這樣也才能有孔可以固定管子
As for the tube sockets, they not only extend the tubes' lifespan but also provide a way to secure the tubes to the board.

![orig (4)](https://github.com/user-attachments/assets/57d29c63-13c2-4e1f-84be-befa49889f07)

可以看到我用鉗子掰彎螺絲的痕跡，輝光管座的前後，使用兩個螺帽固定
You can see marks from using pliers to bend the screws. The Nixie tube socket is secured front and back with two nuts.
![S__37027903_0](https://github.com/user-attachments/assets/593793f8-9f2b-4d3b-b2d3-ba11920a0775)


螺絲頭在PCB底部的樣子
screw head at buttom of PCB
![S__37027922_0](https://github.com/user-attachments/assets/a58173db-1d54-4977-8db7-7ef2be759b53)





## step9. 配線 | Wiring

接下來就是消耗自己腦力的配線, 配完線才發現其實單芯線應該要買耐熱的, 不然焊槍只要碰久一點, 就會讓白管套黑掉, 還會脫落, 記得焊接要用助焊劑, 元件針腳跟單芯線才會更好的融再一起
Next came the wiring, which took some planning. After finishing, I realized heat-resistant single-core wires are better, as regular wires can get damaged or the insulation may burn if the soldering iron stays too long on wire. Be sure to use flux for smoother connections between component pins and wires.

其實「第一版」配線沒有這條彎線, 是因為做好後, 後來又把LM7805改成電容, 需要有一個比較長的繞線, 就任性的把最後一條線繞成「黃金比例」。
In the first version of the wiring, there was no winding wire. Later, after replacing the LM7805 with capacitors, I needed a longer wire. I ended up coiling the final wire into a "golden ratio" spiral, just for fun.

![orig (2)](https://github.com/user-attachments/assets/ebab3a19-8043-4458-a61b-5f92bc82b986)

![S__37027921_0](https://github.com/user-attachments/assets/69445090-58d3-4d9c-9328-566a04181443)

從底板到輝光管座的連接線，以下圖是我連接兩端的方式
The connection wires from the baseboard to the Nixie tube socket are shown below:
- 底部連接方式杜邦線將一端剪掉、鍍上錫，就能焊在PCB底座上
Baseboard side: Cut off one end of the Dupont wire, tin it, and solder it to the PCB base.
- 管座連接方式，杜邦線的外殼拆掉，再用熱縮管罩住就可以插上到管座上
Socket side: Remove the plastic housing of the Dupont wire, cover it with heat-shrink tubing, and then insert it into the tube socket.
<img width="862" height="565" alt="eb223592-8ba8-457f-9854-c33d29ecfc9a" src="https://github.com/user-attachments/assets/4f695c3f-baf9-43b5-b7eb-2124bc441480" />
<img width="948" height="684" alt="1f7fa4b1-5d7b-4cc7-94fa-6bda2b46f75b" src="https://github.com/user-attachments/assets/5baf1146-16d0-439f-b30d-fbfdce6325a4" />

- 杜邦線的顏色與數字的搭配(這點也可按照個人喜好，你也可以買單色的杜邦線，保持跟底座同樣的風格)，想說這樣如果顯示有錯誤的話，也比較好debug，我的杜邦線顏色是按照電阻的顏色來配的
  杜邦線:黑棕紅橙黃綠藍紫黑白 對照管座顯示的1234567890 來接
  Dupont wire color-to-number mapping (this can be arranged based on personal preference; you can also use single-color Dupont wires to keep the same style as the base). I chose resistor color coding so that if there’s a display error, it’s easier to debug.
It’s recommended to buy a few extra wires in case some get damaged during soldering or cut too short. I bought about 70 wires.
My Dupont wire colors follow the resistor color code: black, brown, red, orange, yellow, green, blue, purple, black, white — corresponding to the tube socket digits 1 2 3 4 5 6 7 8 9 0.
<img width="1078" height="239" alt="80073179-0508-4deb-8b4e-f3b38d098fb8" src="https://github.com/user-attachments/assets/2d8e2137-b60d-486c-8389-3f08c4eaf1a1" />




## step 10. 製作按鍵 | Making the Buttons

還是要設定時間、也想要手動可以執行清理管子的功能, 挑了按鍵按起來比較舒服的微動開關, 因為這開關的針腳比較大, 所以只好對PCB板鑽洞,左下角圖片當成是按鈕
To set the time and manually clean the tubes, I chose micro switches that feel good to press. Since these switches have larger pins, I had to drill holes in the PCB. The button in the lower left corner of the image.

微動開關也要加個電阻, 雖然好像說nano板本身就有內阻, 但我還是有加電阻，那著個接線方式的話, 就是一條線是接在nano板的A in, 偵測目前是５V 還是接近０V, 這個按鈕可以去才聊行看個人喜好, 我當初買著個是覺得要搭配時鐘大小所買的, 但缺點就是這針腳很大, 需要修改板子, 我就是直接用刀子對ＰＣＢ挖洞。
micro switch also need a resistor. Although the Nano board supposedly has internal resistors, I added an extra one for safe. The wiring connects one line to the Nano's analog input (A in), which detects whether the signal is 5V or close to 0V. You can choose a button yoe like; I picked one that matched the clock's size. However, since the pins were large, I had drill the holes in the PCB using a knife.

![81+GMLjeZYL](https://github.com/user-attachments/assets/fd728390-f505-4064-a8bc-03a3b1911e8c)

對照代碼的話，可以看到我製作的兩個按鈕，會接到nano 板上的A1 A2，這裏雖然說nano 板本身可以設定內阻，但我還是有加上電阻
Looking at the code, you can see the two buttons I made, connected to A1 and A2 on the Nano board. Although the Nano board itself can set internal resistance, I still added external resistors for insurance.
![S__37027908_0](https://github.com/user-attachments/assets/83b2ab4c-82e3-40d7-a431-e7bb22f7bb4d)
![S__37027920_0](https://github.com/user-attachments/assets/b84577bd-d403-4861-8f01-2661e87636f7)



## step 11.總開關的製作 | Creating the Main Power Switch

因為考慮到時鐘的管子其實壽命通常是幾萬小時, 為了耐用性, 即使已經有在每隔幾十分鐘就「清理輝光管」, 清理方式就是固定時間快速點亮所有的數字, 目的是為了均勻分佈輝光管內的氣體電離和放電，從而延長輝光管的使用壽命，並避免某些數字或部分發光電極過度使用而導致局部老化, 但我希望這個看起來很老的東西, 可以比我還長壽, 於是就做了一個開關, 在每天上班就打開、下班就關掉, 讓他變成一個「手動檔時鐘」
Nixie tubes have a lifespan of several tens of thousands of hours. To increase durability, I programmed the clock to "clean" the tubes every few minutes by briefly lighting up all digits. This helps evenly distribute ionization in the gas, extending tube life and preventing certain digits or electrodes from wearing out prematurely.

右下角紅色當成是總開關
To make the clock last even longer, I added a main power switch. I turn it on at the start of the workday and off when I leave, making it a "manual mode" clock. The red switch in the lower right corner of the image is the main power switch.

![orig (6)](https://github.com/user-attachments/assets/504dc060-816f-4906-b049-9609df8b8f4f)

## step.12 最後整理 | Final Cleaning

最後要記得因為配完線要用酒精把整個板子刷一刷, 之前上電後, 發現我只要導通某一個數字, 但卻一次讓管子內多個數字點亮, 或是某幾個數字微亮, 查了很久的配線, 發現只是因為助焊劑, 導致有些地方短路到, 只要是其中一個IC、管子、接點有短路, 皆會導致這些短路問題。
After finishing the wiring, remember to clean the entire board with alcohol. When I first powered it on, I noticed that multiple digits on the tubes would light up unintentionally, or some digits would glow faintly. After troubleshooting, I realized the issue was caused by leftover flux, creating short circuits in certain areas. Any short circuit on an IC, tube, or connection point can lead to these problems.

把管子、IC、控制板都拿掉的樣子, 洗過後, 曬一下會比較安全
I removed the tubes, ICs, and control board, cleaned everything, and let it dry completely for safety.

還有LED冒號, 也是用個470歐姆的電阻+紅色LED當冒號，照片上可以看到，我使用比較粗的電線來把LED撐起來，看起來像個「電線桿」，其實這樣固定不是很穩定，但整體看時鐘管子之間較緊湊，冒號也是藏在管子中，剛做好到現在，不會彎掉，就沒在改善這問題了
For the LED colon indicator, I used a 470-ohm resistor with a red LED. In the picture, you can see I used thicker wires to hold up the LED, making it look like a “pole.” It’s not very stable, but overall, you can see the clock tubes look more compact, with the colon tucked between them. Since it hasn’t bent since I put it together, I haven’t tried to improve this setup.
![S__37027909_0](https://github.com/user-attachments/assets/f0e744b9-b5e7-439d-9d09-edcf142c7ae5)



---
最後，欣賞一下剩下的素材
Finally, take a look at the remaining photos.
![orig (7)](https://github.com/user-attachments/assets/43951a69-dd6f-454e-8b76-edd13eeae783)
![orig (8)](https://github.com/user-attachments/assets/99d5db41-af29-4fd4-ad12-e53c4bb0fe53)
![orig (9)](https://github.com/user-attachments/assets/9513bffe-bc09-48ad-8181-aca6e6846dec)
![S__37027900_0](https://github.com/user-attachments/assets/f58ccdb2-9ffd-4d37-918c-a52e1db7893a)
![S__37027914_0](https://github.com/user-attachments/assets/66887930-8216-4a21-b4b1-47fd6b121eb7)
![S__37027902_0](https://github.com/user-attachments/assets/ca32a1c0-396e-41c4-b617-aeea5d7420d2)
![S__37027911_0](https://github.com/user-attachments/assets/f477af30-1540-4c95-ae09-1a0e7836c39f)
![S__37027913_0](https://github.com/user-attachments/assets/d60680cf-4dc5-4391-a5e8-bfa0594e65b3)

---
## 總材料 | Materials List：

- PCB板, 記得先買管子, 如果想要大隻的管子也可以, 看起來更有復古感, 如果不是跟我一樣要買小管子的人, 要先買好管子、IC、Ａrduino 等等的主要模組, 大概算一下需要的PCB需要多大, 不然就是要買那種可以裁的PCB, 還有因為我買的板子的焊點金屬比較薄, 對不熟焊槍的人, 可能會容易把PCB的焊點弄起來, 可以買下圖這種較厚一點的PCB, 可能品質更好一點, 但還是去電子材料行看看, 也能問一下材料行的人
![截圖 2024-11-03 下午4 21 00](https://github.com/user-attachments/assets/ab93bfe6-eb22-4048-8389-c9adfc8a934d)
- 辉光管*6 我買的是 IN-12型號, 因為1 隻就要60人民幣了, 台灣買更貴, 所以我是在淘寶買的, 大隻輝光管就看個人預算
- 辉光管座*6 
- 杜邦線*60 建議多買幾條，有可能線焊壞、剪太短，我是買70~80條
- K155ID1辉光管专用驱动*6
- 74HC595*3
- 微動開關*2 + 開關用的電阻*2
- 總開關*1
- Arduino nano*1
- 5V-9V升170V小型升压模块*1
- IC針腳座*9
- DS1302 模組*1
- 電容*1(用多個也可以)
- LED*4 冒號要有4個
- 470歐姆電阻*4
- 10K歐姆電阻*6
- 二極體*1
- nano板的針腳座*1 我是買大於2個針腳座的, 因為沒有nano專用的
- 幾根螺絲、螺帽


- **PCB Board**: Make sure to purchase the tubes first. If you want larger tubes, account for their size when choosing a PCB. If not, calculate the PCB size based on your main components, like the tubes, ICs, and Arduino. or you can buy cuttable PCB. My PCB's solder points were quite thin, which may cause issues if you're not familiar with soldering, as they can lift off the board. You might want to get a thicker PCB, like the one shown below, for better quality. Visiting an electronics store and asking for recommendations could also help.
- **Nixie Tubes x6**: I used the IN-12 model. Each tube cost about 60 RMB; they are more expensive in Taiwan, so I bought them from Taobao. Larger tubes are an option if your budget allows.
- **Nixie Tube Sockets x6**
- Dupont Wire*60**
- Dupont wires ×60(you can buy 70~ 80 just in cass)
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

## 總配線流程  | Wiring Overview：
- 豆腐頭DC5v＞保險絲＞二極體＞電容（電容這裡是並聯, 也就是即使電容移除, 電路也不會受影響）
  - ＞原本的5v供給給以下
  -   ＞升壓模組 >供給給管子
  -   ＞nano主板
  -   ＞led, 讀ds1302, 給寄存器, 讀按鈕, 等等
  -   ＞供給給全部ic所需的5v
  -   ＞微動開關

以上大部分看code也可以對照。

- Charger (DC 5V) > Fuse > Diode > Capacitors (in parallel, so removing them doesn’t disrupt the circuit)
  - > Supplies 5V to:
    - > Boost Module > Powers the tubes
    - > Nano Board
    - > LEDs, DS1302, Shift Registers, Buttons, etc.
    - > All ICs needing 5V
    - > Micro Switch
Most of this wiring can be cross-referenced with the code.


---
## 最後幾點做完後發現可以改善的地方 | Final Improvements and Suggestions:

1. led冒號的固定，我是直接找比較粗的鐵絲將正負極的線用熱縮管固定，現在還是還沒想到更好的方式。
   **LED Colon Mounting**: I used thicker wire with heat shrink tubing to secure the positive and negative leads, but there may be a better way to mount it.
2. 從豆腐頭進入的電源可以買那種快速接頭，會更漂亮
    **Power Connector**: Using a quick connector for the power input from the charger would look neater.

![orig (10)](https://github.com/user-attachments/assets/d457a8c3-2564-4103-b474-2b848ec1024b)

3. LED如果要使用PWM的方式達到呼吸燈的效果, 應該也是可以的, 因為arduino nano某些腳位是有PWM功能的, 但如果想要更有復古感, 也能透過電容、電阻、二極體搭配，來達到使用nano 供給LED、電容5Ｖ後，馬上斷點，這時LED 透過電容內剩餘的電容量將LED點亮, 達到呼吸的效果, 但這更要求你對電容、充放電的知識更足夠才能做到這效果, 以下是圖示
  **Breathing LED Effect**: For a breathing effect on the LED, you could use PWM on certain Arduino Nano pins. Alternatively, for a more vintage effect, you could use a capacitor, resistor, and diode setup. This would work by supplying 5V to both the LED and capacitor, then cutting off the supply so the LED gradually fades as the capacitor discharges. However, this requires a deeper understanding of capacitor charge and discharge behavior. Below is an illustration.

![9009d5d6df61842cbda972c500350613](https://github.com/user-attachments/assets/cab8370c-bb8c-4c12-8f67-55c1c78919ad)

4. 使用後發現的按鈕問題 
Issues Observed with Button Functionality

我原本使用 Nano 板提供的 5V 作為 A1 的輸入電壓，結果可能因此出現以下問題：
4.1按鈕有時候會按下沒反應。透過 serial port monitor 檢查，發現其實有電壓輸入，但有時候無法接近 0V。這顯示出參考電壓若改用外部穩定電源，可能會更穩定。
4.2此外，以上問題可能會導致兩個按鈕在接觸不良或特定情況下，使 Nano 板過度耗電。在我持續開啟時鐘超過半天時，有時會發生分或秒位數字無法顯示或亂跳的情況。不過，當我直接用 USB 為 Nano 板供電時，顯示恢復正常。因此推測，不應該將時鐘左邊兩個按鈕的參考電壓直接接在 Nano 板上。
總結來說，Nano 板的電源應該專注於處理核心邏輯，例如時間獲取、控制 74HC595、按鈕讀取等工作，避免額外的負載以維持系統穩定性。

Initially, I used the 5V from the Nano board as the input voltage for A1 A2, which may have led to the following issues:

Sometimes, button presses don’t respond. Checking with the serial port monitor showed voltage input, but it occasionally doesn’t drop close to 0V. This suggests that the reference voltage would likely be more stable if sourced from an external, stable power supply.

Additionally, the above issue may cause both buttons to draw excess current under certain conditions or poor contact, leading to power instability for the Nano. When I kept the clock running for more than half a day, sometimes the digits for minutes or seconds failed to display or showed random numbers. However, when I powered the Nano directly via USB, the display returned to normal. This suggests that the reference voltage for the two buttons on the left side of the clock should not directly rely on the Nano’s 5V.

In summary, the Nano’s power supply should focus on handling core logic tasks like timekeeping, controlling the 74HC595, and reading button states, avoiding additional loads to maintain system stability.



---

感謝收看，有什麼錯誤資訊，或想問我的歡迎留言
thanks for watching, if there is something wrong i say, leave the command! 
