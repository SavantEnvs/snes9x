#!/usr/bin/env bash
# Builds the snes9x core (libretro source set + minizip + JMA) twice — once with the fuzzing
# sanitizers, once plain for the oracle — then links every harness (libFuzzer + -standalone) and
# the known-answer probe that mayhem/test.sh runs.
set -euo pipefail

[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH
: "${SANITIZER_FLAGS=-fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer}"
: "${DEBUG_FLAGS:=-g -gdwarf-3}"
: "${CC:=clang}" ; : "${CXX:=clang++}" ; : "${LIB_FUZZING_ENGINE:=-fsanitize=fuzzer}"
: "${MAYHEM_JOBS:=$(nproc)}"
: "${COVERAGE_FLAGS=}"
: "${SRC:=/mayhem}"
export SANITIZER_FLAGS DEBUG_FLAGS CC CXX LIB_FUZZING_ENGINE MAYHEM_JOBS COVERAGE_FLAGS

cd "$SRC"
OUT="$SRC/mayhem/build"
rm -rf "$OUT"
mkdir -p "$OUT/san" "$OUT/norm"

# Core library, instrumented for libFuzzer coverage (fuzzer-no-link) so Mayhem sees edges in it.
# alignment: the CPU/PPU/DMA cores read and write 16/32-bit values through casted byte pointers
# (cpuaddr.h, getset.h, ...) by design on x86; every emulated frame would otherwise halt.
RELAX="-fno-sanitize=alignment"
FUZZ_FLAGS="$SANITIZER_FLAGS $RELAX $DEBUG_FLAGS -fsanitize=fuzzer-no-link -O2"
make -s -f mayhem/core.mk -j"$MAYHEM_JOBS" OUT="$OUT/san" CORE_CFLAGS="$FUZZ_FLAGS" CC="$CC" CXX="$CXX"

# Independent plain build for the oracle (no sanitizers, normal optimisation).
make -s -f mayhem/core.mk -j"$MAYHEM_JOBS" OUT="$OUT/norm" CORE_CFLAGS="-O2 $COVERAGE_FLAGS" CC="$CC" CXX="$CXX"

DEFS="-DRIGHTSHIFT_IS_SAR -D__LIBRETRO__ -DALLOW_CPU_OVERCLOCK -DHAVE_STDINT_H -DHAVE_STRINGS_H -DZLIB -DUNZIP_SUPPORT -DJMA_SUPPORT"
INCS="-Imayhem -Ilibretro -Ilibretro/libretro-common/include -I. -Iapu -Iapu/bapu -Iunzip -Ijma -Ifilter"

$CXX $SANITIZER_FLAGS $DEBUG_FLAGS -O2 -c mayhem/lsan_off.cc -o "$OUT/lsan_off.o"
$CC $SANITIZER_FLAGS $DEBUG_FLAGS -c "$STANDALONE_FUZZ_MAIN" -o "$OUT/standalone_main.o"

TARGETS="fuzz_rom fuzz_snapshot fuzz_cheat fuzz_bml fuzz_conffile fuzz_zip"
for t in $TARGETS; do
  $CXX $SANITIZER_FLAGS $RELAX $DEBUG_FLAGS -O2 -fsanitize=fuzzer-no-link $DEFS $INCS -c "mayhem/$t.cc" -o "$OUT/$t.o"
  $CXX $SANITIZER_FLAGS $DEBUG_FLAGS $LIB_FUZZING_ENGINE "$OUT/$t.o" "$OUT/lsan_off.o" "$OUT/san/libs9x.a" -lz -o "$SRC/$t"
  $CXX $SANITIZER_FLAGS $DEBUG_FLAGS "$OUT/$t.o" "$OUT/standalone_main.o" "$OUT/lsan_off.o" "$OUT/san/libs9x.a" -lz -o "$SRC/$t-standalone"
done

# Oracle probe: plain build, dynamically linked.
$CXX -O2 $COVERAGE_FLAGS $DEFS $INCS mayhem/kat.cc "$OUT/norm/libs9x.a" -lz -o "$SRC/s9x_kat"
file "$SRC/s9x_kat" | grep -q 'dynamically linked'
