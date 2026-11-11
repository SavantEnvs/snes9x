// Cheat code parser: Game Genie / Pro Action Replay / Gold Finger strings. Input is "<name>\n<code>".
#include "s9x_frontend.h"
#include "../port.h"
#include "../snes9x.h"
#include "../memmap.h"
#include "../cheats.h"
#include <string>

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
    std::string in(reinterpret_cast<const char *>(data), size);
    size_t nl = in.find('\n');
    if (nl == std::string::npos) return -1;
    std::string name = in.substr(0, nl), code = in.substr(nl + 1);

    retro_reset();  // each input starts from the reset state of the same cartridge
    S9xDeleteCheats();
    std::string err = S9xCheatValidate(code);
    int idx = S9xAddCheatGroup(name, code);
    if (idx < 0) return -1;
    S9xEnableCheatGroup(idx);
    S9xUpdateCheatsInMemory();
    S9xCheatGroupToText(idx);
    retro_run();
    S9xDeleteCheats();
    return 0;
}
