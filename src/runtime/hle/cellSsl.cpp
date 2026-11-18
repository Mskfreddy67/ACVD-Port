// HLE: cellSsl - SSL & Certificate services (Offline Stubs)
#include "runtime/hle.h"
#include "runtime/log.h"

namespace rt {
namespace {

s32 cellSslInit(u32 /*pool*/, u32 /*poolSize*/) {
  LOG_I("ssl", "cellSslInit called");
  return CELL_OK;
}

s32 cellSslEnd() {
  LOG_I("ssl", "cellSslEnd called");
  return CELL_OK;
}

s32 cellSslCertificateLoader(u32 /*type*/, u32 /*buf*/, u32 /*bufSize*/, Ptr<be32> loadedSize) {
  LOG_I("ssl", "cellSslCertificateLoader called");
  if (loadedSize) *loadedSize = 0;
  return CELL_OK;
}

}  // namespace

static hle::Module s_module("cellSsl", {
    HLE_FN(cellSslInit),
    HLE_FN(cellSslEnd),
    HLE_FN(cellSslCertificateLoader),
});

}  // namespace rt
