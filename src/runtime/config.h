// Runtime configuration (command line and the shared settings file). Paths are host paths.
#pragma once
#include <atomic>
#include <cstdlib>
#include <string>

namespace rt {

struct Config {
  std::string data_dir;     // the installation's data folder (prepared data, saves, caches, settings, runs)
  std::string settings_file;  // shared settings (common/settings_file.h): <data>\config\settings.ini
  unsigned keep_runs = 0;   // run directories kept under <data>\runs (0: all)
  std::string disc_root;    // read-only PS3 disc folder (contains PS3_GAME)
  std::string title_id;     // from the disc's PARAM.SFO (TITLE_ID), e.g. BLUS30027
  std::string title;        // from the disc's PARAM.SFO (TITLE)
  std::string prepared;     // ac4-prep output (eboot.elf, update/)
  std::string hdd0;         // writable /dev_hdd0 (game data, saves)
  std::string saves;        // the player's save folder (--user DIR: DIR\savedata), else hdd0's own
  std::string hdd1;         // writable /dev_hdd1 (system cache)
  std::string flash;        // read-only /dev_flash (extracted firmware)
  std::string regulation;   // --regulation FILE: the launcher's Game update row (a regulation.bin; vfs.h)
  std::string run_dir;      // this run's log/capture directory
  bool warm_shaders = false;        // compile the shader manifest into the shader cache, then exit
  std::string priority;             // process priority class: "" (normal), "above-normal", "high"
  std::string pacing = "auto";      // emulated vblank source (gpu/display.h): auto | display | clock
  int render_scale = 0;             // percent; 0 = the settings file's (development override --render-scale)
  std::atomic<double> fps{60};      // game frame rate target (--fps N | display: 0 = the display's refresh | uncapped: -1); PC SETTINGS changes it
  bool fullscreen = false;          // start in borderless fullscreen (F11 / Alt+Enter toggle)
  bool windowed = false;            // --windowed: a window whatever the settings say
  bool auto_confirm = false;        // --auto-confirm: auto-inject Cross for system dialogs
  std::string auto_name;            // --auto-name NAME: player name for OSK dialog under --auto-confirm
  bool diag_shots = false;          // --diag-shots: save diagnostic screenshots (local/diag_step1_*.png, acvd-recomp/diag-out/*.png)
  int window_w = 1280, window_h = 720;  // window client size (windowed)
  std::string spu_cache;            // where collected SPU code records are written
  std::string shader_cache;         // compiled HLSL (derived from the game's shaders; machine-local)
  // --- Diagnostic / developer flags ---
  // These are runtime-configurable (CLI) so survey runs and porting work can be done without
  // a special developer build. They default to false/0, matching the previous player-build behaviour.
  // NOTE: When AC4_DEV=0 is defined (player release), the compiler may still eliminate dead
  // branches; the flags stay correct but tracing output is omitted in some paths.
  bool lenient = false;        // --lenient: unimplemented HLE calls return 0 instead of asserting
  bool trace_hle = false;      // --trace-hle: log every HLE call with args/return
  bool trace_lle = false;      // (internal; not exposed via CLI yet)
  unsigned trace_rsx = 0;      // --trace-rsx N: RSX method tracing verbosity level
  bool dump_shaders = false;   // --dump-shaders: write translated VP/FP HLSL to run_dir
  bool shader_collect = false; // --shader-collect: record shader programs for the manifest
  bool spu_collect = false;    // --spu-collect: write SPU code traces to spu_cache
  bool spu_interp = false;     // --spu-interp: use the SPU interpreter instead of compiled code
  unsigned skip_fp = 0;        // --skip-fp N: skip fragment program N (debugging)
  bool verbose_diag = false;   // --verbose-diag: enable verbose trace channels (sync-sig, state-watch, etc.)
  unsigned dump_surface = 0;   // --dump-surface ADDR: dump D3D11 surface at address to PNG in run_dir

  double seconds = 0;          // --seconds N: auto-exit after N seconds
  bool headless = false;       // --headless: run without window/presentation
  std::string shot_path;       // --shot PATH: save screenshot to PNG before exit
  static constexpr double shot_interval = 0, profile_at = 0, profile_split = 0, trace_rsx_at = 0, frame_report = -1;
  static constexpr unsigned trace_spu_limit = 0, trace_spu_cats = 0;
  static constexpr long shot_surface = -1;
  static constexpr bool isolate_input = false, no_activate = false, hidden_window = false, gpu_submit_thread = true, audio_capture = false,
                        gcm_wait_spin = false, warp = false;
  static constexpr bool verify_native = false, texture_bc = true;
  static constexpr int gpu_timing = 0;
  static constexpr unsigned long long affinity = 0;
  static constexpr double present_delay_ms = 0;
  static inline const std::string shader_records{}, test_overlay{};
};

Config& cfg();

}  // namespace rt
