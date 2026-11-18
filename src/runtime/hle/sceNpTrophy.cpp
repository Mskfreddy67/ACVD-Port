// HLE: sceNpTrophy - Trophy Service (Offline Stubs)
#include <cstring>
#include "runtime/hle.h"
#include "runtime/log.h"

namespace rt {
namespace {

s32 sceNpTrophyInit(u32 /*pool*/, u32 /*poolSize*/, u32 /*opt1*/, u32 /*opt2*/) {
  LOG_I("trophy", "sceNpTrophyInit called");
  return CELL_OK;
}

s32 sceNpTrophyTerm() {
  LOG_I("trophy", "sceNpTrophyTerm called");
  return CELL_OK;
}

s32 sceNpTrophyCreateContext(Ptr<be32> context, u32 commId, u32 commSign, u64 options) {
  LOG_I("trophy", "sceNpTrophyCreateContext(commId=0x%x, commSign=0x%x, opt=0x%llx)", commId, commSign, (unsigned long long)options);
  if (context) *context = 1;
  return CELL_OK;
}

s32 sceNpTrophyDestroyContext(u32 /*context*/) {
  LOG_I("trophy", "sceNpTrophyDestroyContext called");
  return CELL_OK;
}

s32 sceNpTrophyCreateHandle(Ptr<be32> handle) {
  LOG_I("trophy", "sceNpTrophyCreateHandle called");
  if (handle) *handle = 1;
  return CELL_OK;
}

s32 sceNpTrophyDestroyHandle(u32 /*handle*/) {
  LOG_I("trophy", "sceNpTrophyDestroyHandle called");
  return CELL_OK;
}

constexpr u32 SCE_NP_TROPHY_STATUS_NOT_INSTALLED = 1;
constexpr u32 SCE_NP_TROPHY_STATUS_DATA_CORRUPT = 2;
constexpr u32 SCE_NP_TROPHY_STATUS_INSTALLED = 3;
constexpr u32 SCE_NP_TROPHY_STATUS_REQUIRES_UPDATE = 4;
constexpr u32 SCE_NP_TROPHY_STATUS_PROCESSING_SETUP = 5;
constexpr u32 SCE_NP_TROPHY_STATUS_PROCESSING_PROGRESS = 6;
constexpr u32 SCE_NP_TROPHY_STATUS_INSTALLING = 6;
constexpr u32 SCE_NP_TROPHY_STATUS_PROCESSING_FINALIZE = 7;
constexpr u32 SCE_NP_TROPHY_STATUS_PROCESSING_COMPLETE = 8;

struct SceNpTrophyGameDetails {
  be32 numTrophies;
  be32 numPlatinum;
  be32 numGold;
  be32 numSilver;
  be32 numBronze;
  char title[128];
  char description[1024];
  u8 reserved[4];
};

struct SceNpTrophyGameData {
  be32 unlockedTrophies;
  be32 unlockedPlatinum;
  be32 unlockedGold;
  be32 unlockedSilver;
  be32 unlockedBronze;
};

s32 sceNpTrophyRegisterContext(ppu::Ctx& ctx, u32 context, u32 handle, u32 statusCb, u32 arg, u64 options) {
  LOG_I("trophy", "sceNpTrophyRegisterContext(context=%u, handle=%u, cb=0x%x, arg=0x%x, opt=0x%llx)",
        context, handle, statusCb, arg, (unsigned long long)options);
  if (statusCb) {
    // PS3 SDK callback sequence: NOT_INSTALLED -> INSTALLING -> INSTALLED -> PROCESSING_FINALIZE -> PROCESSING_COMPLETE
    hle::call_guest(ctx, statusCb, {context, SCE_NP_TROPHY_STATUS_NOT_INSTALLED, 0, 51, arg});
    hle::call_guest(ctx, statusCb, {context, SCE_NP_TROPHY_STATUS_INSTALLING, 25, 51, arg});
    hle::call_guest(ctx, statusCb, {context, SCE_NP_TROPHY_STATUS_INSTALLED, 51, 51, arg});
    hle::call_guest(ctx, statusCb, {context, SCE_NP_TROPHY_STATUS_PROCESSING_FINALIZE, 51, 51, arg});
    hle::call_guest(ctx, statusCb, {context, SCE_NP_TROPHY_STATUS_PROCESSING_COMPLETE, 51, 51, arg});
  }
  return CELL_OK;
}

s32 sceNpTrophyGetGameInfo(u32 context, u32 handle, Ptr<SceNpTrophyGameDetails> details, Ptr<SceNpTrophyGameData> data) {
  LOG_I("trophy", "sceNpTrophyGetGameInfo(context=%u, handle=%u)", context, handle);
  if (details) {
    details->numTrophies = 51;
    details->numPlatinum = 1;
    details->numGold = 3;
    details->numSilver = 5;
    details->numBronze = 42;
    std::memset(details->title, 0, sizeof(details->title));
    std::strncpy(details->title, "ARMORED CORE VERDICT DAY", sizeof(details->title) - 1);
    std::memset(details->description, 0, sizeof(details->description));
    std::strncpy(details->description, "ARMORED CORE VERDICT DAY Trophy Set", sizeof(details->description) - 1);
    std::memset(details->reserved, 0, sizeof(details->reserved));
  }
  if (data) {
    data->unlockedTrophies = 0;
    data->unlockedPlatinum = 0;
    data->unlockedGold = 0;
    data->unlockedSilver = 0;
    data->unlockedBronze = 0;
  }
  return CELL_OK;
}

s32 sceNpTrophyGetRequiredDiskSpace(u32 /*context*/, u32 /*handle*/, Ptr<be64> reqSpace, u64 /*options*/) {
  LOG_I("trophy", "sceNpTrophyGetRequiredDiskSpace called");
  if (reqSpace) *reqSpace = 0;
  return CELL_OK;
}

s32 sceNpTrophyUnlockTrophy(u32 /*context*/, u32 /*handle*/, u32 trophyId, u32 /*platData*/) {
  LOG_I("trophy", "sceNpTrophyUnlockTrophy(trophyId=%u)", trophyId);
  return CELL_OK;
}

}  // namespace

static hle::Module s_module("sceNpTrophy", {
    HLE_FN(sceNpTrophyCreateContext),
    HLE_FN(sceNpTrophyCreateHandle),
    HLE_FN(sceNpTrophyDestroyContext),
    HLE_FN(sceNpTrophyDestroyHandle),
    HLE_FN(sceNpTrophyGetGameInfo),
    HLE_FN(sceNpTrophyGetRequiredDiskSpace),
    HLE_FN(sceNpTrophyInit),
    HLE_FN(sceNpTrophyRegisterContext),
    HLE_FN(sceNpTrophyTerm),
    HLE_FN(sceNpTrophyUnlockTrophy),
});

}  // namespace rt
