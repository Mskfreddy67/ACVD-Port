// HLE: cellGame - Game Content, Boot, Patch & GameData management
#include <cstring>
#include <filesystem>
#include <string>

#include "common/file.h"
#include "common/sfo.h"
#include "runtime/config.h"
#include "runtime/hle.h"
#include "runtime/vfs.h"

namespace rt {
namespace {

namespace fs = std::filesystem;

constexpr s32 CELL_GAME_RET_OK = 0;
constexpr s32 CELL_GAME_ERROR_PARAM = s32(0x8002cb07);
constexpr s32 CELL_GAME_ERROR_NOTPATCH = s32(0x8002cb27);

constexpr u32 CELL_GAME_GAMETYPE_DISC = 1;

constexpr u32 CELL_GAME_PARAMID_TITLE = 0;
constexpr u32 CELL_GAME_PARAMID_TITLE_ID = 100;
constexpr u32 CELL_GAME_PARAMID_VERSION = 101;
constexpr u32 CELL_GAME_PARAMID_PARENTAL_LEVEL = 102;
constexpr u32 CELL_GAME_PARAMID_APP_VER = 106;

struct CellGameContentSize {
  bes32 hddFreeSizeKB;
  bes32 sizeKB;
  bes32 sysSizeKB;
};

struct CellGameSetInitParams {
  char title[128];
  char titleId[10];
  char reserved0[2];
  char version[6];
  char reserved1[66];
};

static std::string g_last_info_path = "/dev_bdvd/PS3_GAME";
static std::string g_last_usr_path = "/dev_bdvd/PS3_GAME/USRDIR";

s32 cellGameBootCheck(Ptr<be32> type, Ptr<be32> attributes, Ptr<CellGameContentSize> size, Ptr<char> dirName) {
  LOG_I("game", "cellGameBootCheck called");
  if (!type || !attributes) return CELL_GAME_ERROR_PARAM;
  *type = CELL_GAME_GAMETYPE_DISC;
  *attributes = 0;
  if (size) {
    size->hddFreeSizeKB = 50 * 1024 * 1024;  // 50 GB free
    size->sizeKB = 0;
    size->sysSizeKB = 0;
  }
  if (dirName) {
    const std::string& tid = cfg().title_id.empty() ? "BLUS31194" : cfg().title_id;
    write_cstr(dirName.addr, tid, 32);
  }
  g_last_info_path = "/dev_bdvd/PS3_GAME";
  g_last_usr_path = "/dev_bdvd/PS3_GAME/USRDIR";
  return CELL_OK;
}

s32 cellGamePatchCheck(Ptr<CellGameContentSize> size, u32 /*reserved*/) {
  LOG_I("game", "cellGamePatchCheck called");
  if (size) {
    size->hddFreeSizeKB = 50 * 1024 * 1024;
    size->sizeKB = 0;
    size->sysSizeKB = 0;
  }
  // If no patch is installed, PS3 returns CELL_GAME_ERROR_NOTPATCH so game boots cleanly
  return CELL_GAME_ERROR_NOTPATCH;
}

constexpr s32 CELL_GAME_RET_NONE = 1;

s32 cellGameDataCheck(u32 type, Ptr<char> dirName, Ptr<CellGameContentSize> size) {
  LOG_I("game", "cellGameDataCheck(type=%u, dirName=%s)", type, dirName ? dirName.get() : "null");
  if (size) {
    size->hddFreeSizeKB = 50 * 1024 * 1024;
    size->sizeKB = 0;
    size->sysSizeKB = 0;
  }
  if (dirName && dirName.get()) {
    std::string d = dirName.get();
    g_last_info_path = "/dev_hdd0/game/" + d;
    g_last_usr_path = "/dev_hdd0/game/" + d + "/USRDIR";
    std::string path = g_last_info_path + "/PARAM.SFO";
    vfs::Stat st;
    if (vfs::stat(path, &st) != CELL_OK) {
      LOG_I("game", "cellGameDataCheck: '%s' not installed -> returning CELL_GAME_RET_NONE", dirName.get());
      return CELL_GAME_RET_NONE;
    }
  }
  return CELL_GAME_RET_OK;
}

s32 cellGameContentPermit(Ptr<char> contentInfoPath, Ptr<char> usrdirPath) {
  LOG_I("game", "cellGameContentPermit called -> info=%s, usr=%s", g_last_info_path.c_str(), g_last_usr_path.c_str());
  if (!contentInfoPath || !usrdirPath) return CELL_GAME_ERROR_PARAM;
  write_cstr(contentInfoPath.addr, g_last_info_path, 128);
  write_cstr(usrdirPath.addr, g_last_usr_path, 128);
  return CELL_OK;
}

s32 cellGameGetSizeKB(Ptr<bes32> size) {
  LOG_I("game", "cellGameGetSizeKB called");
  if (!size) return CELL_GAME_ERROR_PARAM;
  *size = 0;
  return CELL_OK;
}

s32 cellGameGetParamString(s32 id, Ptr<char> buf, u32 bufsize) {
  LOG_I("game", "cellGameGetParamString(id=%d, bufsize=%u)", id, bufsize);
  if (!buf || bufsize == 0) return CELL_GAME_ERROR_PARAM;
  std::string val;
  if (id == CELL_GAME_PARAMID_TITLE_ID) {
    val = cfg().title_id.empty() ? "BLUS31194" : cfg().title_id;
  } else if (id == CELL_GAME_PARAMID_TITLE) {
    val = cfg().title.empty() ? "ARMORED CORE: VERDICT DAY" : cfg().title;
  } else if (id == CELL_GAME_PARAMID_VERSION || id == CELL_GAME_PARAMID_APP_VER) {
    val = "01.00";
  } else {
    val = "";
  }
  write_cstr(buf.addr, val, bufsize);
  return CELL_OK;
}

s32 cellGameGetParamInt(s32 id, Ptr<bes32> value) {
  LOG_I("game", "cellGameGetParamInt(id=%d)", id);
  if (!value) return CELL_GAME_ERROR_PARAM;
  if (id == CELL_GAME_PARAMID_PARENTAL_LEVEL) {
    *value = 1;
  } else {
    *value = 0;
  }
  return CELL_OK;
}

s32 cellGameSetParamString(s32 id, Ptr<char> buf) {
  LOG_I("game", "cellGameSetParamString(id=%d)", id);
  return CELL_OK;
}

s32 cellGameCreateGameData(Ptr<CellGameSetInitParams> init, Ptr<char> tmp_contentInfoPath, Ptr<char> tmp_usrdirPath) {
  LOG_I("game", "cellGameCreateGameData called");
  const std::string& tid = cfg().title_id.empty() ? "BLUS31194" : cfg().title_id;
  std::string infoPath = "/dev_hdd0/game/" + tid;
  std::string usrPath = "/dev_hdd0/game/" + tid + "/USRDIR";
  if (tmp_contentInfoPath) {
    write_cstr(tmp_contentInfoPath.addr, infoPath, 128);
  }
  if (tmp_usrdirPath) {
    write_cstr(tmp_usrdirPath.addr, usrPath, 128);
  }
  return CELL_OK;
}

s32 cellGameContentErrorDialog(s32 type, s32 errNeedSizeKB, Ptr<char> dirName) {
  LOG_W("game", "cellGameContentErrorDialog(type=%d, size=%d)", type, errNeedSizeKB);
  return CELL_OK;
}

}  // namespace

static hle::Module s_module("cellGame", {
    HLE_FN(cellGameBootCheck),
    HLE_FN(cellGamePatchCheck),
    HLE_FN(cellGameDataCheck),
    HLE_FN(cellGameContentPermit),
    HLE_FN(cellGameGetSizeKB),
    HLE_FN(cellGameGetParamString),
    HLE_FN(cellGameGetParamInt),
    HLE_FN(cellGameSetParamString),
    HLE_FN(cellGameCreateGameData),
    HLE_FN(cellGameContentErrorDialog),
});

}  // namespace rt
