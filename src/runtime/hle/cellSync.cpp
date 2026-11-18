// HLE: cellSync
#include "runtime/hle.h"

namespace rt {
namespace {

s32 cellSyncLFQueueGetDirection(u32 /*queue*/, Ptr<be32> direction) {
  if (direction) *direction = 0;
  return CELL_OK;
}

}  // namespace

static hle::Module s_module("cellSync", {
    HLE_FN(cellSyncLFQueueGetDirection),
});

}  // namespace rt
