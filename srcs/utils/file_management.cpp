#include "utils/file_management.h"
#include <fstream>
#include <cstdlib>

extern "C" int fm_read_file(const char *path, void **out_buf, size_t *out_size)
{
    if (!path || !out_buf || !out_size) return 0;
    std::ifstream   ifs(path, std::ios::binary | std::ios::ate);
    if (!ifs)       return 0;

    std::streamsize sz = ifs.tellg();   // size
    if (sz <= 0)    return 0;
    ifs.seekg(0, std::ios::beg);

    void *out = malloc((size_t)sz);
    if (!out) return 0;

    if (!ifs.read(static_cast<char *>(out), sz)) {
        free(out);
        return 0;
    }
    *out_buf = out;
    *out_size = (size_t)sz;
    return 1;
}
