# SPDX-License-Identifier: MIT
"""
PC editor for Wii-U-Pad-Macro SD folder (macros.ini + macro1.txt .. macro8.txt).

Default folder: <repo>/sd/wiiu/gamepad_macro (same layout as on SD: sd:/wiiu/gamepad_macro).
When built with PyInstaller, defaults to <exe_dir>/sd/wiiu/gamepad_macro.
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
from pc_i18n import PcI18n

_UI_LANG_LABEL_TO_LOCALE = {"中文": "zh", "English": "en"}


class MacroEditorApp(ttk.Frame):
    def __init__(self, master: tk.Tk, macro_dir: Path, i18n: PcI18n | None = None) -> None:
        super().__init__(master, padding=8)
        self.pack(fill=tk.BOTH, expand=True)
        self.macro_dir = Path(macro_dir)
        self.state = BankState()
        self._i18n = i18n or PcI18n()
        self.master = master

        top = ttk.Frame(self)
        top.pack(fill=tk.X, pady=(0, 8))

        lang_row = ttk.Frame(top)
        lang_row.pack(fill=tk.X, pady=(0, 6))
        self._lbl_ui_lang = ttk.Label(lang_row, text="")
        self._lbl_ui_lang.pack(side=tk.LEFT)
        self._ui_lang_var = tk.StringVar(
            value="中文" if self._i18n.locale == "zh" else "English"
        )
        self._ui_lang_box = ttk.Combobox(
            lang_row,
            textvariable=self._ui_lang_var,
            values=("中文", "English"),
            state="readonly",
            width=12,
        )
        self._ui_lang_box.pack(side=tk.LEFT, padx=8)
        self._ui_lang_box.bind("<<ComboboxSelected>>", self._on_ui_lang)

        self._lbl_macro_folder = ttk.Label(top, text="")
        self._lbl_macro_folder.pack(side=tk.LEFT)
        self.dir_var = tk.StringVar(value=str(self.macro_dir))
        ent = ttk.Entry(top, textvariable=self.dir_var, width=64)
        ent.pack(side=tk.LEFT, padx=6, fill=tk.X, expand=True)
        self._btn_browse = ttk.Button(top, text="", command=self._browse_dir)
        self._btn_browse.pack(side=tk.LEFT)

        self._plugin_opts_expanded = False
        self._opts_head = ttk.Frame(self)
        self._opts_head.pack(fill=tk.X, pady=(0, 4))
        self._plugin_opts_toggle = ttk.Button(self._opts_head, text="", command=self._toggle_plugin_opts)
        self._plugin_opts_toggle.pack(side=tk.LEFT)

        self.plugin_opts = ttk.LabelFrame(self, text="", padding=6)

        g = ttk.Frame(self.plugin_opts)
        g.pack(fill=tk.X)
        self.delay_var = tk.IntVar(value=1500)
        self.slot_count_var = tk.IntVar(value=4)
        self.to_run_var = tk.IntVar(value=1)
        self.plugin_on_var = tk.BooleanVar(value=True)
        self.lang_var = tk.IntVar(value=1)
        self._spin_labels: list[tuple[ttk.Label, str]] = []

        def spin(row: int, col: int, msg_key: str, var: tk.Variable, frm: ttk.Frame = g) -> None:
            f = ttk.Frame(frm)
            f.grid(row=row, column=col, sticky=tk.W, padx=8, pady=4)
            lb = ttk.Label(f, text="")
            lb.pack(side=tk.LEFT)
            self._spin_labels.append((lb, msg_key))
            sp = ttk.Spinbox(f, textvariable=var, width=8)
            sp.pack(side=tk.LEFT, padx=4)

        spin(0, 0, "spin_post_menu_delay", self.delay_var)
        spin(0, 1, "spin_macro_slot_count", self.slot_count_var)
        spin(1, 0, "spin_macro_to_run", self.to_run_var)
        spin(1, 1, "spin_language_plugin", self.lang_var)
        pf = ttk.Frame(g)
        pf.grid(row=0, column=2, rowspan=2, sticky=tk.W, padx=12)
        self._chk_plugin = ttk.Checkbutton(pf, text="", variable=self.plugin_on_var)
        self._chk_plugin.pack(anchor=tk.W)

        nb = ttk.Notebook(self)
        nb.pack(fill=tk.BOTH, expand=True, pady=(0, 8))
        self._notebook = nb
        self.slot_editors: list[NodeGraphEditor] = []
        self.slot_raws: list[tk.Text] = []
        self._tab_raw_labels: list[ttk.Label] = []
        self._tab_load_btns: list[ttk.Button] = []
        self._tab_save_btns: list[ttk.Button] = []
        for i in range(8):
            tab = ttk.Frame(nb, padding=4)
            nb.add(tab, text="")

            pw = ttk.Panedwindow(tab, orient=tk.VERTICAL)
            pw.pack(fill=tk.BOTH, expand=True)

            upper = ttk.Frame(pw)
            lower = ttk.Frame(pw)
            pw.add(upper, weight=4)
            pw.add(lower, weight=1)

            ge = NodeGraphEditor(upper, on_change=lambda idx=i: self._sync_raw_from_graph(idx), i18n=self._i18n)
            ge.pack(fill=tk.BOTH, expand=True)
            self.slot_editors.append(ge)

            bf = ttk.Frame(lower)
            bf.pack(side=tk.BOTTOM, fill=tk.X, pady=(4, 0))
            b_load = ttk.Button(bf, text="", command=lambda idx=i: self._apply_raw_to_graph(idx))
            b_load.pack(side=tk.LEFT)
            self._tab_load_btns.append(b_load)
            b_save = ttk.Button(bf, text="", command=self.save)
            b_save.pack(side=tk.LEFT, padx=(12, 0))
            self._tab_save_btns.append(b_save)

            lf = ttk.Frame(lower)
            lf.pack(fill=tk.BOTH, expand=True, side=tk.TOP)
            rl = ttk.Label(lf, text="")
            rl.pack(anchor=tk.W)
            self._tab_raw_labels.append(rl)
            rf = ttk.Frame(lf)
            rf.pack(fill=tk.X, expand=False, pady=(2, 0))
            tx = tk.Text(rf, height=2, font=("Consolas", 10))
            sb = ttk.Scrollbar(rf, orient=tk.VERTICAL, command=tx.yview)
            tx.configure(yscrollcommand=sb.set)
            tx.pack(side=tk.LEFT, fill=tk.X, expand=True)
            sb.pack(side=tk.RIGHT, fill=tk.Y)
            self.slot_raws.append(tx)

        self._help_frame = ttk.LabelFrame(self, text="", padding=6)
        self._help_frame.pack(fill=tk.X, pady=(0, 8))
        self._help_label = ttk.Label(self._help_frame, text="", justify=tk.LEFT)
        self._help_label.pack(anchor=tk.W)

        bot = ttk.Frame(self)
        bot.pack(fill=tk.X)
        self._btn_reload = ttk.Button(bot, text="", command=self.reload)
        self._btn_reload.pack(side=tk.LEFT, padx=(0, 8))
        self._btn_save = ttk.Button(bot, text="", command=self.save)
        self._btn_save.pack(side=tk.LEFT, padx=(0, 8))
        self._lbl_save_footer = ttk.Label(bot, text="", foreground="#555")
        self._lbl_save_footer.pack(side=tk.LEFT, padx=(4, 0))
        self.status = tk.StringVar(value="")
        ttk.Label(bot, textvariable=self.status).pack(side=tk.LEFT, padx=16)

        self._apply_ui_language()
        self.reload()

    def _on_ui_lang(self, _ev: tk.Event | None = None) -> None:
        loc = _UI_LANG_LABEL_TO_LOCALE.get(self._ui_lang_var.get().strip(), "zh")
        self._i18n.set_locale(loc)
        self._apply_ui_language()

    def _apply_ui_language(self) -> None:
        tr = self._i18n.t
        self.master.title(tr("app_title"))
        self._lbl_ui_lang.configure(text=tr("ui_language"))
        self._lbl_macro_folder.configure(text=tr("macro_folder"))
        self._btn_browse.configure(text=tr("browse"))
        self._plugin_opts_toggle.configure(
            text=tr("plugin_opts_collapse" if self._plugin_opts_expanded else "plugin_opts_expand")
        )
        self.plugin_opts.configure(text=tr("plugin_opts_frame"))
        for lb, key in self._spin_labels:
            lb.configure(text=tr(key))
        self._chk_plugin.configure(text=tr("chk_plugin_enabled"))
        for i in range(8):
            self._notebook.tab(i, text=tr("tab_macro", n=i + 1))
            self._tab_load_btns[i].configure(text=tr("btn_load_raw_to_graph"))
            self._tab_save_btns[i].configure(text=tr("btn_save_macros"))
            self._tab_raw_labels[i].configure(text=tr("raw_caption"))
            self.slot_editors[i].set_i18n(self._i18n)
        self._help_frame.configure(text=tr("format_help_title"))
        self._help_label.configure(text=tr("format_help_body"))
        self._btn_reload.configure(text=tr("btn_reload"))
        self._btn_save.configure(text=tr("btn_save_macros"))
        self._lbl_save_footer.configure(text=tr("save_footer_hint"))

    def _toggle_plugin_opts(self) -> None:
        self._plugin_opts_expanded = not self._plugin_opts_expanded
        tr = self._i18n.t
        if self._plugin_opts_expanded:
            self.plugin_opts.pack(fill=tk.X, pady=(0, 8), after=self._opts_head)
            self._plugin_opts_toggle.configure(text=tr("plugin_opts_collapse"))
        else:
            self.plugin_opts.pack_forget()
            self._plugin_opts_toggle.configure(text=tr("plugin_opts_expand"))

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
        tr = self._i18n.t
        if not path.is_dir():
            try:
                path.mkdir(parents=True, exist_ok=True)
            except OSError as e:
                messagebox.showerror(tr("dlg_reload"), tr("err_cannot_create_folder", e=e))
                self.status.set(tr("status_reload_failed"))
                return
        try:
            self.state = load_bank(path)
            self._state_to_gui(self.state)
            self.status.set(f"Loaded from {path}")
        except Exception as e:
            messagebox.showerror(tr("dlg_reload"), str(e))
            self.status.set(tr("status_reload_failed"))

    def save(self) -> None:
        path = Path(self.dir_var.get().strip())
        self.macro_dir = path
        tr = self._i18n.t
        try:
            path.mkdir(parents=True, exist_ok=True)
            st = self._gui_to_state()
            save_bank(path, st)
            self.state = st
            self.status.set(f"Saved to {path}")
            messagebox.showinfo(tr("dlg_save"), tr("dlg_save_ok"))
        except Exception as e:
            messagebox.showerror(tr("dlg_save"), str(e))
            self.status.set(tr("status_save_failed"))


def main() -> int:
    ap = argparse.ArgumentParser(description="GamePad Macro PC editor")
    ap.add_argument(
        "folder",
        nargs="?",
        default=str(default_macro_root()),
        help=f"Macro folder (default: {default_macro_root()})",
    )
    ap.add_argument(
        "--ui-lang",
        choices=("zh", "en"),
        default=None,
        help="UI language (default: from pc_gui_locale.txt or Chinese).",
    )
    args = ap.parse_args()
    i18n = PcI18n()
    if args.ui_lang:
        i18n.set_locale(args.ui_lang)
    root = tk.Tk()
    root.geometry("1160x820")
    MacroEditorApp(root, Path(args.folder), i18n=i18n)
    root.mainloop()
    return 0


if __name__ == "__main__":
    sys.exit(main())
