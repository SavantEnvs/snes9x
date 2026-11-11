// ZIP ROM extraction (minizip + loadzip.cpp).
#include "s9x_frontend.h"
#include "../port.h"
#include "../snes9x.h"
#include "../memmap.h"
#include "../unzip/unzip.h"
#include <vector>


extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    char path[512];
    if (size < 22 || !s9xfe::write_temp(data, size, path, sizeof(path), ".zip")) return -1;
    static std::vector<uint8_t> buf(0x800000 + 0x200);
    uint32 total = 0;
    bool8 ok = LoadZip(path, &total, buf.data(), (uint32)buf.size());
    unlink(path);
    return ok ? 0 : -1;
}
