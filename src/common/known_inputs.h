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

// The game: ARMORED CORE 4, USA disc BLUS30027 (VERSION 01.01; no update exists in the inputs).
constexpr const char* kTitleName = "ARMORED CORE 4";
constexpr const char* kTitleId = "BLUS30027";
constexpr const char* kDiscVersion = "01.01";

// ACVD-Port: ARMORED CORE: VERDICT DAY title identifiers.
// SHA-256 and size of ACVD's EBOOT.elf are discovered at import time (--game acvd) and written to
// the manifest; they are NOT hardcoded here because we don't ship a known-good hash until at least
// one verified dump has been tested.
constexpr const char* kAcvdTitleIdUs  = "BLUS31194";  // North America
constexpr const char* kAcvdTitleIdEu  = "BLES01898";  // Europe
constexpr const char* kAcvdTitleName  = "ARMORED CORE: VERDICT DAY";
// Known disc versions for ACVD (the import accepts any of these).
constexpr const char* kAcvdDiscVersions[] = {"01.00", "01.01", nullptr};
extern const Known kEbootBin;  // PS3_GAME/USRDIR/EBOOT.BIN (disc)
// The decrypted executable: RPCS3 writes EBOOT.elf next to EBOOT.BIN.
extern const Known kEbootElf;

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
