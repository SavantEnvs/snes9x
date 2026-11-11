// Save-state loader (S9xUnfreezeGameMem) against a fixed loaded cartridge.
#include "s9x_frontend.h"
#include "../port.h"
#include "../snes9x.h"
#include "../memmap.h"
#include "../snapshot.h"

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
    if (size < 16) return -1;
    retro_reset();  // each input starts from the reset state of the same cartridge
    if (!retro_unserialize(data, size)) return -1;
    retro_run();
    return 0;
}
