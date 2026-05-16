# SPDX-License-Identifier: MIT
"""PC editor UI strings (Chinese / English)."""

from __future__ import annotations

import sys
from pathlib import Path

Locale = str  # "zh" | "en"

_T: dict[str, dict[str, str]] = {
    "zh": {
        "app_title": "GamePad Macro — PC 编辑器",
        "macro_folder": "宏文件夹：",
        "browse": "浏览…",
        "ui_language": "界面语言：",
        "lang_zh": "中文",
        "lang_en": "English",
        "plugin_opts_expand": "▶ 插件选项（macros.ini）— 点击展开",
        "plugin_opts_collapse": "▼ 插件选项（macros.ini）— 点击折叠",
        "plugin_opts_frame": "插件选项（macros.ini 头部）",
        "spin_post_menu_delay": "关闭菜单后延迟 post_menu_delay_ms",
        "spin_macro_slot_count": "宏槽位数 macro_slot_count（1–8）",
        "spin_macro_to_run": "要运行的槽 macro_to_run（1–8）",
        "spin_language_plugin": "插件界面语言 language（0=英文 1=中文）",
        "chk_plugin_enabled": "启用插件 plugin_enabled",
        "tab_macro": "宏{n}",
        "btn_load_raw_to_graph": "从原文载入节点图",
        "btn_save_macros": "保存宏",
        "raw_caption": "宏原文（可选）：修改后请点击「从原文载入节点图」。保存时以节点图为准；无法解析时保存原文。",
        "format_help_title": "格式说明",
        "format_help_body": (
            "节点图：每行 10 格顺序换行；蓝字(ms)=下一步间隔 gap；棕字(ms)=当前步按住 hold。\n"
            "ZL / ZR 在脚本里分别写成单个字符 1 与 2（不是键盘的数字抽象符）。十字方向 ^ v < >；+- HOME → P M H。\n"
            "左摇杆方向写入 {w}{a}{s}{d}；右摇杆先写 [r] 再接同上。\n"
            "组合一步也可在节点图勾选「两段式」并填第二段，不必手写 &。\n"
            "示例：A+120,50,B+120"
        ),
        "btn_reload": "重新载入",
        "save_footer_hint": "（保存宏 = 写入 macros.ini + macro1.txt…macro8.txt 到上方文件夹）",
        "status_reload_failed": "重新载入失败",
        "status_save_failed": "保存失败",
        "dlg_reload": "重新载入",
        "dlg_save": "保存",
        "dlg_save_ok": "已写入 macros.ini 与 macro1.txt … macro8.txt。",
        "err_cannot_create_folder": "无法创建文件夹：\n{e}",
        "node_delete": "删除节点",
        "node_insert_after": "后方插入",
        "node_canvas_hint": "（点击画布上的节点选中；点虚线框 + 添加）",
        "sel_node_edit": "选中节点编辑",
        "gap_ms": "此步前间隔 gap (ms)：",
        "hold_ms": "按住 hold (ms)：",
        "two_phase_cb": "两段式：第一段按住不松，延迟后再追加第二段（无需手写 &）",
        "seg1_title": "第一段",
        "regular_buttons": "常规按键：",
        "stick_script_hint": "说明：脚本里 1=ZL，2=ZR；十字用 ^ v < >。",
        "left_stick_title": "左摇杆 → {w}{a}{s}{d}",
        "right_stick_title": "右摇杆 → [r]…",
        "center_stick": "居中",
        "seg2_title": "第二段（追加输入）",
        "delay_then_append": "延迟 (ms) 后再追加：",
        "seg2_buttons": "第二段按键：",
        "seg2_left": "第二段左摇杆",
        "seg2_right": "第二段右摇杆",
        "firmware_note_title": "固件说明",
        "firmware_note_body": (
            "当前插件脚本支持的实体键：A B X Y L R ZL(1) ZR(2) Plus(P) Minus(M) HOME(H)、"
            "十字 ↑↓←→(^ v < >)；摇杆用模拟方向块编码。"
            "不含：按下摇杆键(L3/R3)、TV、Sync — 若需要需在固件 macro_script 里再加映射。"
        ),
        "parse_error_banner": "脚本无法解析为节点图；请用下方「宏原文」改正后再「从原文载入」。保存时将保留原文。",
        "two_phase_need_second": "两段式已勾选：请至少选择第二段的一个按键或摇杆方向；仅改延迟不会生效。",
        "explain_fallback": "（当前槽位脚本未解析为节点，无按键说明；请修正下方原文后点「从原文载入」）",
    },
    "en": {
        "app_title": "GamePad Macro — PC editor",
        "macro_folder": "Macro folder:",
        "browse": "Browse…",
        "ui_language": "UI language:",
        "lang_zh": "中文",
        "lang_en": "English",
        "plugin_opts_expand": "▶ Plugin options (macros.ini) — expand",
        "plugin_opts_collapse": "▼ Plugin options (macros.ini) — collapse",
        "plugin_opts_frame": "Plugin options (macros.ini header)",
        "spin_post_menu_delay": "post_menu_delay_ms (after closing menu)",
        "spin_macro_slot_count": "macro_slot_count (1–8)",
        "spin_macro_to_run": "macro_to_run (1–8)",
        "spin_language_plugin": "language (0=EN 1=ZH, console UI)",
        "chk_plugin_enabled": "plugin_enabled",
        "tab_macro": "macro{n}",
        "btn_load_raw_to_graph": "Load text into graph",
        "btn_save_macros": "Save macros",
        "raw_caption": (
            "Raw script (optional): after edits, click “Load text into graph”. "
            "Save uses the graph when it parses; otherwise the raw text is kept."
        ),
        "format_help_title": "Format",
        "format_help_body": (
            "Grid: 10 cells per row, wrap. Blue ms = gap to next step; brown ms = hold for this step.\n"
            "ZL / ZR are written as 1 and 2 in scripts. D-pad: ^ v < >. + − HOME: P M H.\n"
            "Left stick: {w}{a}{s}{d}; right stick: prefix [r] then the same.\n"
            "Two-phase combos: enable “Two-phase” in the panel instead of typing & manually.\n"
            "Example: A+120,50,B+120"
        ),
        "btn_reload": "Reload",
        "save_footer_hint": "(Save writes macros.ini + macro1.txt … macro8.txt to the folder above.)",
        "status_reload_failed": "Reload failed",
        "status_save_failed": "Save failed",
        "dlg_reload": "Reload",
        "dlg_save": "Save",
        "dlg_save_ok": "Wrote macros.ini and macro1.txt … macro8.txt.",
        "err_cannot_create_folder": "Cannot create folder:\n{e}",
        "node_delete": "Delete node",
        "node_insert_after": "Insert after",
        "node_canvas_hint": "(Click a node to select; click the dashed + to add.)",
        "sel_node_edit": "Selected step",
        "gap_ms": "Gap before this step (ms):",
        "hold_ms": "Hold (ms):",
        "two_phase_cb": "Two-phase: hold phase 1, delay, then append phase 2 (no manual &).",
        "seg1_title": "Phase 1",
        "regular_buttons": "Buttons:",
        "stick_script_hint": "In scripts: 1=ZL, 2=ZR; D-pad uses ^ v < >.",
        "left_stick_title": "Left stick → {w}{a}{s}{d}",
        "right_stick_title": "Right stick → [r]…",
        "center_stick": "Center",
        "seg2_title": "Phase 2 (extra input)",
        "delay_then_append": "Delay (ms) before appending:",
        "seg2_buttons": "Phase 2 buttons:",
        "seg2_left": "Phase 2 left stick",
        "seg2_right": "Phase 2 right stick",
        "firmware_note_title": "Firmware note",
        "firmware_note_body": (
            "Supported in scripts: A B X Y L R ZL(1) ZR(2) Plus(P) Minus(M) HOME(H), "
            "D-pad (^ v < >), stick directions as encoded blocks. "
            "Not included: stick click (L3/R3), TV, Sync — add mappings in firmware if needed."
        ),
        "parse_error_banner": (
            "Script could not be parsed into the graph; fix the raw text below and click "
            "“Load text into graph”. Save will keep the raw text."
        ),
        "two_phase_need_second": "Two-phase enabled: pick at least one button or stick direction for phase 2.",
        "explain_fallback": (
            "(This slot is in raw-text mode; no key legend. Fix the text below, then “Load text into graph”.)"
        ),
    },
}

_BTN_ZH: dict[str, str] = {
    "A": "A",
    "B": "B",
    "X": "X",
    "Y": "Y",
    "L": "L",
    "R": "R",
    "1": "ZL（脚本写 1）",
    "2": "ZR（脚本写 2）",
    "^": "十字 ↑",
    "v": "十字 ↓",
    "<": "十字 ←",
    ">": "十字 →",
    "P": "+（Plus）",
    "M": "-（Minus）",
    "H": "HOME",
}

_BTN_EN: dict[str, str] = {
    "A": "A",
    "B": "B",
    "X": "X",
    "Y": "Y",
    "L": "L",
    "R": "R",
    "1": "ZL (script: 1)",
    "2": "ZR (script: 2)",
    "^": "D-pad ↑",
    "v": "D-pad ↓",
    "<": "D-pad ←",
    ">": "D-pad →",
    "P": "+ (Plus)",
    "M": "- (Minus)",
    "H": "HOME",
}


def locale_config_path() -> Path:
    if getattr(sys, "frozen", False):
        return Path(sys.executable).resolve().parent / "pc_gui_locale.txt"
    return Path(__file__).resolve().parent / "pc_gui_locale.txt"


def load_saved_locale() -> Locale:
    p = locale_config_path()
    try:
        if p.is_file():
            line = p.read_text(encoding="utf-8").strip().splitlines()[0].strip().lower()
            if line in ("en", "zh"):
                return line
    except OSError:
        pass
    return "zh"


def save_locale(loc: Locale) -> None:
    loc = "en" if loc == "en" else "zh"
    try:
        locale_config_path().write_text(loc + "\n", encoding="utf-8")
    except OSError:
        pass


class PcI18n:
    __slots__ = ("locale",)

    def __init__(self, locale: Locale | None = None) -> None:
        self.locale = locale if locale in _T else load_saved_locale()

    def btn_label(self, ch: str) -> str:
        m = _BTN_EN if self.locale == "en" else _BTN_ZH
        return m.get(ch, ch)

    def t(self, key: str, **kwargs: object) -> str:
        lang = self.locale if self.locale in _T else "zh"
        s = _T[lang].get(key) or _T["zh"].get(key) or key
        if kwargs:
            try:
                return s.format(**kwargs)
            except (KeyError, ValueError):
                return s
        return s

    def set_locale(self, loc: Locale) -> None:
        self.locale = "en" if loc == "en" else "zh"
        save_locale(self.locale)
