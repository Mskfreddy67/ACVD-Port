// acvd: runtime host for the recompiled Armored Core: Verdict Day (BLUS31194) executable.
//   ac4 [--data <dir>] [--user <dir>] [--disc <dir>] [--flash <dir>] [--seconds N] [--fps N|display|uncapped] ...; options below
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <psapi.h>
#include <shlobj.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <thread>

#include "ac4_version.h"  // AC4_VERSION (generated from VERSION)
#include "common/file.h"
#include "common/sfo.h"
#include "runtime/bindings.h"
#include "runtime/config.h"
#include "runtime/gpu/d3d11_device.h"
#include "runtime/gpu/display.h"
#include "runtime/gpu/png.h"
#include "runtime/gpu/shader_warm.h"
#include "runtime/gpu/window.h"
#include "runtime/rsx/rsx.h"
#include "runtime/settings.h"
#include "runtime/sysutil.h"
#include "runtime/hle.h"
#include "runtime/input.h"
#include "runtime/loader.h"
#include "runtime/lv2/lv2.h"
#include "runtime/lv2/memory.h"
#include "runtime/log.h"
#include "runtime/memory.h"
#include "runtime/ppu_runtime.h"
#include "runtime/ppu_thread.h"
#include "runtime/profiler.h"
#include "runtime/spurs_jq_diag.h"
#include "runtime/reservation.h"

namespace rt {
Config& cfg() {
  static Config c;
  return c;
}
void vdec_dump();
std::string audio_report();  // cellAudio.cpp: output statistics since the last call
void lv2_init();  // lv2 subsystems that need setup after memory/loader
namespace lv2 {
void spu_dump();
void spu_log_second(double now);
void sync_dump();
}
void lwmutex_dump();
namespace gpu {
extern std::atomic<int> g_capture_requests;
}
}  // namespace rt

using namespace rt;
namespace fs = std::filesystem;

namespace {

// Run directories are named after their UTC start time (YYYYMMDD-HHMMSS); with --keep-runs N only
// the newest N stay (the launcher's runs keep their logs without filling the disk).
void prune_runs(const fs::path& runs, unsigned keep) {
  std::error_code ec;
  std::vector<fs::path> dirs;
  for (const auto& e : fs::directory_iterator(runs, ec)) {
    std::string n = e.path().filename().string();
    if (e.is_directory(ec) && n.size() == 15 && n[8] == '-' &&
        std::all_of(n.begin(), n.end(), [](char ch) { return ch == '-' || (ch >= '0' && ch <= '9'); }))
      dirs.push_back(e.path());
  }
  if (dirs.size() <= keep) return;
  std::sort(dirs.begin(), dirs.end());
  for (size_t i = 0; i + keep < dirs.size(); ++i) {
    ac4::guard_write(dirs[i]);
    fs::remove_all(dirs[i], ec);
  }
}

}  // namespace

int main(int argc, char** argv) {
  Config& c = cfg();
  fs::path root = fs::current_path();
  // Where everything machine-local lives: --data DIR (the launcher's data folder), by default
  // local\ in the current directory (the development layout). --settings FILE replaces the
  // shared settings file for a run (tests).
  fs::path data = root / "local", user;
  std::string settings_override;
  for (int i = 1; i + 1 < argc; ++i) {
    std::string a = argv[i];
    if (a == "--data") data = fs::absolute(argv[i + 1]);
    else if (a == "--user") user = fs::absolute(argv[i + 1]);
    else if (a == "--settings") settings_override = argv[i + 1];
    else if (a == "--keep-runs") c.keep_runs = unsigned(std::strtoul(argv[i + 1], nullptr, 10));
  }
  c.data_dir = data.string();
  // The user's game folder: by default the disc folder inside the current directory; the first
  // line of <data>\config\disc_root.txt names another one; --disc overrides both.
  c.disc_root = (root / "Armored Core Verdict Day (USA)").string();
  if (std::ifstream f(data / "config" / "disc_root.txt"); f) {
    std::string line;
    std::getline(f, line);
    while (!line.empty() && (line.back() == '\r' || line.back() == ' ' || line.back() == '"')) line.pop_back();
    size_t b = line.find_first_not_of(" \"");
    if (b != std::string::npos && !line.empty()) c.disc_root = line.substr(b);
  }
  // prepared dir: use the title_id subdirectory when available; fall back to BLUS31194 for ACVD.
  c.prepared = (data / "game" / "BLUS31194").string();
  c.hdd0 = (data / "hdd0").string();
  c.hdd1 = (data / "hdd1").string();
  c.flash = (data / "game" / "dev_flash").string();  // the dev_flash file(s) imported from the player's RPCS3
  c.spu_cache = (data / "spu_cache").string();
  c.shader_cache = (data / "shader_cache").string();
  // The player's own folder (--user, from the launcher): settings.ini and savedata\ there. Without
  // it both stay in the data folder (development runs, test profiles).
  // ACVD-Port: default user directory for Saved Games
  if (user.empty()) {
    wchar_t* path = nullptr;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_SavedGames, 0, nullptr, &path))) {
      user = fs::path(path) / "ARMORED CORE VERDICT DAY";
      CoTaskMemFree(path);
    }
  }
  c.settings_file = !settings_override.empty() ? settings_override
                    : !user.empty()            ? (user / "settings.ini").string()
                                               : ac4::settings::file_in(data).string();
  if (!user.empty()) c.saves = (user / "savedata").string();
  std::string shader_manifest = (root / "data" / "shader_manifest.txt").string();
  // Run directory: <data>\runs\<UTC timestamp>
  std::time_t now = std::time(nullptr);
  std::tm tm{};
  gmtime_s(&tm, &now);
  char stamp[32];
  std::strftime(stamp, sizeof stamp, "%Y%m%d-%H%M%S", &tm);
  fs::path run = data / "runs" / stamp;
  try {
    ac4::guard_write(run);
    fs::create_directories(run);
    if (c.keep_runs) prune_runs(data / "runs", c.keep_runs);
    c.run_dir = run.string();
    log_open((run / "acvd.log").string());
  } catch (const std::exception& e) {
    std::fprintf(stderr, "cannot create the run directory %s: %s\n", run.string().c_str(), e.what());
    return 1;
  }
  // Shared settings (launcher and PC SETTINGS), before the command line overrides them.
  std::vector<std::string> settings_notes;
  settings::init(c.settings_file, &settings_notes);
  bool safe_mode = std::any_of(argv + 1, argv + argc, [](const char* a) { return std::string(a) == "--safe-mode"; });
  if (safe_mode) {
    settings::use_safe_display();
    settings_notes.push_back("safe mode: default display settings in a 1280x720 window for this run (the settings file is unchanged)");
  }
  {
    ac4::settings::Display d = settings::display();
    c.fps = d.fps;
    c.pacing = d.pacing;
    c.fullscreen = d.mode == ac4::settings::Mode::Borderless;
    c.window_w = d.width;
    c.window_h = d.height;
  }
  std::string level = "info";
  [[maybe_unused]] bool pad_selftest = false;  // --pad-selftest (developer builds)
  for (int i = 1; i < argc; ++i) {
    std::string a = argv[i];
    auto next = [&]() -> std::string { return i + 1 < argc ? argv[++i] : std::string(); };
    if (a == "--data" || a == "--user" || a == "--settings" || a == "--keep-runs") next();  // read above
    else if (a == "--safe-mode") continue;  // read above
    else if (a == "--disc") c.disc_root = next();
    else if (a == "--log-level") level = next();
    else if (a == "--warm-shaders") c.warm_shaders = true;
    else if (a == "--shader-manifest") shader_manifest = next();
    else if (a == "--pacing") c.pacing = next();
    else if (a == "--fps") {
      std::string v = next();
      c.fps = v == "display" ? 0 : v == "uncapped" ? ac4::settings::kFpsUncapped : std::max(30.0, std::atof(v.c_str()));
    }
    else if (a == "--auto-confirm") c.auto_confirm = true;
    else if (a == "--auto-name") c.auto_name = next();
    else if (a == "--diag-shots") c.diag_shots = true;
    else if (a == "--windowed") c.fullscreen = false, c.windowed = true;
    else if (a == "--fullscreen") c.fullscreen = true;
    else if (a == "--priority") c.priority = next();
    else if (a == "--flash") c.flash = next();  // the launcher's folder for the chosen system software
    else if (a == "--regulation") c.regulation = next();  // the launcher's Game update row (vfs.h)
    // ---- ACVD / diagnostic flags (Stage 1.1) ----
    else if (a == "--lenient") c.lenient = true;
    else if (a == "--trace-hle") c.trace_hle = true;
    else if (a == "--trace-rsx") {
      // Optional numeric level: --trace-rsx [N]. If the next token starts with '-' or is absent,
      // treat it as a flag (level = 1) so --trace-rsx --lenient doesn't swallow --lenient.
      if (i + 1 < argc && argv[i + 1][0] != '-') {
        ++i;
        c.trace_rsx = unsigned(std::strtoul(argv[i], nullptr, 10));
        if (!c.trace_rsx) c.trace_rsx = 1u;  // "0" or non-numeric → still enable
      } else {
        c.trace_rsx = 1u;
      }
    }
    else if (a == "--verbose-diag") c.verbose_diag = true;
    else if (a == "--dump-surface") {
      std::string v = next();
      c.dump_surface = unsigned(std::strtoul(v.c_str(), nullptr, 0));
    }
    else if (a == "--dump-shaders") c.dump_shaders = true;
    else if (a == "--shader-collect") c.shader_collect = true;
    else if (a == "--spu-collect") c.spu_collect = true;
    else if (a == "--spu-interp") c.spu_interp = true;
    else if (a == "--skip-fp") {
      std::string v = next();
      c.skip_fp = unsigned(std::strtoul(v.c_str(), nullptr, 10));
    }
    else if (a == "--seconds") {
      std::string v = next();
      c.seconds = std::atof(v.c_str());
    }
    else if (a == "--shot") {
      c.shot_path = next();
    }
    else if (a == "--headless") c.headless = true;
    else if (a == "--help" || a == "-h") {
      std::printf("Armored Core: Verdict Day PC port %s\n"
                  "Usage: acvd [options]\n\n"
                  "Options:\n"
                  "  --data <dir>         Local data directory (default: local)\n"
                  "  --disc <dir>         Game disc directory\n"
                  "  --user <dir>         Player save directory\n"
                  "  --windowed           Run in windowed mode\n"
                  "  --fullscreen         Run in fullscreen mode\n"
                  "  --fps <n>            Frame rate limit (e.g. 60, display, uncapped)\n"
                  "  --trace-hle          Log every HLE call with args/return: hle: name(args) -> ret [thread]\n"
                  "  --trace-rsx [level]  Trace RSX methods\n"
                  "  --spu-collect        Record SPU code traces to local/spu_cache\n"
                  "  --spu-interp         Run SPU via interpreter instead of compiled code\n"
                  "  --lenient            Unimplemented HLE calls return 0 instead of asserting\n"
                  "  --help, -h           Show this help message\n",
                  AC4_VERSION);
      return 0;
    }
    else {
      std::fprintf(stderr, "unknown argument %s (use --help for options)\n", a.c_str());
      return 2;
    }
  }
  ac4::add_protected_root(c.disc_root);
  log_set_level(level == "trace" ? Level::Trace : level == "debug" ? Level::Debug : Level::Info);
  LOG_I("main", "Armored Core: Verdict Day PC port %s", AC4_VERSION);
  LOG_I("main", "run dir %s", c.run_dir.c_str());
  if (c.affinity) {  // --affinity MASK (hex): fewer logical CPUs, to estimate smaller machines
    if (SetProcessAffinityMask(GetCurrentProcess(), DWORD_PTR(c.affinity)))
      LOG_I("main", "--affinity: limited to logical CPUs 0x%llx (%d of them)", c.affinity, __builtin_popcountll(c.affinity));
    else
      LOG_W("main", "--affinity 0x%llx not applied (error %lu)", c.affinity, GetLastError());
  }
  if (c.warm_shaders) {  // pipeline step: shader cache from the manifest, no game run
    gpu::WarmResult w = gpu::warm_shaders(shader_manifest, c.disc_root, c.prepared);
    if (!w.error.empty()) {
      LOG_E("warm", "%s", w.error.c_str());
      std::fprintf(stderr, "%s\n", w.error.c_str());
      return 1;
    }
    std::string s = ac4::strprintf("shader warm-up: %u programs, %u compiled, %u already cached, %u skipped, %u failed; %.1f s on %u threads",
                                    w.programs, w.compiled, w.cached, w.skipped, w.failed, w.seconds, w.threads);
    LOG_I("warm", "%s", s.c_str());
    std::printf("%s\n", s.c_str());
    return w.failed ? 1 : 0;
  }
  for (const auto& n : settings_notes) LOG_I("main", "%s", n.c_str());
  // Optional process priority: with other programs keeping every core busy, each handoff between
  // the game's threads otherwise waits for a time slice (the frame rate collapses); above normal,
  // the game's threads run when they become ready. Off by default: it takes CPU time from other
  // programs while the game runs.
  if (!c.priority.empty()) {
    DWORD cls = c.priority == "high" ? HIGH_PRIORITY_CLASS : c.priority == "above-normal" ? ABOVE_NORMAL_PRIORITY_CLASS : 0;
    if (cls && SetPriorityClass(GetCurrentProcess(), cls))
      LOG_I("main", "process priority: %s", c.priority.c_str());
    else
      LOG_W("main", "--priority %s not applied (use above-normal or high)", c.priority.c_str());
  }
  if (c.isolate_input) LOG_I("input", "--isolate-input: the real keyboard, mouse and pads are ignored (scripted input only)");
  if (c.lenient) LOG_W("main", "SURVEY MODE (--lenient): unimplemented calls return 0; results are not evidence of working behaviour");
  LOG_I("main", "disc %s, prepared %s, hdd0 %s, saves %s, settings %s", c.disc_root.c_str(), c.prepared.c_str(), c.hdd0.c_str(),
        c.saves.empty() ? "in hdd0" : c.saves.c_str(), c.settings_file.c_str());
  try {
    auto sfo = ac4::sfo::parse(ac4::read_file(fs::path(c.disc_root) / "PS3_GAME" / "PARAM.SFO"));
    c.title_id = ac4::sfo::get_string(sfo, "TITLE_ID");
    c.title = ac4::sfo::get_string(sfo, "TITLE");
    LOG_I("main", "title %s (%s), version %s", c.title.c_str(), c.title_id.c_str(), ac4::sfo::get_string(sfo, "VERSION").c_str());
    // ACVD-Port: resolve the prepared directory from the disc's title_id (BLUS31194/BLES01898)
    {
      fs::path prepared_by_id = fs::path(data) / "game" / c.title_id;
      if (fs::exists(prepared_by_id))
        c.prepared = prepared_by_id.string();
      else
        LOG_W("main", "prepared dir for %s not found at %s; using %s",
              c.title_id.c_str(), prepared_by_id.string().c_str(), c.prepared.c_str());
    }
  } catch (const std::exception& e) {
    LOG_E("main", "cannot read the disc's PARAM.SFO: %s", e.what());
    return 1;
  }

  if (!vm::init()) return 1;
  install_fault_handler();
  ppu_runtime_init();
  ExecInfo ex;
  try {
    // Firmware modules relocated by ac4-prep (LLE) load first so the game binds to them.
    fs::path lle = fs::path(c.prepared) / "lle";
    if (fs::exists(lle))
      for (const auto& e : fs::directory_iterator(lle))
        if (e.path().extension() == ".elf") load_lle_module(ac4::read_file(e.path()));
    bind_lle_imports();
    ex = load_exec(ac4::read_file(fs::path(c.prepared) / "eboot.elf"));
    lv2::g_sdk_version = ex.sdk_version;
    lv2::memory_init(ex.sdk_version, u64(ex.segs_size) + ex.primary_stack);
    fs::path self = fs::path(c.disc_root) / "PS3_GAME" / "USRDIR" / "EBOOT.BIN";
    if (lv2::read_self_identity(self.string()))
      LOG_I("main", "SELF auth id 0x%016llx, control flags 0x%08x", (unsigned long long)lv2::g_auth_id, lv2::g_ctrl_flags1);
    else
      LOG_W("main", "cannot read the SELF header of %s; program authority ID unknown", self.string().c_str());
  } catch (const std::exception& e) {
    LOG_E("main", "load failed: %s", e.what());
    return 1;
  }
  lv2_init();
  if (!c.headless) gpu::window_dpi_aware();
  if (!c.headless && !gpu::window_create("ARMORED CORE: VERDICT DAY", c.window_w, c.window_h, !c.hidden_window)) {
    LOG_E("main", "window creation failed; running without presentation");
  } else if (!c.headless) {
    ac4::settings::Display d = settings::display();
    if (!d.monitor.empty()) gpu::window_move_to_monitor(d.monitor);  // the chosen monitor, before any fullscreen mode
    if (c.fullscreen)
      gpu::window_set_fullscreen(true);
    else if (!c.windowed && d.mode == ac4::settings::Mode::Fullscreen)  // exclusive fullscreen from the first frame
      gpu::request_exclusive(true, u32(d.fs_width), u32(d.fs_height), u32(d.fs_hz));
  }
  // ACVD: Disable trophy installation check (standalone PC runtime without PS3 XMB trophy database)
  if (ppu::mem()) {
    ppu::mem()[0x01e3ae0b] = 1;
    LOG_I("main", "applied ACVD compatibility: Trophy.Disable = true");
  }
  start_main_thread(ex.entry_opd, ex.primary_stack, ex.primary_prio, ex.tls_vaddr, ex.tls_filesz, ex.tls_memsz,
                    ex.malloc_pagesize, "/dev_bdvd/PS3_GAME/USRDIR/EBOOT.BIN");
  auto t0 = std::chrono::steady_clock::now();
  double close_at = -1, next_shot = c.shot_interval > 0 ? c.shot_interval : -1, next_title = 1;
  static std::mutex stop_mu;  // the watchdog and the main loop may both decide to stop
  static bool s_shot_taken = false;
  auto stop = [&](const char* why) {
    stop_mu.lock();  // never released: the process ends here
    LOG_I("main", "%s; stopping", why);
    LOG_I("main", "%s", rsx::stats().c_str());
    if (!c.shot_path.empty() && !s_shot_taken) {
      std::vector<u8> rgba;
      u32 w = 0, h = 0;
      if (gpu::read_back_last_frame(rgba, w, h)) {
        if (gpu::write_png(c.shot_path, w, h, rgba)) {
          LOG_I("main", "saved screenshot (%ux%u) to %s", w, h, c.shot_path.c_str());
        }
      }
    }
    {
      u32 compiled = 0, cached = 0;
      double secs = 0;
      gpu::shader_stats(&compiled, &cached, &secs);
      LOG_I("main", "shaders: %u compiled (%.1f s), %u from the shader cache", compiled, secs, cached);
    }
    thread_dump();
    lwmutex_dump();
    lv2::sync_dump();
    lv2::spu_dump();
    vdec_dump();
    LOG_I("dump", "PPU conditional stores refused by a change elsewhere in the 128-byte granule: %llu",
          (unsigned long long)ppu::granule_conflicts());
    rsx::dump();
    dump_hle_stats();
    log_flush();
    ExitProcess(0);
  };
  // Watchdog: the time limit and log flushing keep working while the main thread is busy in a
  // modal system dialog (or stuck), so bounded runs stay bounded and keep their logs.
  std::thread([&] {
    u64 last_flips = 0;
    double last_fps_at = 0;
    double last_black_screen_dump_at = 0;
    for (;;) {
      std::this_thread::sleep_for(std::chrono::milliseconds(250));
      double now = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
      static double last_spu_sec_log = 0;
      if (now - last_spu_sec_log >= 1.0) {
        last_spu_sec_log = now;
        lv2::spu_log_second(now);
      }
      if (g_draw_dropped_to_one.load(std::memory_order_acquire)) {
        if (last_black_screen_dump_at == 0 || now - last_black_screen_dump_at >= 5.0) {
          last_black_screen_dump_at = now;
          LOG_I("watchdog", "=== WATCHDOG 5s BLACK SCREEN DUMP (t=%.2fs) ===", now);
          LOG_I("watchdog", "SPURS JQ PUSHES: BEFORE MARKER = %llu | AFTER MARKER = %llu (TOTAL = %llu)",
                (unsigned long long)g_jq_push_before_marker.load(),
                (unsigned long long)g_jq_push_after_marker.load(),
                (unsigned long long)g_jq_total_pushes.load());
          thread_dump();
          lwmutex_dump();
          lv2::sync_dump();
          lv2::spu_dump();
          LOG_I("watchdog", "=== END WATCHDOG DUMP ===");
        }
      }
      if (now - last_fps_at >= 5) {  // displayed frame rate, for performance tracking
        u64 f = rsx::flips();
        std::string busy = "-";
        PROCESS_MEMORY_COUNTERS pmc{};
        GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof pmc);
        static u64 last_faults = 0;
        // Machine-wide CPU use next to ours: load from other programs shows up in measurements.
        auto ft = [](const FILETIME& t) { return (u64(t.dwHighDateTime) << 32) | t.dwLowDateTime; };
        FILETIME si, sk, su, pc, pe, pk, pu;
        GetSystemTimes(&si, &sk, &su);
        GetProcessTimes(GetCurrentProcess(), &pc, &pe, &pk, &pu);
        static u64 last_idle = 0, last_total = 0, last_ours = 0;
        u64 idle = ft(si), total = ft(sk) + ft(su), ours = ft(pk) + ft(pu);  // kernel time includes idle
        double sys_busy = total > last_total ? 100.0 * (1.0 - double(idle - last_idle) / double(total - last_total)) : 0;
        double our_share = total > last_total ? 100.0 * double(ours - last_ours) / double(total - last_total) : 0;
        if (last_fps_at > 0)
          LOG_I("perf", "%.1f frames/s; CPU: machine %.0f%%, this process %.0f%%; %.0f page faults/s, working set %llu MiB; "
                "busiest threads: %s",
                double(f - last_flips) / (now - last_fps_at), sys_busy, our_share,
                double(pmc.PageFaultCount - last_faults) / (now - last_fps_at), (unsigned long long)(pmc.WorkingSetSize >> 20),
                busy.c_str());
        if (last_fps_at > 0) LOG_I("perf", "%s", audio_report().c_str());
        if (last_fps_at > 0 && !c.headless) LOG_I("perf", "%s", gpu::display::report().c_str());
        last_faults = pmc.PageFaultCount;
        last_idle = idle;
        last_total = total;
        last_ours = ours;
        last_flips = f;
        last_fps_at = now;
      }
      log_flush();
      double el = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
      if (c.seconds > 0 && el >= c.seconds + 2) stop("--seconds limit reached (watchdog)");
      if (input_quit_requested()) stop("quit requested in the control file");
    }
  }).detach();
  for (;;) {
    if (!c.headless) gpu::window_pump();
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    double el = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
    if (c.diag_shots) {
      static int s_next_diag_sec = 10;
      if (s_next_diag_sec <= 90 && el >= s_next_diag_sec) {
        std::string diag_path = ac4::strprintf("local/diag_step1_%ds.png", s_next_diag_sec);
        s_next_diag_sec += 5;
        std::vector<u8> rgba;
        u32 w = 0, h = 0;
        if (gpu::read_back_last_frame(rgba, w, h)) {
          if (gpu::write_png(diag_path, w, h, rgba)) {
            LOG_I("main", "saved diagnostic screenshot (%ux%u) to %s at %.1f s", w, h, diag_path.c_str(), el);
          }
        }
      }
    }
    if (!s_shot_taken && !c.shot_path.empty() && el >= 10.0) {
      s_shot_taken = true;
      std::vector<u8> rgba;
      u32 w = 0, h = 0;
      if (gpu::read_back_last_frame(rgba, w, h)) {
        if (gpu::write_png(c.shot_path, w, h, rgba)) {
          LOG_I("main", "saved screenshot (%ux%u) to %s at %.1f s", w, h, c.shot_path.c_str(), el);
        }
      }
    }
    if (gpu::window_close_requested() && close_at < 0) {
      LOG_I("main", "window closed: sending CELL_SYSUTIL_REQUEST_EXITGAME to the game");
      sysutil::post_event(0x0101, 0);
      close_at = el;
    }
    if (close_at >= 0 && el - close_at > 5.0) stop("game did not exit within 5 s of the exit request");
    (void)next_shot;
    (void)next_title;
  }
}
