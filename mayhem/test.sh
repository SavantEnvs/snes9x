#!/usr/bin/env bash
#
# mayhem/test.sh — runs the known-answer probe (s9x_kat, built by mayhem/build.sh) and checks its output.
# exit 0 = pass. EDIT per repo. PATCH-grade oracle: after an agent patches the source, the grader
# rebuilds (build.sh) then runs this. DELETE this file if the repo has no meaningful tests.
#
# IMPORTANT:
#  * Must assert BEHAVIOR/OUTPUT, not just exit status. The oracle has to check asserted values /
#    golden-output diffs / known-answer results — so a PATCH that "fixes" a bug by making the program
#    exit(0) (or any no-op) FAILS here. Running inputs and checking only "exit 0 / didn't crash" is
#    NOT a functional test (it's trivially reward-hackable) — use the project's real assertion suite.
#  * Do NOT build here — mayhem/build.sh already compiled the test suite (with the project's normal
#    flags). This script only RUNS the pre-built tests and reports counts. If the test runner is
#    missing, that's a build.sh bug — fail loudly rather than silently rebuilding.
#  * REQUIRED OUTPUT — a CTRF (https://ctrf.io) summary so Mayhem/the PATCH grader reads the counts:
#      - writes a CTRF JSON report to ${CTRF_REPORT:-$SRC/ctrf-report.json}, and
#      - prints a one-line `CTRF {...}` marker to stdout (same JSON, compact).
#    Only `results.summary` (with tests/passed/failed/pending/skipped/other) is required.
#    Use the emit_ctrf helper below; it computes tests = passed+failed+skipped and sets the exit
#    code (0 iff failed==0). Map your framework's output to passed/failed/skipped.
set -uo pipefail
[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH
: "${MAYHEM_JOBS:=$(nproc)}"   # build parallelism; env-overridable, falls back to nproc (use -j"$MAYHEM_JOBS")
cd "$SRC"

# emit_ctrf <tool> <passed> <failed> [skipped] [pending] [other]
# Writes a CTRF report (file + stdout `CTRF {...}` marker) and returns non-zero iff failed>0.
emit_ctrf() {
  local tool="$1" passed="$2" failed="$3" skipped="${4:-0}" pending="${5:-0}" other="${6:-0}"
  local tests=$(( passed + failed + skipped + pending + other ))
  cat > "${CTRF_REPORT:-$SRC/ctrf-report.json}" <<JSON
{
  "results": {
    "tool": { "name": "$tool" },
    "summary": {
      "tests": $tests,
      "passed": $passed,
      "failed": $failed,
      "pending": $pending,
      "skipped": $skipped,
      "other": $other
    }
  }
}
JSON
  printf 'CTRF {"results":{"tool":{"name":"%s"},"summary":{"tests":%d,"passed":%d,"failed":%d,"pending":%d,"skipped":%d,"other":%d}}}\n' \
    "$tool" "$tests" "$passed" "$failed" "$pending" "$skipped" "$other"
  [ "$failed" -eq 0 ]
}


# s9x_kat is a plain (non-sanitized), dynamically linked build of the same core. It loads a synthetic
# 128 KiB LoROM whose reset code sums 10 x 7 into WRAM $10, runs frames, round-trips a save state, applies a
# cheat, parses BML / INI fixtures and extracts a ROM from a ZIP; every value below is computed by the code.
KAT="$SRC/s9x_kat"
[ -x "$KAT" ] || { echo "missing $KAT (mayhem/build.sh did not produce it)" >&2; emit_ctrf "s9x-kat" 0 1 0; exit 1; }

OUT="$("$KAT" mayhem/kat/sample.bml mayhem/kat/sample.conf mayhem/kat/rom.zip 2>/dev/null)"

EXPECTED=(
  'load=1'
  'name=MAYHEM TEST ROM'
  'size=131072'
  'checksum=120f'
  'lorom=1'
  'ram10=0046'
  'serialize=1'
  'unserialize=1'
  'ram20=ab'
  'badstate=0'
  'cheatvalid=[7e0030=55]'
  'cheattext=7e0030=55'
  'ram30=55'
  'bmlparse=1'
  'bml 2 revision=2024-01-01'
  'bml 3 description=Infinite Lives'
  'bml 3 code=7E0030=55'
  'bml 3 code=7E0031=FF'
  'bml 3 description=Max Gold'
  'bml 3 code=7E0040=99'
  'conf.load=1'
  'conf.fullscreen=1'
  'conf.buffer=80'
  'conf.lang=Klingon'
  'zip=1'
  'zipsize=131072'
  'zipmatch=1'
)

passed=0; failed=0
for line in "${EXPECTED[@]}"; do
  if printf '%s\n' "$OUT" | grep -Fxq -- "$line"; then
    passed=$((passed + 1))
  else
    failed=$((failed + 1)); echo "FAIL: expected line not produced: $line" >&2
  fi
done

emit_ctrf "s9x-kat" "$passed" "$failed" 0
