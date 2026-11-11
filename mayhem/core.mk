# Compiles the snes9x core (libretro source list + zip + JMA) into $(OUT)/libs9x.a.
# Invoked by mayhem/build.sh with: make -f mayhem/core.mk OUT=<dir> CORE_CFLAGS="<flags>"
SRCDIR  := .
DEFS    := -DRIGHTSHIFT_IS_SAR -D__LIBRETRO__ -DALLOW_CPU_OVERCLOCK -DHAVE_STDINT_H -DHAVE_STRINGS_H -DZLIB -DUNZIP_SUPPORT -DJMA_SUPPORT
INCS    := -Ilibretro -Ilibretro/libretro-common/include -I. -Iapu -Iapu/bapu -Iunzip -Ijma -Ifilter

CXX_SRCS := apu/apu.cpp apu/bapu/dsp/sdsp.cpp apu/bapu/smp/smp.cpp apu/bapu/smp/smp_state.cpp \
  bsx.cpp c4.cpp c4emu.cpp cheats.cpp cheats2.cpp clip.cpp conffile.cpp controls.cpp cpu.cpp \
  cpuexec.cpp cpuops.cpp crosshairs.cpp dma.cpp dsp.cpp dsp1.cpp dsp2.cpp dsp3.cpp dsp4.cpp \
  fxinst.cpp fxemu.cpp gfx.cpp globals.cpp memmap.cpp obc1.cpp msu1.cpp ppu.cpp stream.cpp \
  sa1.cpp sa1cpu.cpp screenshot.cpp sdd1.cpp sdd1emu.cpp seta.cpp seta010.cpp seta011.cpp \
  seta018.cpp snapshot.cpp snes9x.cpp spc7110.cpp srtc.cpp tile.cpp tileimpl-n1x1.cpp \
  tileimpl-n2x1.cpp tileimpl-h2x1.cpp sha256.cpp bml.cpp movie.cpp fscompat.cpp \
  loadzip.cpp libretro/libretro.cpp \
  jma/7zlzma.cpp jma/crc32.cpp jma/iiostrm.cpp jma/inbyte.cpp jma/jma.cpp jma/lzma.cpp \
  jma/lzmadec.cpp jma/s9x-jma.cpp jma/winout.cpp
C_SRCS   := filter/snes_ntsc.c unzip/ioapi.c unzip/unzip.c

OBJS := $(patsubst %.cpp,$(OUT)/%.o,$(CXX_SRCS)) $(patsubst %.c,$(OUT)/%.o,$(C_SRCS))

$(OUT)/libs9x.a: $(OBJS)
	rm -f $@
	ar rcs $@ $(OBJS)

# CMemory::ScoreHiROM computes `1 << (rom_size_byte - 7)`; any header size byte < 7 is a negative shift.
# Relaxed here only (a benign pattern that would otherwise halt on most mutated cartridges).
$(OUT)/memmap.o: RELAX := -fno-sanitize=shift-exponent

$(OUT)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(DEFS) $(INCS) $(CORE_CFLAGS) $(RELAX) -fexceptions -c $< -o $@

$(OUT)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(DEFS) $(INCS) $(CORE_CFLAGS) -c $< -o $@
