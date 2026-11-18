// HLE: cellSpursJq
#include <atomic>
#include <cstdio>
#include <string>

#include "common/bytes.h"
#include "runtime/hle.h"
#include "runtime/log.h"
#include "runtime/lv2/lv2.h"
#include "runtime/memory.h"
#include "runtime/spurs_jq_diag.h"

namespace rt {

std::atomic<bool> g_draw_dropped_to_one{false};
std::atomic<uint64_t> g_jq_push_before_marker{0};
std::atomic<uint64_t> g_jq_push_after_marker{0};
std::atomic<uint64_t> g_jq_total_pushes{0};
std::atomic<bool> g_save_diagnostic_pngs_requested{false};

namespace {

static std::atomic<u64> s_jq_call_count{0};

static void log_hexdump_256(const char* label, u32 guest_addr) {
  if (!guest_addr) {
    LOG_I("spurs_jq", "  [%s @ 0x00000000 (nullptr)]", label);
    return;
  }
  if (!vm::committed(guest_addr, 0x100)) {
    LOG_I("spurs_jq", "  [%s @ 0x%08x: uncommitted or unmapped memory]", label, guest_addr);
    return;
  }
  const u8* p = ppu::mem() + guest_addr;
  LOG_I("spurs_jq", "  --- %s hexdump (0x%08x, 0x100 bytes) ---", label, guest_addr);
  for (int row = 0; row < 0x100; row += 16) {
    char hex[64];
    int pos = 0;
    for (int col = 0; col < 16; ++col) {
      pos += snprintf(hex + pos, sizeof(hex) - pos, "%02x ", p[row + col]);
    }
    LOG_I("spurs_jq", "  +0x%02x: %s", row, hex);
  }
}

#define LOG_JQ_CALL(name, fmt, ...) do { \
  u64 call_id = ++s_jq_call_count; \
  double t_sec = double(lv2::system_time_us()) / 1e6; \
  LOG_I("spurs_jq", "#%llu [%.4fs] " name "(" fmt ")", (unsigned long long)call_id, t_sec, ##__VA_ARGS__); \
} while (0)

s32 cellSpursJobQueueAttributeInitialize(u32 attr, u32 spurs, u32 job_bin, u32 sync_bin, u8 max_grab, u32 priority) {
  LOG_JQ_CALL("cellSpursJobQueueAttributeInitialize", "attr=0x%x, spurs=0x%x, job_bin=0x%x, sync_bin=0x%x, max_grab=%u, priority=%u",
              attr, spurs, job_bin, sync_bin, (u32)max_grab, priority);
  return CELL_OK;
}

s32 cellSpursJobQueueAttributeSetSubmitWithEntryLock(u32 attr) {
  LOG_JQ_CALL("cellSpursJobQueueAttributeSetSubmitWithEntryLock", "attr=0x%x", attr);
  return CELL_OK;
}

s32 _cellSpursCreateJobQueue(u32 jq, u32 attr) {
  LOG_JQ_CALL("_cellSpursCreateJobQueue", "jq=0x%x, attr=0x%x", jq, attr);
  return CELL_OK;
}

s32 _cellSpursCreateJobQueueWithJobDescriptorPool(u32 jq, u32 attr, u32 pool, u32 size) {
  LOG_JQ_CALL("_cellSpursCreateJobQueueWithJobDescriptorPool", "jq=0x%x, attr=0x%x, pool=0x%x, size=%u", jq, attr, pool, size);
  return CELL_OK;
}

s32 cellSpursJobQueuePortInitializeWithDescriptorBuffer(u32 jq, u32 port, u32 buf, u32 size) {
  LOG_JQ_CALL("cellSpursJobQueuePortInitializeWithDescriptorBuffer", "jq=0x%x, port=0x%x, buf=0x%x, size=%u", jq, port, buf, size);
  return CELL_OK;
}

s32 cellSpursJobQueuePort2Create(u32 port, u32 jq, u32 attr) {
  LOG_JQ_CALL("cellSpursJobQueuePort2Create", "port=0x%x, jq=0x%x, attr=0x%x", port, jq, attr);
  return CELL_OK;
}

s32 cellSpursJobQueuePort2Destroy(u32 port) {
  LOG_JQ_CALL("cellSpursJobQueuePort2Destroy", "port=0x%x", port);
  return CELL_OK;
}

s32 cellSpursJobQueuePort2Sync(u32 port) {
  LOG_JQ_CALL("cellSpursJobQueuePort2Sync", "port=0x%x", port);
  return CELL_OK;
}

s32 cellSpursJobQueuePortFinalize(u32 port) {
  LOG_JQ_CALL("cellSpursJobQueuePortFinalize", "port=0x%x", port);
  return CELL_OK;
}

s32 _cellSpursJobQueuePort2CopyPushJobBody(u32 port, u32 job, u32 size) {
  bool after = g_draw_dropped_to_one.load(std::memory_order_relaxed);
  if (after) {
    ++g_jq_push_after_marker;
  } else {
    ++g_jq_push_before_marker;
  }
  ++g_jq_total_pushes;
  LOG_JQ_CALL("_cellSpursJobQueuePort2CopyPushJobBody", "port=0x%x, job=0x%08x, size=%u [marker state: %s]",
              port, job, size, after ? "AFTER_MARKER" : "BEFORE_MARKER");
  log_hexdump_256("CopyPushJob descriptor", job);
  return CELL_OK;
}

s32 _cellSpursJobQueuePort2PushJobListBody(u32 port, u32 list, u32 num) {
  bool after = g_draw_dropped_to_one.load(std::memory_order_relaxed);
  if (after) {
    ++g_jq_push_after_marker;
  } else {
    ++g_jq_push_before_marker;
  }
  ++g_jq_total_pushes;
  LOG_JQ_CALL("_cellSpursJobQueuePort2PushJobListBody", "port=0x%x, list=0x%08x, num=%u [marker state: %s]",
              port, list, num, after ? "AFTER_MARKER" : "BEFORE_MARKER");
  log_hexdump_256("PushJobList list buffer", list);
  if (list && vm::committed(list, 4 * num)) {
    for (u32 i = 0; i < std::min(num, 4u); ++i) {
      u32 job_ptr = ppu::r32(list + i * 4);
      if (job_ptr && vm::committed(job_ptr, 0x100)) {
        log_hexdump_256(ac4::strprintf("PushJobList job[%u]", i).c_str(), job_ptr);
      }
    }
  }
  return CELL_OK;
}

s32 cellSpursJoinJobQueue(u32 jq) {
  LOG_JQ_CALL("cellSpursJoinJobQueue", "jq=0x%x", jq);
  return CELL_OK;
}

s32 cellSpursShutdownJobQueue(u32 jq) {
  LOG_JQ_CALL("cellSpursShutdownJobQueue", "jq=0x%x", jq);
  return CELL_OK;
}

}  // namespace

static hle::Module s_module("cellSpursJq", {
    HLE_FN(_cellSpursCreateJobQueue),
    HLE_FN(_cellSpursCreateJobQueueWithJobDescriptorPool),
    HLE_FN(_cellSpursJobQueuePort2CopyPushJobBody),
    HLE_FN(_cellSpursJobQueuePort2PushJobListBody),
    HLE_FN(cellSpursJobQueueAttributeInitialize),
    HLE_FN(cellSpursJobQueueAttributeSetSubmitWithEntryLock),
    HLE_FN(cellSpursJobQueuePort2Create),
    HLE_FN(cellSpursJobQueuePort2Destroy),
    HLE_FN(cellSpursJobQueuePort2Sync),
    HLE_FN(cellSpursJobQueuePortFinalize),
    HLE_FN(cellSpursJobQueuePortInitializeWithDescriptorBuffer),
    HLE_FN(cellSpursJoinJobQueue),
    HLE_FN(cellSpursShutdownJobQueue),
});

}  // namespace rt
