# ARMORED CORE: VERDICT DAY — unofficial PC port (WIP)

An unofficial native Windows version of **ARMORED CORE: VERDICT DAY** (PS3, USA `BLUS31194` / EUR `BLES01898`), built on top of the static recompilation runtime from [SulphurS16/AC4-Port](https://github.com/SulphurS16/AC4-Port) (Project Glintburn). The game's own PS3 PowerPC & SPU code is translated into a native 64-bit Windows executable, built on your PC from your own copy of the game.

> **Status:** Stage 2.2 Complete (Static Recompilation & HLE Bring-up). Native Windows execution in Direct3D 11 Strict Mode running at stable 30 FPS (`acvd.exe`).

This project contains no game data, no game code and no keys, and it decrypts nothing.

Not affiliated with, endorsed or sponsored by FromSoftware, Bandai Namco Entertainment, Sony Interactive Entertainment or any other rights holder. ARMORED CORE is a trademark of its owners.

## Planned Highlights

- **Native, not emulated.** The whole game runs as a 64-bit Windows program without real-time JIT overhead.
- **Resolution & Render Scale.** Draws at your monitor's native resolution (borderless, windowed, or fullscreen) with adjustable render scale (50% to 200%).
- **High frame rates.** Target support for 30, 60, 120, 144 FPS or uncapped (requires decoupling the engine's fixed simulation step).
- **Full mouse and keyboard support.** Raw mouse input mapped directly to AC turning / camera controls, plus full menu & Workshop navigation.
- **In-game PC SETTINGS.** Display, frame rate, mouse sensitivity, and key bindings integrated into the game's system menu.
- **Regulations and mods.** Support for loading custom `regulation.bin` balance files and loose file overrides (`bind/`).

## How it works

**Static recompilation.** A recompiler reads the game's big-endian PowerPC64 (`PPU`) instructions from the decrypted `EBOOT.elf` and translates them into C++ functions. Your PC's compiler (`llvm-mingw`) then builds a native 64-bit Windows binary (`acvd.exe`).

SPU programs are translated ahead of time too (`acvd_spu_gen`), with an interpreter as a safety net. The RSX command stream is translated to **Direct3D 11**, and the game's vertex/fragment shader microcode is converted into **HLSL**, rendered at your target resolution. Graphics, audio, input, files, and saves are reimplemented in C++, while complex PS3 firmware components (such as the `cellSpurs` task scheduler and audio/video decoders) run as recompiled code from your own PS3 system software files.

This project builds upon the runtime architecture created for *Armored Core 4* and *Armored Core: For Answer*, adapting HLE stubs, RSX methods, and game-specific memory hooks to the 5th-generation FromSoftware engine (Dantelion2).

Nothing from the game is included in this repository. Everything game-derived is generated locally on your PC from your own disc dump.

## Bring-up & Build Instructions

Build all ACVD targets using the project's build tool:
```powershell
.\tools\build.ps1 -Target acvd-recomp
.\tools\build.ps1 -Target acvd-spurecomp
.\tools\build.ps1 -Target acvd
```

Diagnostic and analysis scripts are located in `acvd-recomp/`:
```powershell
.\acvd-recomp\diag.ps1 -Elf D:\ACVD_dec\EBOOT.elf
```
