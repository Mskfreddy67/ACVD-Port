// The player's inputs in the keyless setup (docs/INPUTS.md): the disc folder (read in place), files
// the player decrypted (for example with RPCS3's Utilities > Decrypt PS3 Binaries) and the plain
// file that comes with them. Each file is accepted only with its SHA-256; the port holds no keys
// and decrypts nothing.
#pragma once
#include <string>

#include "common/bytes.h"

namespace ac4::inputs {

struct Known {
  const char* name;    // file name as RPCS3 writes it
  const char* sha256;  // lower-case hex
  u64 size;
};

// The game: ARMORED CORE: VERDICT DAY (USA disc BLUS31194 / EUR disc BLES01898).
constexpr const char* kTitleName = "ARMORED CORE: VERDICT DAY";
constexpr const char* kTitleId = "BLUS31194";
constexpr const char* kDiscVersion = "01.00";

constexpr const char* kAcvdTitleIdUs  = "BLUS31194";  // North America
constexpr const char* kAcvdTitleIdEu  = "BLES01898";  // Europe
constexpr const char* kAcvdTitleName  = "ARMORED CORE: VERDICT DAY";
// Known disc versions for ACVD.
constexpr const char* kAcvdDiscVersions[] = {"01.00", "01.01", nullptr};

// The system modules run natively, in load order (each gets the next fixed address slot), as
// decrypted from PS3 system software 4.93: RPCS3 writes <name>.prx next to <name>.sprx in
// dev_flash/sys/external.
constexpr const char* kSystemSoftware = "4.93";
constexpr int kModuleCount = 10;
extern const char* const kModules[kModuleCount];
// The accepted 4.93 decryption of a module (nullptr: not one of kModules).
const Known* module(const std::string& name);
// The system software version a decrypted module comes from when it is not the accepted one, as
// far as known ("4.89"); "" when unknown.
std::string module_other_version(const std::string& name, const std::string& sha256);

// dev_flash/sys/external/flashATRAC.pic: not encrypted, the same in every version; the ATRAC3plus
// decoder reads it at run time (the only dev_flash file the game opens).
extern const Known kFlashAtrac;

}  // namespace ac4::inputs
