#pragma once
#include <atomic>
#include <cstdint>

namespace rt {

extern std::atomic<bool> g_draw_dropped_to_one;
extern std::atomic<uint64_t> g_jq_push_before_marker;
extern std::atomic<uint64_t> g_jq_push_after_marker;
extern std::atomic<uint64_t> g_jq_total_pushes;
extern std::atomic<bool> g_save_diagnostic_pngs_requested;

extern std::atomic<bool> g_osk_unloaded;
extern std::atomic<uint64_t> g_cross_after_osk_time_us;

bool is_cross_window_active();

}  // namespace rt
