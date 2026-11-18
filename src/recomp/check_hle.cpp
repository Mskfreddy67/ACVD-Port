#include <cstdio>
#include <vector>
#include <string>
#include <map>
#include <set>
#include "common/file.h"
#include "recomp/image.h"
#include "runtime/hle.h"

// Reference runtime HLE registrations
namespace rt::hle {
void register_all_modules();
}

int main(int argc, char** argv) {
    if (argc < 2) return 1;
    recomp::Image img(ac4::read_file(argv[1]));
    
    // We can also parse all known HLE by querying hle::find
    rt::hle::Resolved res;
    std::map<std::string, std::vector<std::pair<u32, std::string>>> missing;
    std::map<std::string, std::vector<std::pair<u32, std::string>>> found;

    for (const auto& im : img.imports()) {
        if (rt::hle::find(im.module, im.fnid, res)) {
            found[im.module].push_back({im.fnid, res.name});
        } else {
            missing[im.module].push_back({im.fnid, ""});
        }
    }

    std::printf("=== HLE IMPORT AUDIT FOR %s ===\n", argv[1]);
    size_t total_missing = 0, total_found = 0;
    for (const auto& [mod, list] : missing) total_missing += list.size();
    for (const auto& [mod, list] : found) total_found += list.size();
    std::printf("Total imports: %zu, Found in HLE: %zu, Missing in HLE: %zu\n\n",
                total_found + total_missing, total_found, total_missing);

    std::printf("--- MISSING BY MODULE ---\n");
    for (const auto& [mod, list] : missing) {
        std::printf("[%s]: %zu missing\n", mod.c_str(), list.size());
        for (const auto& item : list) {
            std::printf("  0x%08x\n", item.first);
        }
    }
    return 0;
}
