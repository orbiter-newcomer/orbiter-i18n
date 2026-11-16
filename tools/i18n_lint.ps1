# ============================================================================
# i18n_lint.ps1 - text encoding / localisation rules for the China-localised
# Orbiter fork.
#
# Rules enforced (see i18n/doc/i18n.md):
#   1. All source and data text files must be valid UTF-8 (BOM allowed).
#   2. Source code must not contain hard-coded CJK string literals outside
#      comments (translations live in i18n/locales/zh_CN/*.po).
#   3. Legacy ANSI Win32 text APIs (SetWindowTextA / TextOutA / ...) must not be
#      used with non-ASCII text; use the UTF-8 -> wide path of the codec layer
#      (orbiter::LocalToWide / oapiUtf8ToWString) and the *W APIs instead.
#
# Usage (from the repository root):
#   powershell -ExecutionPolicy Bypass -File tools/i18n_lint.ps1
# Exit code: 0 = clean, 1 = problems found.
# ============================================================================
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
if (-not (Test-Path (Join-Path $root 'i18n'))) { $root = Get-Location }

$srcExts   = @('.cpp', '.h', '.hpp', '.c', '.inc')
$dataExts  = @('.scn', '.txt', '.htm', '.html', '.cfg', '.ini', '.lua', '.md', '.json', '.xml')
$srcDirs   = @('Src', 'OVP', 'Orbitersdk', 'i18n')
$dataDirs  = @('Scenarios', 'Config', 'Html', 'Doc', 'Script', 'GravityModels', 'Kernels', 'Flights', 'BinAssets')
$dataDirsExclude = @('build64', 'install64', '.git', 'temp')

$srcPrefixes  = $srcDirs  | ForEach-Object { (Join-Path $root $_) + '\' }
$dataPrefixes = $dataDirs | ForEach-Object { (Join-Path $root $_) + '\' }
$strictUtf8 = New-Object System.Text.UTF8Encoding($false, $true)
$utf8NoBom = New-Object System.Text.UTF8Encoding($false)
$problems = New-Object System.Collections.Generic.List[object]

function Add-Problem([string]$file, [int]$line, [string]$rule, [string]$detail) {
    $script:problems.Add([pscustomobject]@{ File = $file; Line = $line; Rule = $rule; Detail = $detail })
}

# ---- 1. encoding of source and data files --------------------------------
$allFiles = Get-ChildItem $root -Recurse -File -ErrorAction SilentlyContinue
foreach ($f in $allFiles) {
    $fn = $f.FullName
    if ($fn -match ('\\' + ($dataDirsExclude -join '\\|\\') + '\\')) { continue }
    $isSrc = $false
    foreach ($pre in $srcPrefixes)  { if ($fn.StartsWith($pre))  { $isSrc = $true; break } }
    $isData = $false
    foreach ($pre in $dataPrefixes) { if ($fn.StartsWith($pre)) { $isData = $true; break } }
    if (-not ($isSrc -or $isData)) { continue }
    $ext = $f.Extension.ToLower()
    if (-not ($srcExts -contains $ext -or ($isData -and $dataExts -contains $ext))) { continue }
    $bytes = [System.IO.File]::ReadAllBytes($fn)
    if ($bytes.Length -eq 0) { continue }
    try { [void]$strictUtf8.GetString($bytes) }
    catch {
        Add-Problem ($fn.Replace($root + '\', '')) 1 'encoding' 'file is not valid UTF-8'
        continue
    }
    # Strip a UTF-8 BOM for the content checks below.
    $text = [System.Text.Encoding]::UTF8.GetString($bytes)
    if ($text.Length -gt 0 -and $text[0] -eq [char]0xFEFF) { $text = $text.Substring(1) }
    if (-not $isSrc) { continue }

    # i18n data files legitimately contain Chinese (they are the translation data).
    $isI18nData = $fn.Replace($root + '\', '') -match '^i18n\\' -and
                  ($f.Name -match 'Zh\.inc$' -or $f.Extension -eq '.po')
    if ($isI18nData) { continue }

    # ---- 2. hard-coded CJK string literals in source code ----------------
    $lines = $text -split "`n"
    for ($i = 0; $i -lt $lines.Count; $i++) {
        $line = $lines[$i]
        if ($line -notmatch '[\u4e00-\u9fff]') { continue }
        $t = $line.Trim()
        if ($t.StartsWith('//') -or $t.StartsWith('*') -or $t.StartsWith('/*')) { continue }
        $code = $line
        $cmt = $code.IndexOf('//')
        if ($cmt -ge 0) { $code = $code.Substring(0, $cmt) }
        if ($code -notmatch '[\u4e00-\u9fff]') { continue }
        # Chinese characters inside a string literal => hard-coded translation.
        # Lines that are intentionally exempt (e.g. language endonyms in the
        # language selector) carry a '// lint:allow-cjk' marker.
        if ($line -match '//\s*lint:allow-cjk') { continue }
        if ($code -match '"[^"]*[\u4e00-\u9fff][^"]*"' -or $code -match 'L"[^"]*[\u4e00-\u9fff][^"]*"') {
            Add-Problem ($fn.Replace($root + '\', '')) ($i + 1) 'hardcoded-cjk' $t.Substring(0, [Math]::Min(120, $t.Length))
        }
    }

    # ---- 3. legacy ANSI text API with non-ASCII operands -----------------
    for ($i = 0; $i -lt $lines.Count; $i++) {
        $line = $lines[$i]
        if ($line -notmatch '(SetWindowTextA|TextOutA|DrawTextA|SendDlgItemMessageA|MessageBoxA|GetWindowTextA|CreateFontA)\s*\(') { continue }
        if ($line -notmatch '[\u4e00-\u9fff]') { continue }
        Add-Problem ($fn.Replace($root + '\', '')) ($i + 1) 'ansi-api' $line.Trim().Substring(0, [Math]::Min(120, $line.Trim().Length))
    }
}

# ---- report ---------------------------------------------------------------
$count = $problems.Count
if ($count -eq 0) {
    Write-Host 'i18n lint: OK (encoding, hard-coded CJK, ANSI API checks passed)'
    exit 0
}
Write-Host ("i18n lint: {0} problem(s) found" -f $count)
$problems | Group-Object Rule | Sort-Object Count -Descending | ForEach-Object {
    Write-Host ("  {0}: {1}" -f $_.Name, $_.Count)
}
Write-Host ''
$problems | Sort-Object File, Line | Select-Object -First 300 | ForEach-Object {
    Write-Host ("  [{0}] {1}:{2}: {3}" -f $_.Rule, $_.File, $_.Line, $_.Detail)
}
exit 1
