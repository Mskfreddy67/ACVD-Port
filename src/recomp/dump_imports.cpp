#include <cstdio>
#include <vector>
#include <string>
#include "common/file.h"
#include "recomp/image.h"

int main(int argc, char** argv) {
    if (argc < 2) return 1;
    recomp::Image img(ac4::read_file(argv[1]));
    for (const auto& im : img.imports()) {
        std::printf("%s:0x%08x:0x%08x\n", im.module.c_str(), im.fnid, im.slot);
    }
    return 0;
}
