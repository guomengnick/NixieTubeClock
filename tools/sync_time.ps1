# 輝光管時鐘 USB 對時
# 用法: 雙擊 sync_time.bat, 或在 PowerShell 執行 .\sync_time.ps1 [-Port COM4]
#
# 流程:
#   1. 找到時鐘的 COM 埠 (CH340), 打開後一直送 "?", 等時鐘回 "NOW ..."
#      (打開 COM 埠可能讓 Nano 重開機, 要等開機動畫跑完才會回)
#   2. 等到下一個整秒, 送出 "T2026-11-18 10:00:00"
#   3. 時鐘回 "OK ... drift -5", 印出對時前時鐘快/慢了幾秒
param(
    [string]$Port
)

$ErrorActionPreference = 'Stop'
$BaudRate = 9600
$SendLatencyMs = 22  # 9600 baud 送 21 個字約 22ms, 提早這麼多送出, 時鐘收到時剛好是整秒

function Find-ClockPort {
    $devices = @(Get-CimInstance Win32_PnPEntity -Filter "Name LIKE '%(COM%)'" |
        ForEach-Object {
            if ($_.Name -match '\((COM\d+)\)') {
                [pscustomobject]@{ Port = $Matches[1]; Name = $_.Name }
            }
        })
    $ch340 = @($devices | Where-Object { $_.Name -match 'CH340' })
    if ($ch340.Count -ge 1) { return $ch340[0].Port }
    if ($devices.Count -eq 1) { return $devices[0].Port }

    if ($devices.Count -eq 0) {
        Write-Host '找不到任何 COM 埠, 請確認時鐘的 USB 有插好 (要插對 USB 孔)。' -ForegroundColor Red
    } else {
        Write-Host '找到多個 COM 埠, 不確定哪個是時鐘, 請用 -Port 指定:' -ForegroundColor Yellow
        $devices | ForEach-Object { Write-Host "  $($_.Port)  $($_.Name)" }
    }
    return $null
}

function Read-LineOrNull([System.IO.Ports.SerialPort]$serial) {
    try { return $serial.ReadLine().Trim() } catch [System.TimeoutException] { return $null }
}

if (-not $Port) { $Port = Find-ClockPort }
if (-not $Port) { exit 1 }

$serial = New-Object System.IO.Ports.SerialPort $Port, $BaudRate, 'None', 8, 'One'
$serial.NewLine = "`n"
$serial.ReadTimeout = 200
$serial.DtrEnable = $false  # 盡量不要讓 Nano 重開機 (有些驅動程式還是會重開, 下面會等它)

try {
    $serial.Open()
} catch {
    Write-Host "打不開 $Port : $($_.Exception.Message)" -ForegroundColor Red
    Write-Host '如果 Arduino IDE 的序列埠監控視窗開著, 請先關掉再試。'
    exit 1
}

try {
    Write-Host "連接 $Port, 等待時鐘回應..."
    $clockTime = $null
    $deadline = (Get-Date).AddSeconds(12)
    while (-not $clockTime -and (Get-Date) -lt $deadline) {
        $serial.Write("?`n")
        $askUntil = (Get-Date).AddMilliseconds(500)
        while (-not $clockTime -and (Get-Date) -lt $askUntil) {
            $line = Read-LineOrNull $serial
            if ($line -and $line.StartsWith('NOW ')) { $clockTime = $line.Substring(4) }
        }
    }
    if (-not $clockTime) {
        Write-Host '時鐘沒有回應。請確認時鐘已燒錄有 USB 對時功能的程式, 而且 COM 埠沒選錯。' -ForegroundColor Red
        exit 1
    }
    Write-Host "時鐘目前時間: $clockTime"

    # 前面一直送 "?", 時鐘可能還有幾行回覆在路上, 清掉再開始
    Start-Sleep -Milliseconds 300
    $serial.DiscardInBuffer()

    # 等到下一個整秒 (至少留 1 秒準備) 再送, 時鐘收到時剛好是那一秒的開頭
    $now = Get-Date
    $target = (New-Object DateTime $now.Year, $now.Month, $now.Day, $now.Hour, $now.Minute, $now.Second).AddSeconds(2)
    $waitMs = ($target - (Get-Date)).TotalMilliseconds - $SendLatencyMs
    if ($waitMs -gt 0) { Start-Sleep -Milliseconds ([int]$waitMs) }
    $command = 'T' + $target.ToString('yyyy-MM-dd HH:mm:ss', [Globalization.CultureInfo]::InvariantCulture)
    $serial.Write("$command`n")

    $result = $null
    $deadline = (Get-Date).AddSeconds(3)
    while (-not $result -and (Get-Date) -lt $deadline) {
        $line = Read-LineOrNull $serial
        if ($line -and ($line.StartsWith('OK ') -or $line.StartsWith('ERR'))) { $result = $line }
    }

    if (-not $result) {
        Write-Host '送出時間後沒有收到回覆, 對時可能沒成功。' -ForegroundColor Red
        exit 1
    }
    if ($result.StartsWith('ERR')) {
        Write-Host "時鐘回報錯誤: $result" -ForegroundColor Red
        exit 1
    }

    $drift = [long]($result.Split(' ')[-1])
    Write-Host ''
    Write-Host "對時成功: $($target.ToString('yyyy-MM-dd HH:mm:ss'))" -ForegroundColor Green
    if ($drift -eq 0) {
        Write-Host '時鐘原本就很準 (誤差不到 1 秒)。'
    } elseif ([math]::Abs($drift) -ge 86400) {
        Write-Host "時鐘原本差了 $([math]::Round([math]::Abs($drift) / 86400.0, 1)) 天 (可能是 RTC 電池沒電後被重設)。"
    } elseif ($drift -gt 0) {
        Write-Host "時鐘原本快了 $drift 秒。"
    } else {
        Write-Host "時鐘原本慢了 $(-$drift) 秒。"
    }
} finally {
    $serial.Close()
}
