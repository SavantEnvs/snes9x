// BML (Byuu Markup Language) parser, as used by cheat databases and ROM databases.
#include "s9x_frontend.h"
#include "../bml.h"
#include <string>

static size_t walk(bml_node &n, int depth)
{
    size_t c = 1 + n.name.size() + n.data.size();
    if (depth > 4096) return c;
    for (auto &k : n.child) c += walk(k, depth + 1);
    return c;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    char path[512];
    if (!s9xfe::write_temp(data, size, path, sizeof(path))) return -1;
    bml_node root;
    bool ok = root.parse_file(path);
    unlink(path);
    if (!ok) return -1;
    walk(root, 0);
    root.find_subnode("cheat");
    return 0;
}
