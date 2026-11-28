#Requires -Version 5.1
<#
.SYNOPSIS
  PAE engineering-round orchestrator (mechanical stages only).

.DESCRIPTION
  Automates the deterministic, repeatable parts of a PAE round:
    baseline / build / test / boundary / recompress / verify-staged /
    commit / push / verify-remote / round (read-only core).

  GOVERNANCE (see SKILL.md): this script is a TOOL the agent invokes AFTER
  authorization. It does NOT decide authorization, single-variable discipline,
  test-standard lowering, or human visual gates. Those stay with the agent/owner.

  WRITE SAFETY:
    - commit / push REQUIRE -AllowWrite. Without it they refuse and print intent.
    - push is fast-forward only (never --force).
    - No reset / clean / stash / branch-delete / merge / rebase / file deletion.
    - Default stage 'baseline' is read-only.

.EXAMPLE
  # read-only mechanical core (baseline + build + test + boundary)
  .\pae_round.ps1 -Stage round -OutDir C:\temp\pae-round

.EXAMPLE
  # after owner approval: verify staged bytes, then commit + push + verify
  .\pae_round.ps1 -Stage verify-staged
  .\pae_round.ps1 -Stage commit -AllowWrite -MessageFile C:\temp\msg.txt
  .\pae_round.ps1 -Stage push -AllowWrite
  .\pae_round.ps1 -Stage verify-remote
#>
[CmdletBinding()]
param(
    [string]$Workspace = 'E:\AI-Tools\ProceduralAvatarEngine-poc-sm',
    [string]$Assets = 'E:\AI-Tools\PAE-human-template-poc\assets\makehuman-a8bc2d54\makehuman-a8bc2d54ff0ac92e78ff71431b1023eda42bf482',
    [ValidateSet('baseline','build','test','boundary','recompress','verify-staged','commit','push','verify-remote','round')]
    [string]$Stage = 'baseline',
    [string]$OutDir = (Join-Path $env:TEMP 'pae-round'),
    [string]$Branch = 'poc-sm/shape-model-v2',
    [string]$MessageFile = '',
    [string]$ImageSrc = '',
    [string]$ImageDst = '',
    [switch]$AllowWrite
)

$ErrorActionPreference = 'Stop'
$env:CYGWIN = 'nodosfilewarning'
if (-not (Test-Path -LiteralPath $OutDir)) { New-Item -ItemType Directory -Force -Path $OutDir | Out-Null }

# ---- paths -----------------------------------------------------------------
$HostDir      = Join-Path $Workspace 'tools\human-template-poc\host'
$BuildScript  = Join-Path $HostDir 'build.ps1'
$BuildDir     = Join-Path $Workspace 'build-host\human-template-poc'
$CtestDir     = Join-Path $Workspace 'build-host-ws'
$RecompressPy = Join-Path $HostDir 'tools\recompress_png.py'
$PahtRel      = 'app/src/main/assets/human_template/pae-human-base-mh-a8bc2d54.paht'
$PahtAbs      = Join-Path $Workspace ($PahtRel -replace '/','\')
$DataDir      = Join-Path $Assets 'makehuman\data'
$ProductionPaths = @('app/','native/renderer/','native/runtime/','native/procedural/human_template/','app/src/main/cpp/')

# ---- helpers ---------------------------------------------------------------
function Invoke-Git {
    param([string[]]$GitArgs)
    $output = & git -C $Workspace @GitArgs 2>&1
    $code = $LASTEXITCODE
    return [pscustomobject]@{ Code = $code; Text = ($output | ForEach-Object { "$_" }) }
}

function Write-Log {
    param([string]$Name, [string[]]$Lines)
    $path = Join-Path $OutDir $Name
    $Lines | Out-File -FilePath $path -Encoding utf8
    Write-Output "  log: $path"
}

function Get-PahtHash {
    if (Test-Path -LiteralPath $PahtAbs) {
        return (Get-FileHash -LiteralPath $PahtAbs -Algorithm SHA256).Hash.ToLower()
    }
    return '(missing)'
}

# ---- stages ----------------------------------------------------------------
function Show-Baseline {
    Write-Output '== BASELINE (read-only) =='
    $head = Invoke-Git @('rev-parse','HEAD')
    $br   = Invoke-Git @('branch','--show-current')
    $mail = Invoke-Git @('config','--local','user.email')
    $ab   = Invoke-Git @('rev-list','--left-right','--count',"origin/$Branch...HEAD")
    $st   = Invoke-Git @('status','--short','--untracked-files=no')
    Write-Output "  HEAD       : $($head.Text)"
    Write-Output "  branch     : $($br.Text)"
    Write-Output "  identity   : $($mail.Text)"
    Write-Output "  ahead/behind (remote...local): $($ab.Text)"
    Write-Output "  paht sha256: $(Get-PahtHash)"
    if ($st.Text) { Write-Output '  tracked changes (NOT auto-committed):'; $st.Text | ForEach-Object { Write-Output "    $_" } }
    else { Write-Output '  tracked    : clean' }
    $recent = Get-ChildItem -LiteralPath (Join-Path $HostDir 'src') -File |
        Sort-Object LastWriteTime -Descending | Select-Object -First 3
    Write-Output '  recent host src writes (other-agent check):'
    $recent | ForEach-Object { Write-Output ("    {0}  {1}" -f $_.LastWriteTime.ToString('MM-dd HH:mm'), $_.Name) }
    Write-Log 'baseline.txt' @(
        "HEAD=$($head.Text)", "branch=$($br.Text)", "identity=$($mail.Text)",
        "ahead_behind=$($ab.Text)", "paht=$(Get-PahtHash)", "tracked=$($st.Text -join '; ')"
    )
}

function Invoke-Build {
    Write-Output '== BUILD =='
    $log = @(& $BuildScript *>&1 | ForEach-Object { "$_" })
    $code = $LASTEXITCODE
    $warn = @($log | Where-Object { $_ -match 'warning:' })
    $err  = @($log | Where-Object { $_ -match 'error:' })
    Write-Output "  build exit=$code  warnings=$($warn.Count)  errors=$($err.Count)"
    $err | Select-Object -First 10 | ForEach-Object { Write-Output "    $_" }
    Write-Log 'build.log' $log
    if ($code -ne 0) { throw "build failed ($code)" }
}

function Invoke-Tests {
    Write-Output '== TEST REGRESSION =='
    $scratch = Join-Path $OutDir 'ht-scratch'
    New-Item -ItemType Directory -Force -Path $scratch | Out-Null
    $suites = @(
        @{ Name='wp3'; Exe=(Join-Path $BuildDir 'pae_wp3_tests.exe'); Args=@() },
        @{ Name='wp4'; Exe=(Join-Path $BuildDir 'pae_wp4_tests.exe'); Args=@() },
        @{ Name='ht';  Exe=(Join-Path $BuildDir 'pae_ht_tests.exe');  Args=@($DataDir, $Assets, $scratch) }
    )
    $summary = @()
    foreach ($suite in $suites) {
        if (-not (Test-Path -LiteralPath $suite.Exe)) { Write-Output "  $($suite.Name): EXE MISSING"; continue }
        $out = @(& $suite.Exe @($suite.Args) 2>&1 | ForEach-Object { "$_" })
        $code = $LASTEXITCODE
        $last = ($out | Select-Object -Last 1)
        Write-Output "  $($suite.Name): exit=$code  $last"
        Write-Log "tests-$($suite.Name).log" $out
        $summary += "$($suite.Name) exit=$code :: $last"
    }
    if (Test-Path -LiteralPath $CtestDir) {
        $ct = @(& ctest --test-dir $CtestDir 2>&1 | ForEach-Object { "$_" })
        $ctcode = $LASTEXITCODE
        $ctline = ($ct | Where-Object { $_ -match 'tests passed|tests failed' } | Select-Object -Last 1)
        Write-Output "  ctest: exit=$ctcode  $ctline"
        Write-Log 'tests-ctest.log' $ct
        $summary += "ctest exit=$ctcode :: $ctline"
    } else { Write-Output '  ctest: build-host-ws MISSING (skip)' }
    Write-Log 'tests-summary.txt' $summary
}

function Show-Boundary {
    Write-Output '== BOUNDARY (production zero-diff + paht invariance) =='
    $diffArgs = @('diff','--stat','--') + $ProductionPaths
    $diff = Invoke-Git $diffArgs
    if ($diff.Text) {
        Write-Output '  PRODUCTION PATH CHANGED (review required):'
        $diff.Text | ForEach-Object { Write-Output "    $_" }
    } else { Write-Output '  production paths: zero diff' }
    Write-Output "  paht sha256: $(Get-PahtHash)"
    Write-Log 'boundary.txt' @("production_diff=$($diff.Text -join '; ')", "paht=$(Get-PahtHash)")
}

function Invoke-Recompress {
    Write-Output '== RECOMPRESS EVIDENCE PNG (pixel round-trip) =='
    if (-not $ImageSrc -or -not $ImageDst) { throw 'recompress requires -ImageSrc and -ImageDst' }
    $out = @(& py $RecompressPy $ImageSrc $ImageDst 2>&1 | ForEach-Object { "$_" })
    $code = $LASTEXITCODE
    $out | Select-Object -Last 3 | ForEach-Object { Write-Output "  $_" }
    Write-Log 'recompress.log' $out
    if ($code -ne 0) { throw "recompress failed ($code)" }
}

function Test-StagedBytes {
    Write-Output '== VERIFY STAGED BYTES (check-attr aware) =='
    $staged = (Invoke-Git @('diff','--cached','--name-only')).Text
    if (-not $staged) { Write-Output '  nothing staged'; return }
    $bad = 0
    foreach ($rel in $staged) {
        $idx = (Invoke-Git @('rev-parse',":$rel")).Text
        $attr = (Invoke-Git @('check-attr','text','--',$rel)).Text
        $abs = Join-Path $Workspace ($rel -replace '/','\')
        if ($attr -match 'text: unset') {
            $wt = (Invoke-Git @('hash-object','--no-filters',$abs)).Text
        } else {
            $wt = (Invoke-Git @('hash-object',$abs)).Text
        }
        if ($idx -ne $wt) { $bad++; Write-Output "  MISMATCH: $rel" }
    }
    Write-Output "  staged=$($staged.Count)  byte-mismatches=$bad"
    Write-Log 'verify-staged.txt' @("staged=$($staged.Count)", "mismatches=$bad")
    if ($bad -gt 0) { throw "$bad staged file(s) differ from working tree" }
}

function Invoke-Commit {
    Write-Output '== COMMIT =='
    if (-not $AllowWrite) { Write-Output '  REFUSED: commit requires -AllowWrite (owner authorization). Intent only.'; return }
    if (-not $MessageFile -or -not (Test-Path -LiteralPath $MessageFile)) { throw 'commit requires existing -MessageFile' }
    $staged = (Invoke-Git @('diff','--cached','--name-only')).Text
    if (-not $staged) { throw 'nothing staged; stage task-grouped paths first' }
    $res = Invoke-Git @('commit','-F',$MessageFile)
    $res.Text | Select-Object -First 2 | ForEach-Object { Write-Output "  $_" }
    if ($res.Code -ne 0) { throw "commit failed ($($res.Code))" }
    $id = Invoke-Git @('log','-1','--format=%h %an <%ae> | %cn <%ce>')
    Write-Output "  identity: $($id.Text)"
    if ($id.Text -notmatch 'noreply\.github\.com') { Write-Output '  WARNING: identity is not noreply' }
    Write-Log 'commit.txt' @($res.Text, "identity=$($id.Text)")
}

function Invoke-Push {
    Write-Output '== PUSH (fast-forward only) =='
    if (-not $AllowWrite) { Write-Output '  REFUSED: push requires -AllowWrite (owner authorization). Intent only.'; return }
    $ok = $false; $attempt = 0; $out = @()
    foreach ($attempt in 1..4) {
        $res = Invoke-Git @('push','origin',$Branch)
        $out = $res.Text
        if ($res.Code -eq 0) { $ok = $true; break }
        Write-Output "  attempt $attempt failed:"; $out | Select-Object -First 2 | ForEach-Object { Write-Output "    $_" }
        Start-Sleep -Seconds 8
    }
    $out | ForEach-Object { Write-Output "  $_" }
    Write-Log 'push.txt' $out
    if (-not $ok) { throw "push failed after $attempt attempts" }
}

function Show-RemoteVerify {
    Write-Output '== VERIFY REMOTE (ls-remote) =='
    $ok = $false; $attempt = 0; $remote = @()
    foreach ($attempt in 1..5) {
        $res = Invoke-Git @('ls-remote','origin')
        if ($res.Code -eq 0) { $ok = $true; $remote = $res.Text; break }
        Start-Sleep -Seconds 6
    }
    if (-not $ok) { throw "ls-remote failed after $attempt attempts (network)" }
    $localHead = (Invoke-Git @('rev-parse','HEAD')).Text
    $remoteLine = ($remote | Where-Object { $_ -match "refs/heads/$Branch$" })
    $remoteSha = ($remoteLine -split '\s+')[0]
    $ab = (Invoke-Git @('rev-list','--left-right','--count',"origin/$Branch...HEAD")).Text
    Write-Output "  remote $Branch : $remoteSha"
    Write-Output "  local HEAD            : $localHead"
    Write-Output "  ahead/behind          : $ab"
    if ($remoteSha -eq $localHead) { Write-Output '  MATCH: remote == local' }
    else { Write-Output '  NOTE: remote != local (fetch may be stale; push then re-verify)' }
    Write-Log 'verify-remote.txt' @("remote=$remoteSha", "local=$localHead", "ahead_behind=$ab")
}

# ---- dispatch --------------------------------------------------------------
switch ($Stage) {
    'baseline'      { Show-Baseline }
    'build'         { Invoke-Build }
    'test'          { Invoke-Tests }
    'boundary'      { Show-Boundary }
    'recompress'    { Invoke-Recompress }
    'verify-staged' { Test-StagedBytes }
    'commit'        { Invoke-Commit }
    'push'          { Invoke-Push }
    'verify-remote' { Show-RemoteVerify }
    'round'         { Show-Baseline; Invoke-Build; Invoke-Tests; Show-Boundary }
}
Write-Output "stage '$Stage' done."