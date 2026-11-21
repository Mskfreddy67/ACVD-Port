#include "runtime/bindings.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cctype>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <sstream>

#include "common/ini.h"
#include "common/settings_file.h"
#include "runtime/input.h"
#include "runtime/log.h"

namespace rt::bindings {
namespace {

enum KeySlot : u32 {
  kSlotMoveForward = 0, kSlotMoveBack, kSlotStrafeRight, kSlotStrafeLeft, kSlotTurnRight, kSlotTurnLeft, kSlotLookUp, kSlotLookDown,
  kSlotResetView, kSlotLock, kSlotBoost, kSlotOverBoost, kSlotQuickBoost, kSlotRightWeapon, kSlotLeftWeapon, kSlotShoulder,
  kSlotSwitchRight, kSlotSwitchLeft, kSlotPause, kSlotCount
};

// cellPad digital words.
constexpr u16 SELECT = 0x1, L3 = 0x2, R3 = 0x4, START = 0x8, UP = 0x10, RIGHT = 0x20, DOWN = 0x40, LEFT = 0x80;
constexpr u16 L2 = 0x1, R2 = 0x2, L1 = 0x4, R1 = 0x8, TRIANGLE = 0x10, CIRCLE = 0x20, CROSS = 0x40, SQUARE = 0x80;

enum class Kind { D1, D2, Axis, Macro };
// The key-only controls (macros): the controller's button combinations, the purges ([switch right
// weapon] + [switch left weapon] + the weapon's use button; the game's KEY CONFIGURATION).
enum Macro { kPurgeRight, kPurgeLeft, kPurgeShoulder, kMacroCount };
struct Control {
  const char* name;
  Kind kind;
  u16 bit;
  int axis;       // 0 lx, 1 ly, 2 rx, 3 ry
  bool positive;
  int slot;       // the action it stands for in missions
};
const Control kControls[] = {
    {"lstick_up", Kind::Axis, 0, 1, false, kSlotMoveForward},     {"lstick_down", Kind::Axis, 0, 1, true, kSlotMoveBack},
    {"lstick_left", Kind::Axis, 0, 0, false, kSlotStrafeLeft},    {"lstick_right", Kind::Axis, 0, 0, true, kSlotStrafeRight},
    {"rstick_up", Kind::Axis, 0, 3, false, kSlotLookUp},          {"rstick_down", Kind::Axis, 0, 3, true, kSlotLookDown},
    {"rstick_left", Kind::Axis, 0, 2, false, kSlotTurnLeft},      {"rstick_right", Kind::Axis, 0, 2, true, kSlotTurnRight},
    {"cross", Kind::D2, CROSS, 0, false, kSlotRightWeapon},       {"square", Kind::D2, SQUARE, 0, false, kSlotLeftWeapon},
    {"r1", Kind::D2, R1, 0, false, kSlotSwitchRight},             {"circle", Kind::D2, CIRCLE, 0, false, kSlotShoulder},
    {"triangle", Kind::D2, TRIANGLE, 0, false, kSlotOverBoost},   {"l2", Kind::D2, L2, 0, false, kSlotBoost},
    {"r2", Kind::D2, R2, 0, false, kSlotQuickBoost},              {"l1", Kind::D2, L1, 0, false, kSlotSwitchLeft},
    {"r3", Kind::D1, R3, 0, false, kSlotResetView},               {"l3", Kind::D1, L3, 0, false, kSlotLock},
    {"start", Kind::D1, START, 0, false, kSlotPause},             {"select", Kind::D1, SELECT, 0, false, -1},
    {"up", Kind::D1, UP, 0, false, -1},             {"down", Kind::D1, DOWN, 0, false, -1},
    {"left", Kind::D1, LEFT, 0, false, -1},         {"right", Kind::D1, RIGHT, 0, false, -1},
    {"purge_right_weapon", Kind::Macro, 0, 0, false, kPurgeRight},
    {"purge_left_weapon", Kind::Macro, 0, 0, false, kPurgeLeft},
    {"purge_shoulder_weapon", Kind::Macro, 0, 0, false, kPurgeShoulder},
};
constexpr size_t kControlCount = sizeof(kControls) / sizeof(kControls[0]);

// What an input code of the game's assignment (key_assign.h) presses on the pad.
struct PadElem {
  bool valid = false;
  Kind kind = Kind::D1;
  u16 bit = 0;
  int axis = 0;
  bool positive = false;
};

struct KeyName {
  const char* name;
  u32 code;
};
const KeyName kKeys[] = {
    {"Space", VK_SPACE},        {"Enter", VK_RETURN},       {"Escape", VK_ESCAPE},      {"Tab", VK_TAB},
    {"Backspace", VK_BACK},     {"Shift", VK_SHIFT},        {"Ctrl", VK_CONTROL},       {"Alt", VK_MENU},
    {"Up", VK_UP},              {"Down", VK_DOWN},          {"Left", VK_LEFT},          {"Right", VK_RIGHT},
    {"Home", VK_HOME},          {"End", VK_END},            {"PageUp", VK_PRIOR},       {"PageDown", VK_NEXT},
    {"Insert", VK_INSERT},      {"Delete", VK_DELETE},      {"CapsLock", VK_CAPITAL},   {"Semicolon", VK_OEM_1},
    {"Equals", VK_OEM_PLUS},    {"Comma", VK_OEM_COMMA},    {"Minus", VK_OEM_MINUS},    {"Period", VK_OEM_PERIOD},
    {"Slash", VK_OEM_2},        {"Backquote", VK_OEM_3},    {"LBracket", VK_OEM_4},     {"Backslash", VK_OEM_5},
    {"RBracket", VK_OEM_6},     {"Quote", VK_OEM_7},        {"NumAdd", VK_ADD},         {"NumSubtract", VK_SUBTRACT},
    {"NumMultiply", VK_MULTIPLY}, {"NumDivide", VK_DIVIDE}, {"NumDecimal", VK_DECIMAL}, {"Mouse1", kMouse + 0},
    {"Mouse2", kMouse + 1},     {"Mouse3", kMouse + 2},     {"Mouse4", kMouse + 3},     {"Mouse5", kMouse + 4},
    {"WheelUp", kWheelUp},      {"WheelDown", kWheelDown},
};

std::mutex g_mu;  // bindings change on the window thread (PC SETTINGS) while pads are read
std::vector<u32> g_bind[kControlCount];
std::vector<u32> g_menu[kControlCount];  // [menu]: in the game's menus first
std::atomic<double> g_sensitivity_x{1.0}, g_sensitivity_y{1.0};
std::atomic<bool> g_invert_x{false}, g_invert_y{false};
std::atomic<MouseMode> g_mouse_mode{MouseMode::Stick};
std::chrono::steady_clock::time_point g_wheel_up_until, g_wheel_down_until;

std::string trim(std::string s) {
  while (!s.empty() && std::isspace(u8(s.back()))) s.pop_back();
  size_t b = 0;
  while (b < s.size() && std::isspace(u8(s[b]))) ++b;
  return s.substr(b);
}

bool equal_ci(const std::string& a, const char* b) {
  size_t n = std::strlen(b);
  if (a.size() != n) return false;
  for (size_t i = 0; i < n; ++i)
    if (std::tolower(u8(a[i])) != std::tolower(u8(b[i]))) return false;
  return true;
}

std::vector<u32> parse_list(const std::string& value, const char* control) {
  std::vector<u32> out;
  std::istringstream items(value);
  for (std::string item; std::getline(items, item, ',');) {
    item = trim(item);
    if (item.empty()) continue;
    u32 code = key_code(item);
    if (!code) LOG_W("input", "settings [keyboard] %s: unknown input '%s'", control, item.c_str());
    else if (!bindable(code)) LOG_W("input", "settings [keyboard] %s: '%s' is reserved by the window", control, item.c_str());
    else if (std::find(out.begin(), out.end(), code) == out.end()) out.push_back(code);
  }
  return out;
}

void parse_section(const ac4::Ini& ini, const char* section, std::vector<u32>* bind) {
  std::lock_guard lk(g_mu);
  for (size_t c = 0; c < kControlCount; ++c) bind[c].clear();
  for (const auto& [name, value] : ini.entries(section)) {
    size_t c = 0;
    while (c < kControlCount && !equal_ci(name, kControls[c].name)) ++c;
    if (c == kControlCount) {
      LOG_W("input", "settings [%s]: unknown pad control '%s'", section, name.c_str());
      continue;
    }
    bind[c] = parse_list(value, kControls[c].name);
  }
}
void parse_keyboard(const ac4::Ini& ini) {
  parse_section(ini, "keyboard", g_bind);
  parse_section(ini, "menu", g_menu);
}

}  // namespace

size_t control_count() { return kControlCount; }
const char* control_name(size_t c) { return c < kControlCount ? kControls[c].name : ""; }

u32 key_code(const std::string& n) {
  for (const KeyName& k : kKeys)
    if (equal_ci(n, k.name)) return k.code;
  if (n.size() == 1 && std::isalnum(u8(n[0]))) return u32(std::toupper(u8(n[0])));  // 'A'-'Z', '0'-'9'
  if (n.size() >= 2 && (n[0] == 'F' || n[0] == 'f') && std::isdigit(u8(n[1]))) {
    int f = std::atoi(n.c_str() + 1);
    if (f >= 1 && f <= 24) return u32(VK_F1 + f - 1);
  }
  if (n.size() == 4 && equal_ci(n.substr(0, 3), "Num") && std::isdigit(u8(n[3]))) return u32(VK_NUMPAD0 + (n[3] - '0'));
  return 0;
}

std::string input_name(u32 code) {
  for (const KeyName& k : kKeys)
    if (k.code == code) return k.name;
  if ((code >= 'A' && code <= 'Z') || (code >= '0' && code <= '9')) return std::string(1, char(code));
  if (code >= VK_F1 && code <= VK_F24) return "F" + std::to_string(code - VK_F1 + 1);
  if (code >= VK_NUMPAD0 && code <= VK_NUMPAD9) return "Num" + std::to_string(code - VK_NUMPAD0);
  return "";
}

bool bindable(u32 code) { return code != VK_F1 && code != VK_F11 && !input_name(code).empty(); }

void load(const ac4::Ini& ini) {
  parse_keyboard(ini);
  MouseSettings m;
  auto sens = [&](const char* key, double* out) {
    if (auto v = ini.get("mouse", key)) {
      double s = std::atof(v->c_str());
      if (s > 0 && s <= 100) *out = s;
      else LOG_W("input", "settings [mouse] %s '%s' ignored (use a number above 0)", key, v->c_str());
    }
  };
  sens("sensitivity", &m.sensitivity_x);  // the earlier single setting: both axes
  m.sensitivity_y = m.sensitivity_x;
  sens("sensitivity_x", &m.sensitivity_x);
  sens("sensitivity_y", &m.sensitivity_y);
  if (auto v = ini.get("mouse", "invert_x")) m.invert_x = equal_ci(*v, "true") || *v == "1";
  if (auto v = ini.get("mouse", "invert_y")) m.invert_y = equal_ci(*v, "true") || *v == "1";
  if (auto v = ini.get("mouse", "mode")) {
    if (equal_ci(trim(*v), "camera")) m.mode = MouseMode::Camera;
    else if (!equal_ci(trim(*v), "stick")) LOG_W("input", "settings [mouse] mode '%s' ignored (use stick or camera)", v->c_str());
  }
  set_mouse(m);
  size_t bound = 0;
  std::string bound_list, unbound_list;
  {
    std::lock_guard lk(g_mu);
    for (size_t c = 0; c < kControlCount; ++c) {
      if (!g_bind[c].empty()) {
        ++bound;
        std::string keys;
        for (u32 code : g_bind[c]) keys += (keys.empty() ? "" : "/") + input_name(code);
        bound_list += ac4::strprintf(" %s=[%s]", kControls[c].name, keys.c_str());
      } else {
        unbound_list += ac4::strprintf(" %s", kControls[c].name);
      }
    }
  }
  LOG_I("input", "keyboard/mouse: %zu of %zu controls bound; mouse as the %s, sensitivity %.3f x, %.3f y%s%s", bound, kControlCount,
        m.mode == MouseMode::Camera ? "camera" : "right stick", m.sensitivity_x, m.sensitivity_y, m.invert_x ? ", X inverted" : "",
        m.invert_y ? ", Y inverted" : "");
  LOG_I("input", "key map: %zu bound controls:%s", bound, bound_list.c_str());
  LOG_I("input", "key map: %zu unbound controls:%s", kControlCount - bound, unbound_list.c_str());
}

const char* mouse_mode_name(MouseMode m) { return m == MouseMode::Camera ? "camera" : "stick"; }

void store(ac4::Ini& ini) {
  std::lock_guard lk(g_mu);
  for (size_t c = 0; c < kControlCount; ++c) {
    std::string v;
    for (u32 code : g_bind[c]) v += (v.empty() ? "" : ", ") + input_name(code);
    ini.set("keyboard", kControls[c].name, v);
    std::string m;
    for (u32 code : g_menu[c]) m += (m.empty() ? "" : ", ") + input_name(code);
    if (!m.empty() || ini.get("menu", kControls[c].name)) ini.set("menu", kControls[c].name, m);
  }
  ini.remove("mouse", "sensitivity");  // the earlier single setting
  ini.set("mouse", "sensitivity_x", ac4::strprintf("%.2f", g_sensitivity_x.load()));
  ini.set("mouse", "sensitivity_y", ac4::strprintf("%.2f", g_sensitivity_y.load()));
  ini.set("mouse", "invert_x", g_invert_x.load() ? "true" : "false");
  ini.set("mouse", "invert_y", g_invert_y.load() ? "true" : "false");
  ini.set("mouse", "mode", mouse_mode_name(g_mouse_mode.load()));
}

std::vector<u32> get(size_t c, Layer l) {
  std::lock_guard lk(g_mu);
  return c < kControlCount ? (l == Layer::Menu ? g_menu : g_bind)[c] : std::vector<u32>{};
}

void set(size_t c, const std::vector<u32>& codes, Layer l) {
  if (c >= kControlCount) return;
  std::lock_guard lk(g_mu);
  (l == Layer::Menu ? g_menu : g_bind)[c] = codes;
}

void reset_defaults(Layer l) {
  if (l == Layer::Menu)
    parse_section(ac4::Ini::parse(std::string("[menu]\n") + ac4::settings::default_menu_text()), "menu", g_menu);
  else
    parse_section(ac4::Ini::parse(std::string("[keyboard]\n") + ac4::settings::default_keyboard_text()), "keyboard", g_bind);
}

PadOut sample(bool deliver) {
  PadOut o;
  if (!input_has_focus()) return o;
  auto now = std::chrono::steady_clock::now();
  if (s32 w = input_mouse_wheel_take()) {  // one notch: a press of 0.1 s
    (w > 0 ? g_wheel_up_until : g_wheel_down_until) = now + std::chrono::milliseconds(100);
  }
  // Presses since the game's last data count as down once, however short (a tap between two reads).
  // The game reads port 0 once per frame (the player's pad object, 0x951f94: 300 reads in 5 s at
  // 60 fps) and acts on a press of one frame (run 20261029-144921: instant clicks 5 of 5).
  std::array<bool, 256> tap{};
  u32 mouse_tap = 0;
  if (deliver) input_taps_take(&tap, &mouse_tap);
  auto down = [&](u32 code) {
    if (code < kMouse) return input_key_down(code) || tap[code];
    if (code < kWheelUp) return input_mouse_button(int(code - kMouse)) || ((mouse_tap >> (code - kMouse)) & 1) != 0;
    return now < (code == kWheelUp ? g_wheel_up_until : g_wheel_down_until);
  };
  bool neg[4] = {}, pos[4] = {};
  std::lock_guard lk(g_mu);
  auto apply = [&](const PadElem& e) {
    if (!e.valid) return;
    if (e.kind == Kind::D1) o.d1 |= e.bit;
    else if (e.kind == Kind::D2) o.d2 |= e.bit;
    else (e.positive ? pos : neg)[e.axis] = true;
  };

  for (size_t c = 0; c < kControlCount; ++c) {
    const Control& k = kControls[c];
    if (k.kind == Kind::Macro) continue;
    bool on = false;
    // Menu shortcuts take precedence globally. Otherwise a key such as Enter, bound to menu
    // CROSS and keyboard SELECT, emits both pad bits in menus (and the game can reject the chord).
    auto menu_key = [&](u32 code) {
      for (const auto& menu_control : g_menu)
        if (std::find(menu_control.begin(), menu_control.end(), code) != menu_control.end()) return true;
      return false;
    };
    for (u32 code : g_bind[c]) {
      if (!menu_key(code) && down(code)) { on = true; break; }
    }
    if (!on) {
      for (u32 code : g_menu[c]) {
        if (down(code)) { on = true; break; }
      }
    }
    if (!on) continue;
    apply(PadElem{true, k.kind, k.bit, k.axis, k.positive});
  }

  int* axes[4] = {&o.lx, &o.ly, &o.rx, &o.ry};
  for (int a = 0; a < 4; ++a)
    if (neg[a] || pos[a]) *axes[a] = neg[a] && pos[a] ? 128 : neg[a] ? 0 : 255;
  return o;
}

std::string mission_button(size_t c) {
  if (c >= kControlCount || kControls[c].slot < 0) return "";
  const Control& k = kControls[c];
  if (k.kind == Kind::D1) {
    if (k.bit == START) return "START";
    if (k.bit == SELECT) return "SELECT";
    if (k.bit == L3) return "L3";
    if (k.bit == R3) return "R3";
    if (k.bit == UP) return "D-PAD UP";
    if (k.bit == DOWN) return "D-PAD DOWN";
    if (k.bit == LEFT) return "D-PAD LEFT";
    if (k.bit == RIGHT) return "D-PAD RIGHT";
  } else if (k.kind == Kind::D2) {
    if (k.bit == CROSS) return "CROSS";
    if (k.bit == CIRCLE) return "CIRCLE";
    if (k.bit == SQUARE) return "SQUARE";
    if (k.bit == TRIANGLE) return "TRIANGLE";
    if (k.bit == L1) return "L1";
    if (k.bit == R1) return "R1";
    if (k.bit == L2) return "L2";
    if (k.bit == R2) return "R2";
  }
  return "";
}

MouseSettings mouse() {
  return {g_sensitivity_x.load(std::memory_order_relaxed), g_sensitivity_y.load(std::memory_order_relaxed),
          g_invert_x.load(std::memory_order_relaxed), g_invert_y.load(std::memory_order_relaxed),
          g_mouse_mode.load(std::memory_order_relaxed)};
}

void set_mouse(const MouseSettings& m) {
  g_sensitivity_x = m.sensitivity_x;
  g_sensitivity_y = m.sensitivity_y;
  g_invert_x = m.invert_x;
  g_invert_y = m.invert_y;
  g_mouse_mode = m.mode;
}

}  // namespace rt::bindings
