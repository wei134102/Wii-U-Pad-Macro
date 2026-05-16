# SPDX-License-Identifier: MIT
"""
PC editor for Wii-U-Pad-Macro SD folder (macros.ini + macro1.txt .. macro8.txt).

Default folder: <repo>/sd/wiiu/gamepad_macro (same layout as on SD: sd:/wiiu/gamepad_macro).
"""

from __future__ import annotations

import argparse
import os
import sys
from pathlib import Path


def _fix_windows_tcl_tk_paths() -> None:
    """
    Third-party tools (e.g. CSR BlueSuite) often set TCL_LIBRARY / TK_LIBRARY to
    invalid folders; Tk then fails with 'Can't find a usable init.tcl'.
    Force Tcl/Tk bundled with this Python install.
    """
    roots = [Path(sys.prefix).resolve()]
    bp = getattr(sys, "base_prefix", "")
    if bp and Path(bp).resolve() != roots[0]:
        roots.append(Path(bp).resolve())

    for prefix in roots:
        for ver in ("8.6", "9.0"):
            tcl_dir = prefix / "tcl" / f"tcl{ver}"
            if (tcl_dir / "init.tcl").is_file():
                os.environ["TCL_LIBRARY"] = str(tcl_dir)
                break
        else:
            continue
        break

    for prefix in roots:
        for ver in ("8.6", "9.0"):
            tk_dir = prefix / "tcl" / f"tk{ver}"
            if (tk_dir / "tk.tcl").is_file():
                os.environ["TK_LIBRARY"] = str(tk_dir)
                break
        else:
            continue
        break


_fix_windows_tcl_tk_paths()

import tkinter as tk
from tkinter import filedialog, messagebox, ttk

from macro_bank_io import BankState, default_macro_root, load_bank, save_bank
from node_editor import NodeGraphEditor


FORMAT_HELP = (
    "节点图：每行 10 格顺序换行；蓝字(ms)=下一步间隔 gap；棕字(ms)=当前步按住 hold。\n"
    "ZL / ZR 在脚本里分别写成单个字符 1 与 2（不是键盘的数字抽象符）。十字方向 ^ v < >；+- HOME → P M H。\n"
    "左摇杆方向写入 {w}{a}{s}{d}；右摇杆先写 [r] 再接同上。\n"
    "组合一步也可在节点图勾选「两段式」并填第二段，不必手写 &。\n"
    "示例：A+120,50,B+120"
)


class MacroEditorApp(ttk.Frame):
    def __init__(self, master: tk.Tk, macro_dir: Path) -> None:
        super().__init__(master, padding=8)
        self.pack(fill=tk.BOTH, expand=True)
        self.macro_dir = Path(macro_dir)
        self.state = BankState()

        top = ttk.Frame(self)
        top.pack(fill=tk.X, pady=(0, 8))

        ttk.Label(top, text="Macro folder:").pack(side=tk.LEFT)
        self.dir_var = tk.StringVar(value=str(self.macro_dir))
        ent = ttk.Entry(top, textvariable=self.dir_var, width=72)
        ent.pack(side=tk.LEFT, padx=6, fill=tk.X, expand=True)
        ttk.Button(top, text="Browse…", command=self._browse_dir).pack(side=tk.LEFT)

        self._plugin_opts_expanded = False
        self._opts_head = ttk.Frame(self)
        self._opts_head.pack(fill=tk.X, pady=(0, 4))
        self._plugin_opts_toggle = ttk.Button(
            self._opts_head,
            text="▶ Plugin options（macros.ini）— 点击展开",
            command=self._toggle_plugin_opts,
        )
        self._plugin_opts_toggle.pack(side=tk.LEFT)

        self.plugin_opts = ttk.LabelFrame(self, text="Plugin options (macros.ini header)", padding=6)

        g = ttk.Frame(self.plugin_opts)
        g.pack(fill=tk.X)
        self.delay_var = tk.IntVar(value=1500)
        self.slot_count_var = tk.IntVar(value=4)
        self.to_run_var = tk.IntVar(value=1)
        self.plugin_on_var = tk.BooleanVar(value=True)
        self.lang_var = tk.IntVar(value=1)

        def spin(row: int, col: int, label: str, var: tk.Variable, frm: ttk.Frame = g) -> None:
            f = ttk.Frame(frm)
            f.grid(row=row, column=col, sticky=tk.W, padx=8, pady=4)
            ttk.Label(f, text=label).pack(side=tk.LEFT)
            sp = ttk.Spinbox(f, textvariable=var, width=8)
            sp.pack(side=tk.LEFT, padx=4)

        spin(0, 0, "post_menu_delay_ms", self.delay_var)
        spin(0, 1, "macro_slot_count (1–8)", self.slot_count_var)
        spin(1, 0, "macro_to_run (1–8)", self.to_run_var)
        spin(1, 1, "language (0=EN 1=ZH)", self.lang_var)
        pf = ttk.Frame(g)
        pf.grid(row=0, column=2, rowspan=2, sticky=tk.W, padx=12)
        ttk.Checkbutton(pf, text="plugin_enabled", variable=self.plugin_on_var).pack(anchor=tk.W)

        nb = ttk.Notebook(self)
        nb.pack(fill=tk.BOTH, expand=True, pady=(0, 8))
        self.slot_editors: list[NodeGraphEditor] = []
        self.slot_raws: list[tk.Text] = []
        for i in range(8):
            tab = ttk.Frame(nb, padding=4)
            nb.add(tab, text=f"macro{i + 1}")

            pw = ttk.Panedwindow(tab, orient=tk.VERTICAL)
            pw.pack(fill=tk.BOTH, expand=True)

            upper = ttk.Frame(pw)
            lower = ttk.Frame(pw)
            pw.add(upper, weight=4)
            pw.add(lower, weight=1)

            ge = NodeGraphEditor(upper, on_change=lambda idx=i: self._sync_raw_from_graph(idx))
            ge.pack(fill=tk.BOTH, expand=True)
            self.slot_editors.append(ge)

            bf = ttk.Frame(lower)
            bf.pack(side=tk.BOTTOM, fill=tk.X, pady=(4, 0))
            ttk.Button(bf, text="从原文载入节点图", command=lambda idx=i: self._apply_raw_to_graph(idx)).pack(
                side=tk.LEFT
            )
            ttk.Button(bf, text="保存宏", command=self.save).pack(side=tk.LEFT, padx=(12, 0))

            lf = ttk.Frame(lower)
            lf.pack(fill=tk.BOTH, expand=True, side=tk.TOP)
            ttk.Label(
                lf,
                text="宏原文（可选）：修改后请点击「从原文载入节点图」。保存时以节点图为准；无法解析时保存原文。",
            ).pack(anchor=tk.W)
            rf = ttk.Frame(lf)
            rf.pack(fill=tk.X, expand=False, pady=(2, 0))
            tx = tk.Text(rf, height=2, font=("Consolas", 10))
            sb = ttk.Scrollbar(rf, orient=tk.VERTICAL, command=tx.yview)
            tx.configure(yscrollcommand=sb.set)
            tx.pack(side=tk.LEFT, fill=tk.X, expand=True)
            sb.pack(side=tk.RIGHT, fill=tk.Y)
            self.slot_raws.append(tx)

        help_f = ttk.LabelFrame(self, text="格式说明", padding=6)
        help_f.pack(fill=tk.X, pady=(0, 8))
        ttk.Label(help_f, text=FORMAT_HELP, justify=tk.LEFT).pack(anchor=tk.W)

        bot = ttk.Frame(self)
        bot.pack(fill=tk.X)
        ttk.Button(bot, text="重新载入", command=self.reload).pack(side=tk.LEFT, padx=(0, 8))
        ttk.Button(bot, text="保存宏", command=self.save).pack(side=tk.LEFT, padx=(0, 8))
        ttk.Label(
            bot,
            text="（保存宏 = 写入 macros.ini + macro1.txt…macro8.txt 到上方文件夹）",
            foreground="#555",
        ).pack(side=tk.LEFT, padx=(4, 0))
        self.status = tk.StringVar(value="")
        ttk.Label(bot, textvariable=self.status).pack(side=tk.LEFT, padx=16)

        self.reload()

    def _toggle_plugin_opts(self) -> None:
        self._plugin_opts_expanded = not self._plugin_opts_expanded
        if self._plugin_opts_expanded:
            # Must pack after the toggle row; otherwise pack() appends to the end and the
            # panel appears below the notebook / status bar (looks "missing").
            self.plugin_opts.pack(fill=tk.X, pady=(0, 8), after=self._opts_head)
            self._plugin_opts_toggle.configure(text="▼ Plugin options（macros.ini）— 点击折叠")
        else:
            self.plugin_opts.pack_forget()
            self._plugin_opts_toggle.configure(text="▶ Plugin options（macros.ini）— 点击展开")

    def _sync_raw_from_graph(self, idx: int) -> None:
        ge = self.slot_editors[idx]
        if ge.is_fallback_mode():
            return
        s = ge.get_script()
        tx = self.slot_raws[idx]
        tx.delete("1.0", tk.END)
        tx.insert(tk.END, s)

    def _apply_raw_to_graph(self, idx: int) -> None:
        raw = self.slot_raws[idx].get("1.0", tk.END).strip()
        self.slot_editors[idx].reload_from_raw(raw)
        ge = self.slot_editors[idx]
        if not ge.is_fallback_mode():
            self.slot_raws[idx].delete("1.0", tk.END)
            self.slot_raws[idx].insert(tk.END, ge.get_script())

    def _browse_dir(self) -> None:
        p = filedialog.askdirectory(initialdir=self.macro_dir)
        if p:
            self.macro_dir = Path(p)
            self.dir_var.set(str(self.macro_dir))
            self.reload()

    def _gui_to_state(self) -> BankState:
        st = BankState(
            post_menu_delay_ms=max(0, int(self.delay_var.get())),
            macro_slot_count=max(1, min(8, int(self.slot_count_var.get()))),
            macro_to_run=max(1, min(8, int(self.to_run_var.get()))),
            plugin_enabled=bool(self.plugin_on_var.get()),
            language=0 if int(self.lang_var.get()) == 0 else 1,
            slots=[
                self.slot_raws[i].get("1.0", tk.END).strip()
                if self.slot_editors[i].is_fallback_mode()
                else self.slot_editors[i].get_script()
                for i in range(8)
            ],
        )
        return st

    def _state_to_gui(self, st: BankState) -> None:
        self.delay_var.set(st.post_menu_delay_ms)
        self.slot_count_var.set(st.macro_slot_count)
        self.to_run_var.set(st.macro_to_run)
        self.plugin_on_var.set(st.plugin_enabled)
        self.lang_var.set(st.language)
        for i in range(8):
            sl = st.slots[i]
            self.slot_editors[i].set_script(sl)
            self.slot_raws[i].delete("1.0", tk.END)
            if self.slot_editors[i].is_fallback_mode():
                self.slot_raws[i].insert(tk.END, sl)
            else:
                self.slot_raws[i].insert(tk.END, self.slot_editors[i].get_script())

    def reload(self) -> None:
        path = Path(self.dir_var.get().strip())
        self.macro_dir = path
        if not path.is_dir():
            try:
                path.mkdir(parents=True, exist_ok=True)
            except OSError as e:
                messagebox.showerror("Reload", f"Cannot create folder:\n{e}")
                self.status.set("Reload failed")
                return
        try:
            self.state = load_bank(path)
            self._state_to_gui(self.state)
            self.status.set(f"Loaded from {path}")
        except Exception as e:
            messagebox.showerror("Reload", str(e))
            self.status.set("Reload failed")

    def save(self) -> None:
        path = Path(self.dir_var.get().strip())
        self.macro_dir = path
        try:
            path.mkdir(parents=True, exist_ok=True)
            st = self._gui_to_state()
            save_bank(path, st)
            self.state = st
            self.status.set(f"Saved to {path}")
            messagebox.showinfo("Save", "macros.ini and macro1.txt .. macro8.txt written.")
        except Exception as e:
            messagebox.showerror("Save", str(e))
            self.status.set("Save failed")


def main() -> int:
    ap = argparse.ArgumentParser(description="GamePad Macro PC editor")
    ap.add_argument(
        "folder",
        nargs="?",
        default=str(default_macro_root()),
        help=f"Macro folder (default: {default_macro_root()})",
    )
    args = ap.parse_args()
    root = tk.Tk()
    root.title("GamePad Macro — PC editor")
    root.geometry("1160x820")
    MacroEditorApp(root, Path(args.folder))
    root.mainloop()
    return 0


if __name__ == "__main__":
    sys.exit(main())
