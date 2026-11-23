#!/usr/bin/env bash
# P1-A evidence capture: deterministic state via intent extras, same camera
# presets as M12. Waits for a non-black frame on every cold start.
set -u
SER=MAX0019111000104
OUT="docs/phases/P1A-geometry-poc/evidence"
ADB="E:/AndroidSdk/platform-tools/adb.exe"
PROBE="C:/Users/Giant/Desktop/GITCODE/Procedural Avatar Engine/tools/png_probe.py"
export MSYS_NO_PATHCONV=1

coverage() {
  python -I "$PROBE" "$1" --bottom 1005 2>/dev/null | awk '/coverage_pct/{print $2}'
}

capture_nonblack() {
  local target="$1" tmp="$OUT/_tmp.png" c="" try
  for try in $(seq 1 15); do
    sleep 1
    "$ADB" -s "$SER" exec-out screencap -p > "$tmp"
    c=$(coverage "$tmp")
    if [ -n "$c" ] && python -I -c "import sys; sys.exit(0 if float('$c')>1.0 else 1)" 2>/dev/null; then
      mv "$tmp" "$target"
      echo "  ok cov=${c}% (try $try) -> $target"
      return 0
    fi
  done
  mv "$tmp" "$target"
  echo "  WARN low coverage ${c}% -> $target" >&2
  return 1
}

start_case() { # preset view pose
  "$ADB" -s "$SER" shell am force-stop com.pae.host >/dev/null 2>&1
  sleep 0.6
  "$ADB" -s "$SER" shell am start -n com.pae.host/.MainActivity \
    --ei debug_generator 2 --ei debug_pose "$3" \
    --ei debug_preset "$1" --ei debug_view "$2" >/dev/null 2>&1
}

# Gray clay, neutral pose, all eight fixed cameras (same as M12 dv3_presetN).
for p in 0 1 2 3 4 5 6 7; do
  echo "clay preset$p"
  start_case "$p" 3 0
  capture_nonblack "$OUT/clay_preset$p.png"
done

# Wireframe and normals on representative cameras.
for spec in "0 wire_front 1" "6 wire_shoulder 1" "0 normals_front 2" "6 normals_shoulder 2"; do
  set -- $spec
  echo "$2"
  start_case "$1" "$3" 0
  capture_nonblack "$OUT/$2.png"
done

# Elbow bend action on front and shoulder cameras.
for spec in "0 action_elbow_front 2" "6 action_elbow_shoulder 2"; do
  set -- $spec
  echo "$2"
  start_case "$1" 3 "$3"
  capture_nonblack "$OUT/$2.png"
done

rm -f "$OUT/_tmp.png"
"$ADB" -s "$SER" logcat -d -t 400 > "$OUT/logcat-p1a.txt" 2>/dev/null || true
echo "capture done"
