#!/usr/bin/env bash
# M12-A anatomy evidence capture, DETERMINISTIC + self-verifying.
# - State is set ABSOLUTELY via `am start` intent extras (no button cycling).
# - After each cold start the GL surface needs a moment to attach, so we poll
#   screencap until the frame is non-black (body coverage > 1%) before saving.
set -u
SER=MAX0019111000104
OUT="docs/phases/M12-unified-human-geometry/evidence"
ADB="E:/AndroidSdk/platform-tools/adb.exe"
PROBE="C:/Users/Giant/Desktop/GITCODE/Procedural Avatar Engine/tools/png_probe.py"
export MSYS_NO_PATHCONV=1

coverage() { # prints body coverage % of the png at $1
  python -I "$PROBE" "$1" --bottom 1005 2>/dev/null | awk '/coverage_pct/{print $2}'
}

capture_nonblack() { # $1 = target png; retries until coverage > 1%
  local tmp="$OUT/_tmp.png" c="" try
  for try in $(seq 1 12); do
    sleep 1
    "$ADB" -s "$SER" exec-out screencap -p > "$tmp"
    c=$(coverage "$tmp")
    if [ -n "$c" ] && python -I -c "import sys; sys.exit(0 if float('$c')>1.0 else 1)" 2>/dev/null; then
      mv "$tmp" "$1"
      echo "  ok cov=${c}% (try $try)"
      return 0
    fi
  done
  mv "$tmp" "$1"
  echo "  WARN low coverage ${c}% after retries -> $1" >&2
  return 1
}

for dv in 0 1 2; do
  for p in 0 1 2 3 4 5 6 7; do
    "$ADB" -s "$SER" shell am force-stop com.pae.host 2>/dev/null
    sleep 0.8
    "$ADB" -s "$SER" shell am start -n com.pae.host/.MainActivity \
      --ez debug_anatomy true --ei debug_preset "$p" --ei debug_view "$dv" >/dev/null 2>&1
    echo "dv${dv} preset${p}:"
    capture_nonblack "$OUT/dv${dv}_preset${p}.png"
  done
done
rm -f "$OUT/_tmp.png"
echo "capture done"