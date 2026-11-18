// HLE: cellVoice - VoIP Voice Chat (Offline / Device Not Present Stubs)
#include "runtime/hle.h"
#include "runtime/log.h"

namespace rt {
namespace {

constexpr s32 CELL_VOICE_ERROR_NOT_INITIALIZED = s32(0x80310601);
constexpr s32 CELL_VOICE_ERROR_PORT_NOT_FOUND  = s32(0x80310607);

s32 cellVoiceInit(u32 /*arg1*/, u32 /*arg2*/, u32 /*arg3*/, u32 /*arg4*/) {
  LOG_I("voice", "cellVoiceInit: voice chat disabled");
  return CELL_OK;
}

s32 cellVoiceEnd() {
  LOG_I("voice", "cellVoiceEnd");
  return CELL_OK;
}

s32 cellVoiceStart() { return CELL_OK; }
s32 cellVoiceStop() { return CELL_OK; }

s32 cellVoiceCreatePort(u32 /*param*/, Ptr<be32> portId) {
  if (portId) *portId = 1;
  return CELL_OK;
}

s32 cellVoiceDeletePort(u32 /*portId*/) { return CELL_OK; }
s32 cellVoiceResetPort(u32 /*portId*/) { return CELL_OK; }
s32 cellVoiceGetPortInfo(u32 /*portId*/, u32 /*info*/) { return CELL_OK; }

s32 cellVoiceConnectIPortToOPort(u32 /*iPort*/, u32 /*oPort*/) { return CELL_OK; }
s32 cellVoiceDisconnectIPortFromOPort(u32 /*iPort*/, u32 /*oPort*/) { return CELL_OK; }

s32 cellVoiceReadFromOPort(u32 /*portId*/, u32 /*buf*/, Ptr<be32> size) {
  if (size) *size = 0;
  return CELL_OK;
}

s32 cellVoiceWriteToIPortEx(u32 /*portId*/, u32 /*buf*/, Ptr<be32> size, u32 /*opt*/) {
  return CELL_OK;
}

s32 cellVoiceCreateNotifyEventQueue(Ptr<be64> /*key*/, Ptr<be32> queueId) {
  if (queueId) *queueId = 1;
  return CELL_OK;
}

s32 cellVoiceSetNotifyEventQueue(u64 /*key*/) { return CELL_OK; }
s32 cellVoiceRemoveNotifyEventQueue(u64 /*key*/) { return CELL_OK; }

}  // namespace

static hle::Module s_module("cellVoice", {
    HLE_FN(cellVoiceConnectIPortToOPort),
    HLE_FN(cellVoiceCreateNotifyEventQueue),
    HLE_FN(cellVoiceCreatePort),
    HLE_FN(cellVoiceDeletePort),
    HLE_FN(cellVoiceDisconnectIPortFromOPort),
    HLE_FN(cellVoiceEnd),
    HLE_FN(cellVoiceGetPortInfo),
    HLE_FN(cellVoiceInit),
    HLE_FN(cellVoiceReadFromOPort),
    HLE_FN(cellVoiceRemoveNotifyEventQueue),
    HLE_FN(cellVoiceResetPort),
    HLE_FN(cellVoiceSetNotifyEventQueue),
    HLE_FN(cellVoiceStart),
    HLE_FN(cellVoiceStop),
    HLE_FN(cellVoiceWriteToIPortEx),
});

}  // namespace rt
