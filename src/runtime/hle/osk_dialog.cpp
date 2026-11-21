// HLE (cellSysutil): cellOskDialog — the system's on-screen keyboard. The text entry is a system
// dialog (sysdialog.h) shown while the game keeps running; the system callback events follow the
// SDK sequence: LOADED, then FINISHED when the user confirms or backs out (or the game aborts),
// and UNLOADED once the game has collected the text with cellOskDialogUnloadAsync.
#include <thread>

#include "runtime/config.h"
#include "runtime/hle.h"
#include "runtime/lv2/lv2.h"
#include "runtime/spurs_jq_diag.h"
#include "runtime/sysdialog.h"
#include "runtime/sysutil.h"

namespace rt {
std::atomic<bool> g_osk_unloaded{false};
std::atomic<uint64_t> g_cross_after_osk_time_us{0};

bool is_cross_window_active() {
  uint64_t cross_t = g_cross_after_osk_time_us.load(std::memory_order_relaxed);
  if (cross_t == 0) return false;
  uint64_t now = lv2::system_time_us();
  return (now >= cross_t && (now - cross_t) <= 20000000ull); // 20s
}
}  // namespace rt

namespace rt::osk {
namespace {

constexpr s32 OSK_ERROR_IME_ALREADY_IN_USE = s32(0x8002b501), OSK_ERROR_PARAM = s32(0x8002b504);
constexpr s32 MSGDIALOG_ERROR_DIALOG_NOT_OPENED = s32(0x8002b302);
constexpr u64 EV_LOADED = 0x0502, EV_FINISHED = 0x0503, EV_UNLOADED = 0x0504;
// CellOskDialogInputFieldResult (PS3 SDK): 0: OK, 1: CANCELED, 2: ABORT, 3: NO_INPUT_TEXT
constexpr s32 RESULT_OK = 0, RESULT_CANCELED = 1, RESULT_ABORT = 2, RESULT_NO_INPUT_TEXT = 3;
constexpr u32 kStringSize = 512;  // CELL_OSKDIALOG_STRING_SIZE

struct InputFieldInfo { be32 message, init_text, limit_length; };
struct CallbackReturnParam {
  bes32 result;
  bes32 numCharsResultString;
  be32 pResultString;
  bes32 reserved;
};

std::mutex g_mu;
bool g_open = false, g_finished = false, g_aborted = false;
s32 g_result = RESULT_CANCELED;
std::u16string g_text;
std::thread g_worker;

std::u16string read_u16(u32 addr, u32 max) {
  std::u16string s;
  for (u32 i = 0; addr && i < max; ++i) {
    char16_t c = char16_t(ppu::r16(addr + 2 * i));
    if (!c) break;
    s.push_back(c);
  }
  return s;
}
std::string to_utf8(const std::u16string& s) {
  std::string o;
  for (char16_t c : s) {  // BMP only (the keyboard produces no surrogates here)
    if (c < 0x80) o += char(c);
    else if (c < 0x800) o += char(0xc0 | (c >> 6)), o += char(0x80 | (c & 0x3f));
    else o += char(0xe0 | (c >> 12)), o += char(0x80 | ((c >> 6) & 0x3f)), o += char(0x80 | (c & 0x3f));
  }
  return o;
}

void join_worker() {
  if (g_worker.joinable()) g_worker.join();
}

}  // namespace

// cellOskDialogLoadAsync(container, dialogParam, inputFieldInfo)
s32 cellOskDialogLoadAsync(ppu::Ctx& ctx, u32 container, u32 dialog_param, Ptr<InputFieldInfo> field) {
  if (!dialog_param || !field || !field->message || !field->init_text || field->limit_length > kStringSize) return OSK_ERROR_PARAM;
  std::lock_guard lk(g_mu);
  if (g_open) return OSK_ERROR_IME_ALREADY_IN_USE;
  join_worker();
  u32 limit = std::min<u32>(field->limit_length ? u32(field->limit_length) : kStringSize - 1, kStringSize - 1);
  std::u16string prompt = read_u16(field->message, kStringSize), initial = read_u16(field->init_text, kStringSize);
  if (initial.size() > limit) initial.resize(limit);
  g_open = true;
  g_finished = g_aborted = false;
  g_result = RESULT_CANCELED;
  g_text.clear();
  std::string prompt8 = to_utf8(prompt);
  {
    u32 lr = u32(ctx.lr);
    u32 sp = u32(ctx.r[1]);
    std::string frames = ac4::strprintf(" [0] 0x%08x (LR)", lr);
    u32 cur_sp = sp;
    for (int depth = 1; depth <= 4 && cur_sp >= 0x00010000u && cur_sp < 0xF0000000u && !(cur_sp & 7); ++depth) {
      u32 next_sp = u32(ppu::r64(cur_sp));
      if (!next_sp) next_sp = ppu::r32(cur_sp);
      if (next_sp <= cur_sp || next_sp >= 0xF0000000u) break;
      u32 ret_addr = u32(ppu::r64(next_sp + 16));
      if (!ret_addr) ret_addr = ppu::r32(next_sp + 16);
      frames += ac4::strprintf(" [%d] 0x%08x", depth, ret_addr);
      cur_sp = next_sp;
    }
    LOG_I("osk", "cellOskDialogLoadAsync PPU backchain:%s", frames.c_str());
  }
  LOG_I("osk", "cellOskDialogLoadAsync: '%s' (initial '%s', up to %u characters)", prompt8.c_str(), to_utf8(initial).c_str(), limit);
  sysutil::post_event(EV_LOADED, 0);
  g_worker = std::thread([prompt8, initial, limit] {
    std::u16string text;
    bool ok = false;
    if (cfg().auto_confirm && !cfg().auto_name.empty()) {
      for (char c : cfg().auto_name) text.push_back(char16_t(static_cast<unsigned char>(c)));
      ok = true;
    } else {
      ok = sysdlg::text_input(prompt8, initial, limit, &text);
    }
    std::lock_guard lk(g_mu);
    if (!g_aborted) {
      g_result = !ok ? RESULT_CANCELED : text.empty() ? RESULT_NO_INPUT_TEXT : RESULT_OK;
      g_text = ok ? text : std::u16string();
      g_finished = true;
      LOG_I("osk", "input %s: '%s'", g_result == RESULT_OK ? "entered" : g_result == RESULT_CANCELED ? "canceled" : "empty",
            to_utf8(g_text).c_str());
      sysutil::post_event(EV_FINISHED, 0);
    }
  });
  return CELL_OK;
}

// cellOskDialogUnloadAsync(outputInfo): the entered text into the game's buffer (capacity
// numCharsResultString characters), then the dialog is gone.
s32 cellOskDialogUnloadAsync(Ptr<CallbackReturnParam> out) {
  if (!out || out->numCharsResultString < 0) return OSK_ERROR_PARAM;
  {
    std::lock_guard lk(g_mu);
    if (!g_open) return MSGDIALOG_ERROR_DIALOG_NOT_OPENED;
    if (!g_finished) {  // unloaded while still open: closes it like an abort
      g_aborted = true;
      g_result = RESULT_ABORT;
      g_finished = true;
    }
  }
  sysdlg::abort();
  join_worker();
  std::lock_guard lk(g_mu);
  out->result = g_result;
  out->reserved = 0;
  if (out->pResultString && (g_result == RESULT_OK || g_result == RESULT_NO_INPUT_TEXT)) {
    u32 cap = u32(s32(out->numCharsResultString));
    u32 n = 0;
    for (; n < g_text.size() && n + 1 < cap && n < kStringSize - 1; ++n) ppu::w16(out->pResultString + 2 * n, u16(g_text[n]));
    if (cap) ppu::w16(out->pResultString + 2 * n, 0);
    out->numCharsResultString = s32(n);
  }
  g_open = false;
  rt::g_osk_unloaded.store(true, std::memory_order_release);
  LOG_I("osk", "cellOskDialogUnloadAsync: result %d, numChars %d (CROSS trigger armed for HLE diagnostics)", g_result, s32(out->numCharsResultString));
  sysutil::post_event(EV_UNLOADED, 0);
  return CELL_OK;
}

s32 cellOskDialogAbort() {
  {
    std::lock_guard lk(g_mu);
    if (!g_open) return MSGDIALOG_ERROR_DIALOG_NOT_OPENED;
    if (g_finished) return CELL_OK;
    g_aborted = true;
    g_result = RESULT_ABORT;
    g_finished = true;
  }
  sysdlg::abort();
  LOG_I("osk", "cellOskDialogAbort");
  sysutil::post_event(EV_FINISHED, 0);
  return CELL_OK;
}

s32 cellOskDialogAddSupportLanguage(u32 /*dialog_param*/, u32 /*language*/) {
  return CELL_OK;
}

}  // namespace rt::osk

namespace rt {
static hle::Module s_module_osk("cellSysutil", {
    HLE_FN(osk::cellOskDialogAbort),
    HLE_FN(osk::cellOskDialogAddSupportLanguage),
    HLE_FN(osk::cellOskDialogLoadAsync),
    HLE_FN(osk::cellOskDialogUnloadAsync),
});
}  // namespace rt
