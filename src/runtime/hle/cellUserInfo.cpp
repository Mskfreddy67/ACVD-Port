// HLE: cellUserInfo - User Profile & Account Info
#include <cstring>
#include <string>

#include "runtime/hle.h"
#include "runtime/log.h"

namespace rt {
namespace {

constexpr u32 CELL_USERINFO_USERNAME_SIZE = 64;
constexpr u32 CELL_USERINFO_USER_MAX = 16;

struct CellUserInfoUserStat {
  be32 id;
  char name[CELL_USERINFO_USERNAME_SIZE];
};

struct CellUserInfoUserList {
  be32 userId[CELL_USERINFO_USER_MAX];
};

struct CellUserInfoTypeSet {
  u32 title;
  be32 focus;
  be32 type;
  u32 reserved;
};

s32 cellUserInfoGetStat(u32 id, Ptr<CellUserInfoUserStat> stat) {
  LOG_I("userinfo", "cellUserInfoGetStat(id=%u)", id);
  if (!stat) return s32(0x8002c303);
  stat->id = 1;
  std::strncpy(stat->name, "Raven", CELL_USERINFO_USERNAME_SIZE - 1);
  stat->name[CELL_USERINFO_USERNAME_SIZE - 1] = '\0';
  return CELL_OK;
}

s32 cellUserInfoGetList(Ptr<be32> listNum, Ptr<CellUserInfoUserList> listBuf, Ptr<be32> currentUserId) {
  LOG_I("userinfo", "cellUserInfoGetList");
  if (listNum) *listNum = 1;
  if (listBuf) {
    std::memset(listBuf.get(), 0, sizeof(CellUserInfoUserList));
    listBuf->userId[0] = 1;
  }
  if (currentUserId) *currentUserId = 1;
  return CELL_OK;
}

s32 cellUserInfoSelectUser_ListType(Ptr<CellUserInfoTypeSet> listType, u32 funcSelect, u32 container, u32 userdata) {
  LOG_I("userinfo", "cellUserInfoSelectUser_ListType called");
  return CELL_OK;
}

}  // namespace

static hle::Module s_module("cellUserInfo", {
    HLE_FN(cellUserInfoGetStat),
    HLE_FN(cellUserInfoGetList),
    HLE_FN(cellUserInfoSelectUser_ListType),
});

}  // namespace rt
