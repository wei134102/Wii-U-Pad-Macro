# Wii-U-Pad-Macro (GamePad Macro demo)

Experimental **Aroma WUPS** plugin: after `VPADRead` returns, it merges a **configurable macro script** into `VPADStatus` (SD slots, comma-separated script, full-screen node graph; runs after the config menu is closed and the delay elapses).

**Repository:** <https://github.com/wei134102/Wii-U-Pad-Macro>

English readme (this file). Chinese documentation: [`README.md`](README.md).

---

## Attribution (ideas / references)

This repo **implements** Wii U macro logic and script parsing independently. The following projects informed **design** or **project layout**; they are **not** wholesale sources of the business logic:

| Source | What we took from it |
|--------|----------------------|
| **[hid_to_vpad](https://github.com/Maschell/hid_to_vpad)** (Maschell) | Pattern: patch **`VPADStatus` after `VPADRead`** (`hold` / `trigger` / `release`, etc.). This plugin uses **WUPS `WUPS_MUST_REPLACE` + WUT `VPADRead`** at the same layer. |
| **[controller_patcher](https://github.com/Maschell/controller_patcher)** | Background on HID → `VPADData` on older stacks; this repo does **not** link `libcontrollerpatcher`. |
| **[Wii-U-Time-Sync](https://github.com/Nightkingale/Wii-U-Time-Sync)** | **Directory layout**: when placed **next to** this tree, the Makefile picks up `external/libwupsxx` and `WiiUPluginSystem` like common WUPS examples. |
| **devkitPro / [wut](https://github.com/devkitPro/wut) / WUPS** | Toolchain, `VPAD` types, plugin entry. |

Plugin metadata also cites the `hid_to_vpad`-style hook (see `WUPS_PLUGIN_*` in `source/main.cpp`).

---

## Features (summary)

- Multiple SD macro slots, `macro_script`-style comma syntax (including two-phase `first&delay&second+hold` extensions).
- On-console WUPS config UI with graph editing; PC folder `PC_Edieted/` has a Tk node editor and sync for `macros.ini` / `macro1.txt` … `macro8.txt`.

---

## Building the plugin

1. Place this repo **next to** [Wii-U-Time-Sync](https://github.com/Nightkingale/Wii-U-Time-Sync) so the Makefile can resolve `../Wii-U-Time-Sync/external/libwupsxx/...` (`PARENT_DIR` in the root `Makefile`).
2. Install **devkitPro** and set `DEVKITPRO`.
3. From the repo root: `make`  
   Output name depends on the `Makefile` (e.g. `GamePad_Macro_Demo.wps`).

---

## Quick usage

1. Install the built `.wps` under your Aroma plugins path. Put macro files under `sd:/wiiu/gamepad_macro/` (same layout as `sd/wiiu/gamepad_macro` in this repo).
2. WUPS config: set macro delay, edit slot scripts or the node graph (PC editor is recommended).
3. **Close the config menu**; after the delay the macro arms and injects for Player 1 GamePad.
4. You can FTP updated macros to `sd:/wiiu/gamepad_macro/` and test in-game without restarting the title/plugin.
5. Default shortcut to open the Aroma plugin overlay is often **− + L + D-Pad Down**; exit with **B** to leave the menu and start the post-menu delay.

---

## License

- **Original code in this repository:** **MIT** (see [`LICENSE`](LICENSE) and SPDX headers in sources).
- **Linked libraries at build time** (`libwups`, `libwut`, WiiUPluginSystem import libs, etc.) follow **devkitPro / upstream** terms; see your toolchain and the [Wii-U-Time-Sync](https://github.com/Nightkingale/Wii-U-Time-Sync) tree for third-party license files.
- If you **copy or redistribute** third-party **source** (e.g. [hid_to_vpad](https://github.com/Maschell/hid_to_vpad)), comply with **that project’s** license; it is separate from this repo’s MIT.

---

## PC editor (`PC_Edieted/`)

Python 3 + Tk. Edits the same tree as on SD: `macros.ini` and `macro1.txt` … `macro8.txt`.

### UI language (Chinese / English)

- Use the **“UI language”** combobox at the top (**中文** / **English**).
- Choice is saved to `pc_gui_locale.txt` next to `editor.py` (or next to the `.exe` when using a frozen build).
- The **`language` field in `macros.ini`** is separate: it selects **on-console plugin UI** (0 = English, 1 = Chinese), not the PC editor.

### Run from source

```bat
cd PC_Edieted
run_editor.bat
```

Optional: `python editor.py --ui-lang en` or `zh` to override the saved UI language once.

### Build a standalone `.exe` (Windows)

1. Install Python 3 (with Tk).
2. From `PC_Edieted/`:

   ```bat
   build_exe.bat
   ```

   This installs PyInstaller if needed, clears `TCL_LIBRARY` / `TK_LIBRARY` (some tools such as CSR **BlueSuite** set them incorrectly and break Tk / PyInstaller), then runs PyInstaller with `--collect-all tkinter`, and copies `../sd/wiiu/gamepad_macro` into `dist/sd/wiiu/gamepad_macro` so the default macro folder exists beside `GamePadMacroEditor.exe`.

3. Distribute **`dist/GamePadMacroEditor.exe`** together with the **`dist/sd`** folder (or recreate that path next to the exe). The app looks for macros under `<folder containing the exe>/sd/wiiu/gamepad_macro`.

### GitHub Actions / releases

- Every push to `main` / `master` and every pull request builds **`.wps`** (Linux + `devkitpro/devkitppc`) and **`GamePadMacroEditor-windows.zip`** (Windows + PyInstaller) and uploads **Artifacts**.
- Pushing a **`v*`** tag (e.g. `v0.1.0`) also creates a **GitHub Release** with:
  - `GamePad_Macro_Demo.wps`
  - `GamePadMacroEditor-windows.zip` (exe + `sd/wiiu/gamepad_macro` sample tree)
- The WUPS job checks out [Wii-U-Time-Sync](https://github.com/Nightkingale/Wii-U-Time-Sync) on **`master`** with **recursive submodules** next to this repo (same layout as local builds). Workflow: [`.github/workflows/main.yml`](.github/workflows/main.yml).

---

This software is provided “as is”, without warranty. Modifying controller input may affect gameplay; use at your own risk.
