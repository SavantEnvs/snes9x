// Known-answer probe (built WITHOUT sanitizers, dynamically linked): drives the same core with fixed
// inputs and prints computed values that mayhem/test.sh compares against golden output.
#include "s9x_frontend.h"
#include "../port.h"
#include "../snes9x.h"
#include "../memmap.h"
#include "../snapshot.h"
#include "../cheats.h"
#include "../bml.h"
#include "../conffile.h"
#include "../stream.h"
#include <string>
#include <vector>

static void dump_bml(bml_node &n, int d)
{
    printf("bml %d %s=%s\n", d, n.name.c_str(), n.data.c_str());
    for (auto &c : n.child) dump_bml(c, d + 1);
}

int main(int argc, char **argv)
{
    if (argc == 3 && !strcmp(argv[1], "--dump-rom")) {  // seed/fixture generator
        static uint8_t r[s9xfe::kRomSize];
        s9xfe::make_rom(r);
        FILE *f = fopen(argv[2], "wb");
        fwrite(r, 1, sizeof(r), f);
        fclose(f);
        return 0;
    }
    if (argc == 3 && !strcmp(argv[1], "--dump-state")) {  // seed generator for fuzz_snapshot
        s9xfe::init();
        static uint8_t r[s9xfe::kRomSize];
        s9xfe::make_rom(r);
        s9xfe::load(r, sizeof(r));
        retro_run();
        std::vector<uint8_t> s(retro_serialize_size());
        retro_serialize(s.data(), s.size());
        FILE *f = fopen(argv[2], "wb");
        fwrite(s.data(), 1, s.size(), f);
        fclose(f);
        return 0;
    }
    if (argc < 4) { fprintf(stderr, "usage: kat <sample.bml> <sample.conf> <rom.zip>\n"); return 2; }
    s9xfe::init();

    static uint8_t rom[s9xfe::kRomSize];
    s9xfe::make_rom(rom);
    printf("load=%d\n", (int)s9xfe::load(rom, sizeof(rom)));
    printf("name=%s\n", Memory.ROMName);
    printf("size=%u\n", (unsigned)Memory.CalculatedSize);
    printf("checksum=%04x\n", (unsigned)Memory.ROMChecksum);
    printf("lorom=%d\n", (int)Memory.LoROM);
    retro_run();
    retro_run();
    printf("ram10=%02x%02x\n", Memory.RAM[0x11], Memory.RAM[0x10]);

    // save-state round trip
    Memory.RAM[0x20] = 0xAB;
    std::vector<uint8_t> st(retro_serialize_size());
    printf("serialize=%d\n", (int)retro_serialize(st.data(), st.size()));
    Memory.RAM[0x20] = 0;
    printf("unserialize=%d\n", (int)retro_unserialize(st.data(), st.size()));
    printf("ram20=%02x\n", Memory.RAM[0x20]);
    printf("badstate=%d\n", (int)retro_unserialize(rom, 64));

    // cheats
    printf("cheatvalid=[%s]\n", S9xCheatValidate("7E0030=55").c_str());
    int idx = S9xAddCheatGroup("test", "7E0030=55");
    S9xEnableCheatGroup(idx);
    printf("cheattext=%s\n", S9xCheatGroupToText(idx).c_str());
    retro_run();
    printf("ram30=%02x\n", Memory.RAM[0x30]);
    S9xDeleteCheats();

    // BML
    bml_node root;
    printf("bmlparse=%d\n", (int)root.parse_file(argv[1]));
    dump_bml(root, 0);

    // config file
    ConfigFile cf;
    printf("conf.load=%d\n", (int)cf.LoadFile(argv[2]));
    printf("conf.fullscreen=%d\n", (int)cf.GetBool("Display::Fullscreen", false));
    printf("conf.buffer=%d\n", (int)cf.GetInt("Sound::BufferSize", -1));
    printf("conf.lang=%s\n", cf.GetString("Settings::Language", "none"));

    // zip
    static std::vector<uint8_t> buf(0x800000 + 0x200);
    uint32 total = 0;
    printf("zip=%d\n", (int)LoadZip(argv[3], &total, buf.data(), (uint32)buf.size()));
    printf("zipsize=%u\n", (unsigned)total);
    printf("zipmatch=%d\n", (int)(total == sizeof(rom) && !memcmp(buf.data(), rom, sizeof(rom))));
    return 0;
}
