#!/usr/bin/env python3
"""
acvd-recomp/gen_spu_manifest.py
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Generate data/acvd_spu_manifest.txt from SPU code traces collected by
running the game with  --spu-interp --spu-collect.

Usage:
    python gen_spu_manifest.py --spu-cache <dir> --prepared <dir> --out <manifest.txt>

Arguments:
    --spu-cache   Directory that holds *.spurec files written by --spu-collect.
    --prepared    ac4-prep output directory for ACVD (holds eboot.elf, lle/*.elf).
    --out         Output manifest file (default: data/acvd_spu_manifest.txt).

The .spurec binary format (little-endian, host-side struct from the runtime):
    struct SpuRecHeader {
        uint8_t  magic[8];   // "SPUREC01"
        uint64_t id;         // content hash used as the record name
        uint32_t n_entries;  // number of local-store code entries
        uint32_t n_ranges;   // number of source ranges
    };
    struct SpuEntry  { uint32_t ls_addr; };          // entry point in LS
    struct SpuRange  {
        uint32_t ls_start, ls_end;                   // LS byte range [start, end)
        uint32_t source_idx;                         // index into the source list
        uint32_t file_offset;                        // byte offset within that source file
    };

The manifest format mirrors data/spu_manifest.txt exactly so that
ac4-prep spu-records can consume it unmodified.
"""

import argparse
import hashlib
import os
import struct
import sys
from pathlib import Path

MAGIC = b"SPUREC01"
HDR_FMT = "<8sQII"
HDR_SIZE = struct.calcsize(HDR_FMT)
ENTRY_FMT = "<I"
ENTRY_SIZE = struct.calcsize(ENTRY_FMT)
RANGE_FMT = "<IIII"
RANGE_SIZE = struct.calcsize(RANGE_FMT)


def sha256_file(path: Path) -> str:
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def parse_spurec(data: bytes):
    """Return (record_id: int, entries: list[int], ranges: list[tuple])."""
    if len(data) < HDR_SIZE:
        raise ValueError("file too short for header")
    magic, rec_id, n_entries, n_ranges = struct.unpack_from(HDR_FMT, data, 0)
    if magic != MAGIC:
        raise ValueError(f"bad magic {magic!r}")
    off = HDR_SIZE
    entries = []
    for _ in range(n_entries):
        (addr,) = struct.unpack_from(ENTRY_FMT, data, off)
        entries.append(addr)
        off += ENTRY_SIZE
    ranges = []
    for _ in range(n_ranges):
        ls_start, ls_end, src_idx, file_off = struct.unpack_from(RANGE_FMT, data, off)
        ranges.append((ls_start, ls_end, src_idx, file_off))
        off += RANGE_SIZE
    return rec_id, entries, ranges


def collect_sources(prepared: Path) -> list[tuple[str, str]]:
    """Return ordered [(rel_path, sha256)] for all source files."""
    sources = []
    eboot = prepared / "eboot.elf"
    if not eboot.exists():
        raise FileNotFoundError(f"eboot.elf not found in {prepared}")
    sources.append(("eboot.elf", sha256_file(eboot)))
    lle_dir = prepared / "lle"
    if lle_dir.exists():
        for elf in sorted(lle_dir.glob("*.elf")):
            sources.append((f"lle/{elf.name}", sha256_file(elf)))
    return sources


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--spu-cache", required=True, type=Path, metavar="DIR",
                    help="directory containing *.spurec files")
    ap.add_argument("--prepared", required=True, type=Path, metavar="DIR",
                    help="ac4-prep output directory (eboot.elf, lle/)")
    ap.add_argument("--out", type=Path, default=Path("data/acvd_spu_manifest.txt"), metavar="FILE",
                    help="output manifest path (default: data/acvd_spu_manifest.txt)")
    args = ap.parse_args()

    spu_cache: Path = args.spu_cache
    prepared: Path  = args.prepared
    out_path: Path  = args.out

    if not spu_cache.is_dir():
        sys.exit(f"error: --spu-cache {spu_cache} is not a directory")
    if not prepared.is_dir():
        sys.exit(f"error: --prepared {prepared} is not a directory")

    # Enumerate source files and build the sha-indexed lookup.
    print(f"collecting sources from {prepared} ...", file=sys.stderr)
    sources = collect_sources(prepared)  # [(rel_path, sha256)]
    if not sources:
        sys.exit("error: no source files found in prepared directory")

    # Parse every .spurec file.
    spurec_files = sorted(spu_cache.glob("*.spurec"))
    if not spurec_files:
        sys.exit(f"error: no *.spurec files found in {spu_cache}")
    print(f"found {len(spurec_files)} .spurec file(s)", file=sys.stderr)

    records: dict[int, tuple[list[int], list[tuple]]] = {}
    for sp in spurec_files:
        try:
            data = sp.read_bytes()
            rec_id, entries, ranges = parse_spurec(data)
        except Exception as exc:
            print(f"warning: skipping {sp.name}: {exc}", file=sys.stderr)
            continue
        if rec_id in records:
            # Merge entry points (keep unique, sorted).
            old_entries, old_ranges = records[rec_id]
            merged_entries = sorted(set(old_entries) | set(entries))
            # Merge ranges (keep unique tuples).
            merged_ranges = list(dict.fromkeys(old_ranges + ranges))
            records[rec_id] = (merged_entries, merged_ranges)
        else:
            records[rec_id] = (entries, ranges)

    print(f"parsed {len(records)} unique SPU record(s)", file=sys.stderr)

    # Write the manifest.
    out_path.parent.mkdir(parents=True, exist_ok=True)
    with open(out_path, "w", newline="\n") as f:
        f.write("# ACVD native port: SPU code manifest v1 (generated by acvd-recomp/gen_spu_manifest.py).\n")
        f.write("# Positions only, no code: each record lists SPU entry points and, per local-store range\n")
        f.write("# 'start:end:source:offset' (hex), where the same bytes lie in the user's prepared files.\n")
        f.write("# ac4-prep spu-records rebuilds the code records from those files.\n")

        for idx, (rel_path, sha) in enumerate(sources):
            f.write(f"source {idx} {rel_path} {sha}\n")

        for rec_id, (entries, ranges) in sorted(records.items()):
            # Record id as 16-hex-char string.
            rid = f"{rec_id:016x}"
            # Entry points (hex, comma-separated, no 0x prefix — matches AC4 manifest style).
            ep_str = ",".join(f"{e:x}" for e in sorted(entries))
            # Ranges: start:end:src_idx:offset (all hex, no 0x).
            range_str = " ".join(f"{ls_s:x}:{ls_e:x}:{si}:{fo:x}" for ls_s, ls_e, si, fo in ranges)
            f.write(f"record {rid} {ep_str} {range_str}\n")

    print(f"wrote {len(records)} record(s) to {out_path}", file=sys.stderr)
    print(f"manifest: {out_path}")


if __name__ == "__main__":
    main()
