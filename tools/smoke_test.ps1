# ==============================================================
# smoke_test.ps1 - Orbiter scene smoke test (fully automated)
#
# 启动指定场景，运行到 --maxsystime 秒后自动退出，然后检查：
#   1) 进程是否正常退出（无崩溃、无超时）
#   2) Orbiter.log 中是否出现致命错误（FATAL / 异常 / 着色器编译失败等）
#
# 全自动要点：
#   - 使用 ProcessStartInfo + UseShellExecute=false（直接 CreateProcess）启动，
#     绕过 shell 层的 Windows 安全提示（否则会触发
#     "Windows 无法验证此应用程序的发布者" 弹窗，需要人手点击）；
#   - 运行期间轮询日志：一旦发现致命错误立即结束进程并判定 FAIL，
#     不依赖 --maxsystime（避免错误弹窗/卡死场景下干等超时）；
#   - 超时后也会自动结束进程，不会有人工交互。
#
# 用法（默认用 Delta-glider 场景跑 6 秒模拟时间）：
#   powershell -ExecutionPolicy Bypass -File .\tools\smoke_test.ps1
#   powershell -ExecutionPolicy Bypass -File .\tools\smoke_test.ps1 -MaxSysTime 10
#   powershell -ExecutionPolicy Bypass -File .\tools\smoke_test.ps1 -Scenario 'Quickstart'
#
# 退出码：0 = 通过，1 = 失败
# ==============================================================
[CmdletBinding()]
param(
    # 场景名（相对 Scenarios 目录，不带 .scn 后缀）
    [string]$Scenario = 'Delta-glider\DG and DG-S',
    # --maxsystime 参数：运行多少秒模拟时间后自动退出
    [int]$MaxSysTime = 6,
    # 进程最长等待时间（秒），超过视为卡死/弹窗阻塞
    [int]$RunTimeoutSec = 150,
    # 游戏安装目录（默认从脚本位置推断：<repo>\install64\Orbiter）
    [string]$GameDir,
    # 启动前结束已存在的 Orbiter 进程
    [switch]$KillExisting,
    # 保留失败现场：不结束进程
    [switch]$Keep,
    # 日志轮询间隔（毫秒）
    [int]$PollMs = 800
)

$ErrorActionPreference = 'Stop'

$scriptRoot = $PSScriptRoot
if (-not $GameDir) {
    $repoRoot = Split-Path -Parent $scriptRoot          # <repo>\orbiter
    $GameDir  = Join-Path $repoRoot 'install64\Orbiter'
}
$exe = Join-Path $GameDir 'Orbiter.exe'
$log = Join-Path $GameDir 'Orbiter.log'

if (-not (Test-Path $exe)) {
    Write-Host "[FAIL] Orbiter.exe not found: $exe"
    exit 1
}

# ---- stale process check -------------------------------------
$running = @(Get-Process Orbiter -ErrorAction SilentlyContinue)
if ($running.Count -gt 0) {
    if ($KillExisting) {
        Write-Host "[info] stopping $($running.Count) running Orbiter process(es)"
        $running | Stop-Process -Force
        Start-Sleep -Seconds 2
    } else {
        Write-Host '[FAIL] Orbiter is already running; close it or pass -KillExisting'
        exit 1
    }
}

# ---- fatal log patterns --------------------------------------
# Note: benign D3D9ERROR lines ("UnDeleted Surfaces") are not treated as fatal;
# the patterns below are real failure conditions.
$fatalPatterns = @(
    'FATAL',
    'Unhandled exception',
    'APPCRASH',
    'Assertion failed',
    'access violation',
    'Compilation failed',
    'Effect Error'
)

function Find-FatalLine([string]$path, [string[]]$patterns) {
    if (-not (Test-Path $path)) { return $null }
    try {
        $tail = @(Get-Content $path -Tail 60 -Encoding Default -ErrorAction SilentlyContinue)
    } catch {
        return $null
    }
    foreach ($line in $tail) {
        foreach ($pattern in $patterns) {
            if ($line -match [regex]::Escape($pattern)) { return $line }
        }
    }
    return $null
}

Write-Host '== Orbiter smoke test =='
Write-Host "  game      : $GameDir"
Write-Host "  scenario  : $Scenario"
Write-Host "  maxtime   : ${MaxSysTime}s (simulation)"
Write-Host "  timeout   : ${RunTimeoutSec}s (wall clock)"

# ---- launch (direct CreateProcess: no shell security prompt) --
$startInfo = New-Object System.Diagnostics.ProcessStartInfo
$startInfo.FileName = $exe
$startInfo.Arguments = "--scenariox=`"$Scenario`" --maxsystime=$MaxSysTime"
$startInfo.WorkingDirectory = $GameDir
$startInfo.UseShellExecute = $false
$startTime = Get-Date
$process = [System.Diagnostics.Process]::Start($startInfo)

# ---- wait loop: exit / fatal log / timeout --------------------
$fatalLine = $null
$timedOut = $false
while (-not $process.HasExited) {
    $elapsed = ((Get-Date) - $startTime).TotalSeconds
    if ($elapsed -ge $RunTimeoutSec) {
        $timedOut = $true
        break
    }
    $fatalLine = Find-FatalLine $log $fatalPatterns
    if ($fatalLine) { break }
    Start-Sleep -Milliseconds $PollMs
}

$elapsed = [int]((Get-Date) - $startTime).TotalSeconds

# ---- stop the process in every non-clean case -----------------
if (-not $process.HasExited) {
    if (-not $Keep) {
        try { $process.Kill() } catch {}
        $process.WaitForExit(5000) | Out-Null
    }
}
$exitCode = -1
if ($process.HasExited) { $exitCode = $process.ExitCode }

# ---- result checks -------------------------------------------
$failures = @()
if ($timedOut) {
    $failures += "process did not exit within ${RunTimeoutSec}s (hang / blocked by a dialog?)"
}
if ($fatalLine) {
    $failures += "fatal log entry: $fatalLine"
}
if ($process.HasExited -and -not $timedOut -and -not $fatalLine -and $exitCode -ne 0) {
    $failures += "process exit code: $exitCode"
}
if (Test-Path $log) {
    $logLines = @(Get-Content $log -Encoding Default)
    $hasShutdown = @($logLines | Where-Object { $_ -match '\*\*\*\* Fast process shutdown' }).Count -gt 0
    if ($process.HasExited -and -not $hasShutdown -and -not $timedOut -and -not $fatalLine) {
        $failures += 'log does not contain "Fast process shutdown"'
    }
} else {
    $failures += "Orbiter.log not found: $log"
}

# ---- report --------------------------------------------------
Write-Host ''
Write-Host '== result =='
Write-Host "  exit code : $exitCode"
Write-Host "  elapsed   : ${elapsed}s"

if (Test-Path $log) {
    Write-Host ''
    Write-Host '-- Orbiter.log (tail) --'
    Get-Content $log -Tail 10 -Encoding Default | ForEach-Object { "  $_" }
    if ($fatalLine) {
        Write-Host ''
        Write-Host '-- fatal entry --'
        Write-Host "  $fatalLine"
    }
}

if ($failures.Count -gt 0) {
    Write-Host ''
    Write-Host 'SMOKE TEST: FAIL'
    $failures | ForEach-Object { Write-Host "  - $_" }
    exit 1
}

Write-Host ''
Write-Host 'SMOKE TEST: PASS'
exit 0
