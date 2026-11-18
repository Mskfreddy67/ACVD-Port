import os
import re

hle_dir = r"D:\ACVD-Port\src\runtime\hle"
implemented = {} # (module, fnid_hex_or_name)

# Parse all C++ files in hle
for fname in os.listdir(hle_dir):
    if not fname.endswith(".cpp"): continue
    path = os.path.join(hle_dir, fname)
    with open(path, "r", encoding="utf-8", errors="ignore") as f:
        text = f.read()
    
    # find s_module("module_name", { ... })
    # There could be multiple or single module
    mod_matches = re.findall(r's_module\(\s*"([^"]+)"\s*,\s*\{([^}]+)\}', text, re.DOTALL)
    for mod_name, body in mod_matches:
        if mod_name not in implemented:
            implemented[mod_name] = set()
        # Find HLE_FN(foo), HLE_RAW(foo), HLE_TODO(foo), Func{"foo", ..., fnid}
        fns = re.findall(r'HLE_(?:FN|RAW|TODO)\(\s*([^,\s\)]+)\s*\)', body)
        for fn in fns:
            # could have namespace
            fn_clean = fn.split("::")[-1]
            implemented[mod_name].add(fn_clean)
        # explicit fnids
        explicit_fnids = re.findall(r'0x[0-9a-fA-F]+', body)
        for ef in explicit_fnids:
            implemented[mod_name].add(ef.lower())

print("Parsed HLE modules:", len(implemented))

# Also calculate fnid for known names
import hashlib
def fnid_of(name):
    suffix = bytes([0x67, 0x59, 0x65, 0x99, 0x04, 0x25, 0x04, 0x90,
                    0x56, 0x64, 0x27, 0x49, 0x94, 0x89, 0x74, 0x1A])
    h = hashlib.sha1(name.encode('utf-8') + suffix).digest()
    return f"0x{(h[0] | (h[1]<<8) | (h[2]<<16) | (h[3]<<24)):08x}"

# Map implemented function names to fnids
implemented_fnids = {}
for mod, fns in implemented.items():
    implemented_fnids[mod] = set()
    for fn in fns:
        if fn.startswith("0x"):
            implemented_fnids[mod].add(fn.lower())
        else:
            fid = fnid_of(fn).lower()
            implemented_fnids[mod].add(fid)

# Read ACVD imports
with open(r"D:\ACVD-Port\acvd_imports_utf8.txt", "r") as f:
    acvd_imports = [line.strip().split(":") for line in f if line.strip()]

missing_hle = []
found_hle = []

for mod, fnid, slot in acvd_imports:
    fnid = fnid.lower()
    if mod in implemented_fnids and fnid in implemented_fnids[mod]:
        found_hle.append((mod, fnid, slot))
    else:
        missing_hle.append((mod, fnid, slot))

diag_dir = r"D:\ACVD-Port\acvd-recomp\diag-out"
os.makedirs(diag_dir, exist_ok=True)

with open(os.path.join(diag_dir, "missing_hle.txt"), "w") as f:
    f.write(f"# Missing HLE imports: {len(missing_hle)} out of {len(acvd_imports)} total imports\n")
    missing_by_mod = {}
    for mod, fnid, slot in missing_hle:
        missing_by_mod.setdefault(mod, []).append((fnid, slot))
    for mod in sorted(missing_by_mod.keys()):
        f.write(f"[{mod}] ({len(missing_by_mod[mod])} missing)\n")
        for fnid, slot in missing_by_mod[mod]:
            f.write(f"  {fnid} (slot {slot})\n")

print(f"Total ACVD imports: {len(acvd_imports)}")
print(f"Found in runtime HLE: {len(found_hle)}")
print(f"Missing in runtime HLE: {len(missing_hle)}")

