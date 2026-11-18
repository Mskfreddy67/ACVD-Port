// HLE: sceNpCommerce2 - In-game Store / DLC (Offline Stubs)
#include "runtime/hle.h"
#include "runtime/log.h"

namespace rt {
namespace {

constexpr s32 SCE_NP_ERROR_OFFLINE = s32(0x8002aa0c);

s32 sceNpCommerce2Init(u32 /*mem*/, u32 /*size*/) {
  LOG_I("np", "sceNpCommerce2Init called (offline)");
  return CELL_OK;
}

s32 sceNpCommerce2ExecuteStoreBrowse(u32 /*ctx*/, u32 /*param*/, u32 /*cb*/, u32 /*arg*/) {
  LOG_W("np", "sceNpCommerce2ExecuteStoreBrowse called: store unavailable offline");
  return SCE_NP_ERROR_OFFLINE;
}

}  // namespace

static hle::Module s_module("sceNpCommerce2", {
    HLE_FN(sceNpCommerce2Init),
    HLE_FN(sceNpCommerce2ExecuteStoreBrowse),
});

}  // namespace rt
