// HLE: sceNp2 - PlayStation Network 2 / Matching2 (Offline Stubs)
#include "runtime/hle.h"
#include "runtime/log.h"

namespace rt {

s32 sceNpInit(u32 poolsize, u32 poolptr);
s32 sceNpTerm();

namespace {

constexpr s32 SCE_NP_ERROR_OFFLINE = s32(0x8002aa0c);
constexpr s32 SCE_NP_MATCHING2_ERROR_NOT_INITIALIZED = s32(0x8002a701);

s32 sceNp2Init(u32 poolSize, u32 pool) {
  LOG_I("np2", "sceNp2Init: delegating to sceNpInit (poolSize=0x%x, pool=0x%x)", poolSize, pool);
  return sceNpInit(poolSize, pool ? pool : 1);
}

s32 sceNp2Term() {
  LOG_I("np2", "sceNp2Term: delegating to sceNpTerm");
  return sceNpTerm();
}

s32 sceNpMatching2Init2(u32 /*arg1*/, u32 /*arg2*/, u32 /*arg3*/) { return CELL_OK; }
s32 sceNpMatching2Term2() { return CELL_OK; }

s32 sceNpMatching2CreateContext(u32 /*param*/, Ptr<be32> ctxId) {
  if (ctxId) *ctxId = 1;
  return CELL_OK;
}

s32 sceNpMatching2DestroyContext(u32 /*ctxId*/) { return CELL_OK; }
s32 sceNpMatching2ContextStart(u32 /*ctxId*/) { return CELL_OK; }
s32 sceNpMatching2ContextStop(u32 /*ctxId*/) { return CELL_OK; }

s32 sceNpMatching2RegisterContextCallback(u32 /*ctxId*/, u32 /*cb*/, u32 /*arg*/) { return CELL_OK; }
s32 sceNpMatching2RegisterLobbyEventCallback(u32 /*ctxId*/, u32 /*cb*/, u32 /*arg*/) { return CELL_OK; }
s32 sceNpMatching2RegisterRoomEventCallback(u32 /*ctxId*/, u32 /*cb*/, u32 /*arg*/) { return CELL_OK; }
s32 sceNpMatching2RegisterSignalingCallback(u32 /*ctxId*/, u32 /*cb*/, u32 /*arg*/) { return CELL_OK; }

s32 sceNpMatching2GetServerIdListLocal(u32 /*ctxId*/, u32 /*list*/, Ptr<be32> count) {
  if (count) *count = 0;
  return CELL_OK;
}

s32 sceNpMatching2GetServerInfo(u32 /*ctxId*/, u32 /*serverId*/, u32 /*info*/) {
  return SCE_NP_ERROR_OFFLINE;
}

s32 sceNpMatching2GetWorldInfoList(u32 /*ctxId*/, u32 /*serverId*/, u32 /*param*/) {
  return SCE_NP_ERROR_OFFLINE;
}

s32 sceNpMatching2GetLobbyInfoList(u32 /*ctxId*/, u32 /*serverId*/, u32 /*worldId*/, u32 /*param*/) {
  return SCE_NP_ERROR_OFFLINE;
}

s32 sceNpMatching2JoinLobby(u32 /*ctxId*/, u32 /*lobbyId*/) { return SCE_NP_ERROR_OFFLINE; }
s32 sceNpMatching2LeaveLobby(u32 /*ctxId*/, u32 /*lobbyId*/) { return CELL_OK; }

s32 sceNpMatching2CreateJoinRoom(u32 /*ctxId*/, u32 /*param*/) { return SCE_NP_ERROR_OFFLINE; }
s32 sceNpMatching2JoinRoom(u32 /*ctxId*/, u32 /*roomId*/) { return SCE_NP_ERROR_OFFLINE; }
s32 sceNpMatching2LeaveRoom(u32 /*ctxId*/, u32 /*roomId*/) { return CELL_OK; }
s32 sceNpMatching2SearchRoom(u32 /*ctxId*/, u32 /*param*/) { return SCE_NP_ERROR_OFFLINE; }

s32 sceNpMatching2SetRoomDataExternal(u32 /*ctxId*/, u32 /*roomId*/, u32 /*data*/) { return CELL_OK; }
s32 sceNpMatching2SetRoomDataInternal(u32 /*ctxId*/, u32 /*roomId*/, u32 /*data*/) { return CELL_OK; }
s32 sceNpMatching2SetLobbyMemberDataInternal(u32 /*ctxId*/, u32 /*lobbyId*/, u32 /*data*/) { return CELL_OK; }
s32 sceNpMatching2GetLobbyMemberDataInternalList(u32 /*ctxId*/, u32 /*lobbyId*/, u32 /*param*/) { return SCE_NP_ERROR_OFFLINE; }

s32 sceNpMatching2GrantRoomOwner(u32 /*ctxId*/, u32 /*roomId*/, u32 /*target*/) { return CELL_OK; }
s32 sceNpMatching2KickoutRoomMember(u32 /*ctxId*/, u32 /*roomId*/, u32 /*target*/) { return CELL_OK; }

s32 sceNpMatching2SignalingGetConnectionStatus(u32 /*ctxId*/, u32 /*connId*/, Ptr<be32> status) {
  if (status) *status = 0;
  return CELL_OK;
}

s32 sceNpMatching2SignalingGetPingInfo(u32 /*ctxId*/, u32 /*connId*/, u32 /*info*/) {
  return SCE_NP_ERROR_OFFLINE;
}

s32 sceNpMatching2GetEventData(u32 /*ctxId*/, u32 /*buf*/, Ptr<be32> size) {
  if (size) *size = 0;
  return CELL_OK;
}

s32 sceNpMatching2ClearEventData(u32 /*ctxId*/) { return CELL_OK; }
s32 sceNpMatching2AbortRequest(u32 /*ctxId*/, u32 /*reqId*/) { return CELL_OK; }
s32 sceNpMatching2SetDefaultRequestOptParam(u32 /*param*/) { return CELL_OK; }
s32 sceNpMatching2GetMemoryInfo(u32 /*info*/) { return CELL_OK; }

}  // namespace

static hle::Module s_module("sceNp2", {
    HLE_FN(sceNp2Init),
    HLE_FN(sceNp2Term),
    HLE_FN(sceNpMatching2AbortRequest),
    HLE_FN(sceNpMatching2ClearEventData),
    HLE_FN(sceNpMatching2ContextStart),
    HLE_FN(sceNpMatching2ContextStop),
    HLE_FN(sceNpMatching2CreateContext),
    HLE_FN(sceNpMatching2CreateJoinRoom),
    HLE_FN(sceNpMatching2DestroyContext),
    HLE_FN(sceNpMatching2GetEventData),
    HLE_FN(sceNpMatching2GetLobbyInfoList),
    HLE_FN(sceNpMatching2GetLobbyMemberDataInternalList),
    HLE_FN(sceNpMatching2GetMemoryInfo),
    HLE_FN(sceNpMatching2GetServerIdListLocal),
    HLE_FN(sceNpMatching2GetServerInfo),
    HLE_FN(sceNpMatching2GetWorldInfoList),
    HLE_FN(sceNpMatching2GrantRoomOwner),
    HLE_FN(sceNpMatching2Init2),
    HLE_FN(sceNpMatching2JoinLobby),
    HLE_FN(sceNpMatching2JoinRoom),
    HLE_FN(sceNpMatching2KickoutRoomMember),
    HLE_FN(sceNpMatching2LeaveLobby),
    HLE_FN(sceNpMatching2LeaveRoom),
    HLE_FN(sceNpMatching2RegisterContextCallback),
    HLE_FN(sceNpMatching2RegisterLobbyEventCallback),
    HLE_FN(sceNpMatching2RegisterRoomEventCallback),
    HLE_FN(sceNpMatching2RegisterSignalingCallback),
    HLE_FN(sceNpMatching2SearchRoom),
    HLE_FN(sceNpMatching2SetDefaultRequestOptParam),
    HLE_FN(sceNpMatching2SetLobbyMemberDataInternal),
    HLE_FN(sceNpMatching2SetRoomDataExternal),
    HLE_FN(sceNpMatching2SetRoomDataInternal),
    HLE_FN(sceNpMatching2SignalingGetConnectionStatus),
    HLE_FN(sceNpMatching2SignalingGetPingInfo),
    HLE_FN(sceNpMatching2Term2),
});

}  // namespace rt
