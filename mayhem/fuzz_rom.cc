// Cartridge loader + emulator core: header scoring/deinterleave, coprocessor detection, then two frames.
// Calls CMemory::LoadROMMem directly (the core's own entry point, bounded by MAX_ROM_SIZE); the libretro
// wrapper retro_load_game reads past short buffers in is_bsx() before reaching the core.
#include "s9x_frontend.h"
#include "../port.h"
#include "../snes9x.h"
#include "../memmap.h"

extern "C" int LLVMFuzzerInitialize(int *, char ***)
{
    s9xfe::init();
    static uint8_t rom[s9xfe::kRomSize];
    s9xfe::make_rom(rom);
    if (!s9xfe::load(rom, sizeof(rom))) abort();
    return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (size == 0 || size > CMemory::MAX_ROM_SIZE) return -1;
    if (!Memory.LoadROMMem(data, (uint32)size, "fuzz")) return -1;
    Memory.ClearSRAM();
    retro_run();
    retro_run();
    return 0;
}
