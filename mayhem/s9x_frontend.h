// Headless libretro frontend used by every harness: initialises the core once and offers a
// synthetic 128 KiB LoROM cartridge so state-dependent parsers (snapshots, cheats) have a loaded game.
#pragma once
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <unistd.h>
#include "libretro.h"

namespace s9xfe {

inline bool env_cb(unsigned cmd, void *data)
{
    switch (cmd) {
    case RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY:
    case RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY:
        *static_cast<const char **>(data) = ".";
        return true;
    case RETRO_ENVIRONMENT_SET_PIXEL_FORMAT:
    case RETRO_ENVIRONMENT_SET_PERFORMANCE_LEVEL:
    case RETRO_ENVIRONMENT_SET_SUPPORT_ACHIEVEMENTS:
    case RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS:
    case RETRO_ENVIRONMENT_SET_CONTROLLER_INFO:
    case RETRO_ENVIRONMENT_SET_GEOMETRY:
        return true;
    default:
        return false;
    }
}
inline void video_cb(const void *, unsigned, unsigned, size_t) {}
inline void audio_cb(int16_t, int16_t) {}
inline size_t audio_batch_cb(const int16_t *, size_t frames) { return frames; }
inline void input_poll_cb() {}
inline int16_t input_state_cb(unsigned, unsigned, unsigned, unsigned) { return 0; }

inline void init()
{
    retro_set_environment(env_cb);
    retro_set_video_refresh(video_cb);
    retro_set_audio_sample(audio_cb);
    retro_set_audio_sample_batch(audio_batch_cb);
    retro_set_input_poll(input_poll_cb);
    retro_set_input_state(input_state_cb);
    retro_init();
}

inline bool load(const uint8_t *data, size_t size)
{
    retro_game_info g = {nullptr, data, size, nullptr};
    return retro_load_game(&g);
}

// 128 KiB LoROM. Reset code (native mode, 16-bit) sums ten times 7 into WRAM[$10], then spins:
//   SEI CLC XCE LDA #$80 STA $2100 REP #$30 LDA #0 LDX #10 / CLC ADC #7 DEX BNE / STA $0010 / JMP *
constexpr size_t kRomSize = 0x20000;
inline void make_rom(uint8_t *rom)
{
    static const uint8_t code[] = {
        0x78, 0x18, 0xFB, 0xA9, 0x80, 0x8D, 0x00, 0x21, 0xC2, 0x30, 0xA9, 0x00, 0x00,
        0xA2, 0x0A, 0x00, 0x18, 0x69, 0x07, 0x00, 0xCA, 0xD0, 0xF9,
        0x8D, 0x10, 0x00, 0x4C, 0x1A, 0x80,
    };
    memset(rom, 0, kRomSize);
    memcpy(rom, code, sizeof(code));
    memset(rom + 0x7FC0, ' ', 21);
    memcpy(rom + 0x7FC0, "MAYHEM TEST ROM", 15);
    rom[0x7FD5] = 0x20;  // LoROM, slow
    rom[0x7FD6] = 0x00;  // ROM only
    rom[0x7FD7] = 0x07;  // 128 KiB
    rom[0x7FD8] = 0x00;  // no SRAM
    rom[0x7FD9] = 0x01;  // USA
    rom[0x7FDA] = 0x33;
    rom[0x7FFC] = 0x00;  // reset vector $8000
    rom[0x7FFD] = 0x80;
    uint32_t sum = 0x1FE;  // complement + checksum bytes always add up to 0x1FE
    for (size_t i = 0; i < kRomSize; i++) sum += rom[i];
    uint16_t cs = sum & 0xFFFF, cc = cs ^ 0xFFFF;
    rom[0x7FDC] = cc & 0xFF; rom[0x7FDD] = cc >> 8;
    rom[0x7FDE] = cs & 0xFF; rom[0x7FDF] = cs >> 8;
}

// Writes data to a fresh file under $TMPDIR (mkstemp); returns path in out (size n), or false.
inline bool write_temp(const uint8_t *data, size_t size, char *out, size_t n, const char *suffix = "")
{
    const char *dir = getenv("TMPDIR");
    snprintf(out, n, "%s/s9xfuzz.XXXXXX%s", (dir && *dir) ? dir : "/tmp", suffix);
    int fd = mkstemps(out, (int)strlen(suffix));
    if (fd < 0) return false;
    size_t off = 0;
    while (off < size) {
        ssize_t w = write(fd, data + off, size - off);
        if (w <= 0) { close(fd); unlink(out); return false; }
        off += (size_t)w;
    }
    close(fd);
    return true;
}

}  // namespace s9xfe
