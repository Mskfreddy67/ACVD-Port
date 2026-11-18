// HLE: cellMic - Microphone Input Service (Offline Stubs)
#include "runtime/hle.h"
#include "runtime/log.h"

namespace rt {
namespace {

s32 cellMicOpenEx(u32 /*port*/, u32 /*opt1*/, u32 /*opt2*/) {
  LOG_I("mic", "cellMicOpenEx: no mic attached");
  return CELL_OK;
}

s32 cellMicClose(u32 /*port*/) { return CELL_OK; }
s32 cellMicStart(u32 /*port*/) { return CELL_OK; }
s32 cellMicStop(u32 /*port*/) { return CELL_OK; }

s32 cellMicRead(u32 /*port*/, u32 /*buf*/, u32 /*size*/, Ptr<be32> readSize) {
  if (readSize) *readSize = 0;
  return CELL_OK;
}

s32 cellMicSetNotifyEventQueue(u64 /*key*/) { return CELL_OK; }

}  // namespace

static hle::Module s_module("cellMic", {
    HLE_FN(cellMicClose),
    HLE_FN(cellMicOpenEx),
    HLE_FN(cellMicRead),
    HLE_FN(cellMicSetNotifyEventQueue),
    HLE_FN(cellMicStart),
    HLE_FN(cellMicStop),
});

}  // namespace rt
