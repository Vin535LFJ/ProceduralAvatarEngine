# Builds and runs chain B of the P1-VR-ARCH-1 proof: template_proof.cpp, the
# single-file converter that links the engine's own host static library
# (build-host/libpae_core.a) and uses its Math, MeshData, MeshTopology and
# canonical::detectSelfIntersections. It reads the pinned MakeHuman assets and
# writes OBJ / weights.csv / base.paehuman / logs / an evidence hash list.
#
# Chain B needs the engine's host build first:
#     cmake -S native -B build-host -G "Unix Makefiles" -DPAE_HOST=ON `
#         -DCMAKE_MAKE_PROGRAM=C:/cygwin64/bin/make.exe -DCMAKE_CXX_COMPILER=C:/cygwin64/bin/c++.exe
#     cmake --build build-host -j 8
#
# Every action is checked for direction (DIRECTION_CHECK in the log); the run
# exits non-zero when an action moves the limb the wrong way.
# Timing and peak memory: Process.PeakWorkingSet64 on the Start-Process object
# is not reliable in PowerShell 5.1, so the value is sampled through
# Get-Process while the proof runs.
#   -Source / -Exe / -OutDir / -LogName allow a negative-control run.
param(
    [string]$PaeRepo = '',
    [string]$Assets = '',
    [string]$Source = '',
    [string]$Exe = '',
    [string]$OutDir = '',
    [string]$LogName = 'template-proof.log',
    [string]$BuildLog = ''
)
$ErrorActionPreference = 'Stop'
$host_dir = Split-Path -Parent $MyInvocation.MyCommand.Path
$repo_root = Split-Path -Parent (Split-Path -Parent (Split-Path -Parent $host_dir))
if (-not $PaeRepo) { $PaeRepo = $repo_root }
$work = Join-Path $repo_root 'build-host\human-template-poc'
if (-not $Source) { $Source = Join-Path $host_dir 'template_proof.cpp' }
if (-not $Exe) { $Exe = Join-Path $work 'template_proof.exe' }
if (-not $OutDir) { $OutDir = Join-Path $work 'out' }
if (-not $BuildLog) { $BuildLog = Join-Path $work 'template_proof-build.log' }

# Pinned MakeHuman assets (CC0 data) stay outside git. Default: <repo>/assets/.
$commit = 'a8bc2d54ff0ac92e78ff71431b1023eda42bf482'
if (-not $Assets) { $Assets = Join-Path $repo_root "assets\makehuman-a8bc2d54\makehuman-$commit" }
if (-not (Test-Path -LiteralPath (Join-Path $Assets 'makehuman\data\3dobjs\base.obj'))) {
    throw "pinned MakeHuman assets not found under '$Assets'. Download the archive of commit $commit (codeload.github.com/makehumancommunity/makehuman/zip/$commit) and pass -Assets <extracted repo root>."
}
$lib = Join-Path $PaeRepo 'build-host\libpae_core.a'
if (-not (Test-Path -LiteralPath $lib)) {
    throw "missing $lib - build the engine's host library first (see the comment at the top of this script)"
}

$env:CYGWIN = 'nodosfilewarning'
$diag = & 'C:/cygwin64/bin/c++.exe' -std=c++17 -O2 -Wall -Wextra "-I$PaeRepo/native" $Source $lib -o $Exe 2>&1
$buildExit = $LASTEXITCODE
$warnings = @($diag | Where-Object { "$_" -match 'warning:|error:' } | ForEach-Object { "$_" -replace '\s+', ' ' })
New-Item -ItemType Directory -Force -Path (Split-Path -Parent $BuildLog) | Out-Null
$diag | Out-File -Encoding utf8 $BuildLog
"build exit=$buildExit warnings_or_errors=$($warnings.Count)"
$warnings | Select-Object -First 20
if ($buildExit -ne 0) { exit 1 }

New-Item -ItemType Directory -Force -Path $OutDir | Out-Null
$log = Join-Path $OutDir $LogName
$err = Join-Path $OutDir 'template-proof-memory.err'
$sw = [Diagnostics.Stopwatch]::StartNew()
$p = Start-Process -FilePath $Exe -ArgumentList @("`"$Assets`"", "`"$OutDir`"") -NoNewWindow -PassThru -RedirectStandardOutput $log -RedirectStandardError $err
$null = $p.Handle
$peak = 0; $cpu = 0.0
while (-not $p.HasExited) {
    $g = Get-Process -Id $p.Id -ErrorAction SilentlyContinue
    if ($g) {
        try { $peak = [Math]::Max($peak, $g.PeakWorkingSet64); $cpu = [Math]::Max($cpu, $g.TotalProcessorTime.TotalSeconds) } catch {}
    }
    Start-Sleep -Milliseconds 60
}
$p.WaitForExit()
$sw.Stop()
$wall = [math]::Round($sw.Elapsed.TotalSeconds, 7)
"run exit=$($p.ExitCode) wall=$wall peak_working_set=$peak cpu_seconds=$([math]::Round($cpu, 3)) stderr_bytes=$((Get-Item $err).Length)"
if ($LogName -eq 'template-proof.log') {
    "wall_seconds=$wall cpu_seconds=$([math]::Round($cpu, 3)) measured_by=powershell_stopwatch_and_get-process_sampling" | Out-File -Encoding ascii (Join-Path $OutDir 'template-proof-timing.log')
    "{`n    `"WallSeconds`":  $wall,`n    `"PeakWorkingSet64`":  $peak`n}" | Out-File -Encoding utf8 (Join-Path $OutDir 'template-proof-memory.json')
    Copy-Item -LiteralPath $log -Destination (Join-Path $OutDir 'template-proof-memory.log') -Force
}
Get-Content $log -Encoding UTF8 | Where-Object { $_ -match '^(ACTION|DIRECTION)' } | ForEach-Object { $_ -replace ' twist_height_drift=\S+ twist_radius_drift=\S+', '' }
exit $p.ExitCode
