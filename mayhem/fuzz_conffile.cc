// INI-style configuration parser (snes9x.conf).
#include "s9x_frontend.h"
#include "../port.h"
#include "../conffile.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    char path[512];
    if (!s9xfe::write_temp(data, size, path, sizeof(path))) return -1;
    ConfigFile cf;
    bool ok = cf.LoadFile(path);
    unlink(path);
    if (!ok) return -1;
    cf.Exists("Display::Fullscreen");
    cf.GetBool("Display::Fullscreen", false);
    cf.GetInt("Sound::BufferSize", 0);
    cf.GetString("Settings::Language", "");
    cf.GetSection("Display");
    cf.Clear();
    return 0;
}
