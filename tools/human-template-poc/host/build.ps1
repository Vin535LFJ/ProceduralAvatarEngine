# Builds the P1-VR-ARCH-1 host-only prototype (chain A: pae_ht_tests + pae_ht_pipeline).
# Same toolchain as the engine's host build: native CMake + Cygwin make + g++ 11.
# The tool lives in tools/human-template-poc and is NOT part of the Android build:
# it reads three host evidence helpers from the engine checkout and modifies nothing.
param(
    [string]$PaeRepo = ''
)
$ErrorActionPreference = 'Stop'
$host_dir = Split-Path -Parent $MyInvocation.MyCommand.Path
# tools/human-template-poc/host -> repository root (three levels up).
$repo_root = Split-Path -Parent (Split-Path -Parent (Split-Path -Parent $host_dir))
if (-not $PaeRepo) { $PaeRepo = $repo_root }
# build-host/ is git-ignored, the same place the engine keeps its own host build.
$build = Join-Path $repo_root 'build-host\human-template-poc'
if (-not (Test-Path -LiteralPath $PaeRepo\native\procedural\shape\ClayRenderer.h)) {
    throw "PAE_REPO does not look like a ProceduralAvatarEngine checkout: $PaeRepo"
}
cmake -S $host_dir -B $build -G 'Unix Makefiles' `
    -DCMAKE_MAKE_PROGRAM=C:/cygwin64/bin/make.exe `
    -DCMAKE_CXX_COMPILER=C:/cygwin64/bin/c++.exe `
    -DCMAKE_BUILD_TYPE=Release `
    "-DPAE_REPO=$PaeRepo"
if ($LASTEXITCODE -ne 0) { throw "cmake configure failed ($LASTEXITCODE)" }
cmake --build $build -j 8
if ($LASTEXITCODE -ne 0) { throw "cmake build failed ($LASTEXITCODE)" }
Write-Output "built: $build\pae_ht_tests.exe, $build\pae_ht_pipeline.exe"
