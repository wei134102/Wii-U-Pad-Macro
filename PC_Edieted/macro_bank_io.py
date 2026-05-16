# SPDX-License-Identifier: MIT
"""Load/save GamePad Macro SD bank (macros.ini + macro1.txt .. macro8.txt)."""

from __future__ import annotations

from dataclasses import dataclass, field
from pathlib import Path


def default_macro_root() -> Path:
    """Repo-relative: Wii-U-Pad-Macro/sd/wiiu/gamepad_macro"""
    return Path(__file__).resolve().parent.parent / "sd" / "wiiu" / "gamepad_macro"


def trim(s: str) -> str:
    return s.strip()


@dataclass
class BankState:
    post_menu_delay_ms: int = 1500
    macro_slot_count: int = 4
    macro_to_run: int = 1
    plugin_enabled: bool = True
    language: int = 1  # 0 EN, 1 ZH (matches plugin)
    slots: list[str] = field(default_factory=lambda: [""] * 8)


def parse_kv_line(line: str) -> tuple[str, str] | None:
    if "=" not in line:
        return None
    key, val = line.split("=", 1)
    key = trim(key)
    val = trim(val)
    return (key, val) if key else None


def parse_ini(text: str, state: BankState, slots: list[str]) -> None:
    current_slot = -1
    for raw in text.splitlines():
        line = trim(raw)
        if not line or line.startswith("#"):
            continue
        if len(line) > 2 and line[0] == "[" and line[-1] == "]":
            sec = line[1:-1].strip()
            if sec.startswith("macro") and sec[5:].isdigit():
                n = int(sec[5:])
                if 1 <= n <= 8:
                    current_slot = n - 1
            continue
        kv = parse_kv_line(line)
        if kv is None:
            if current_slot >= 0:
                slots[current_slot] = line
            continue
        key, val = kv
        if current_slot >= 0 and key == "script":
            slots[current_slot] = val
            continue
        if key == "post_menu_delay_ms":
            state.post_menu_delay_ms = int(val)
        elif key == "macro_slot_count":
            state.macro_slot_count = max(1, min(8, int(val)))
        elif key == "macro_to_run":
            state.macro_to_run = max(1, min(8, int(val)))
        elif key == "plugin_enabled":
            state.plugin_enabled = val != "0"
        elif key == "language":
            state.language = 0 if int(val) == 0 else 1


def load_bank(directory: Path) -> BankState:
    directory = Path(directory)
    slots = [""] * 8
    state = BankState(slots=list(slots))
    ini_path = directory / "macros.ini"
    if ini_path.is_file():
        parse_ini(ini_path.read_text(encoding="utf-8"), state, state.slots)
    # macroN.txt overrides (same order as plugin import_all)
    for i in range(8):
        p = directory / f"macro{i + 1}.txt"
        if p.is_file():
            body = p.read_text(encoding="utf-8")
            while body.endswith("\n") or body.endswith("\r"):
                body = body[:-1]
            state.slots[i] = trim(body.replace("\r\n", "\n").replace("\r", "\n"))
    return state


def _ini_slot_body(script: str) -> str:
    """Single segment for [macroN] body line (avoid breaking ini structure)."""
    s = trim(script.replace("\r\n", "\n").replace("\r", "\n"))
    if "\n" in s:
        s = ",".join(trim(x) for x in s.split("\n") if trim(x))
    return s


def build_macros_ini(state: BankState) -> str:
    lines = [
        "# GamePad Macro bank v1",
        "# Copy sd:/wiiu/gamepad_macro/ to share macros",
        "version=1",
        f"post_menu_delay_ms={state.post_menu_delay_ms}",
        f"macro_slot_count={state.macro_slot_count}",
        f"macro_to_run={state.macro_to_run}",
        f"plugin_enabled={1 if state.plugin_enabled else 0}",
        f"language={state.language}",
        "",
    ]
    for i in range(8):
        lines.append(f"[macro{i + 1}]")
        lines.append(_ini_slot_body(state.slots[i]))
        lines.append("")
    return "\n".join(lines).rstrip() + "\n"


def save_bank(directory: Path, state: BankState) -> None:
    directory = Path(directory)
    directory.mkdir(parents=True, exist_ok=True)
    ini_path = directory / "macros.ini"
    ini_path.write_text(build_macros_ini(state), encoding="utf-8")
    for i in range(8):
        body = trim(state.slots[i].replace("\r\n", "\n").replace("\r", "\n"))
        (directory / f"macro{i + 1}.txt").write_text(body + ("\n" if body else ""), encoding="utf-8")
