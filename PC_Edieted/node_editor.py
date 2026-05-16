# SPDX-License-Identifier: MIT
"""Node grid macro editor (10 columns per row, auto wrap): gap above link, hold below."""

from __future__ import annotations

import tkinter as tk
from tkinter import ttk
from typing import Callable

import macro_script_py as ms

COLS = 10
MARGIN = 14
CELL_W = 96
CELL_H = 88
BOX = 44
ROUTE_PAD = 28

# Script char -> checkbox label (matches firmware macro_script char_mask)
_BTN_ROWS = [
    ["A", "B", "X", "Y"],
    ["L", "R", "1", "2"],
    ["^", "v", "<", ">"],
    ["P", "M", "H"],
]

_BTN_LABEL = {
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


def _axes_from_boxes(up: bool, dn: bool, lf: bool, rt: bool) -> tuple[float, float]:
    y = (1.0 if up else 0.0) + (-1.0 if dn else 0.0)
    x = (-1.0 if lf else 0.0) + (1.0 if rt else 0.0)
    return x, y


def _boxes_from_axes(x: float, y: float) -> tuple[bool, bool, bool, bool]:
    return y > 0.5, y < -0.5, x < -0.5, x > 0.5


def trim_tail(steps: list[ms.Step]) -> None:
    while steps and not (ms.step_has_input(steps[-1]) or steps[-1].pre_gap_ms > 0):
        steps.pop()


def _mask_bit(ch: str) -> int:
    return ms._CHAR_TO_MASK.get(ch, 0)


class NodeGraphEditor(ttk.Frame):
    def __init__(
        self,
        master: tk.Widget,
        on_change: Callable[[], None] | None = None,
    ) -> None:
        super().__init__(master)
        self._on_change = on_change
        self._steps: list[ms.Step] = []
        self._fallback_script: str | None = None
        self._selected: int | None = None
        self._synced_sel_idx: int | None = -1
        self._force_resync_combo: bool = True
        self._follow_sensitive: list[ttk.Widget] = []

        # Footer must be packed before the expanding canvas, otherwise the canvas
        # steals all vertical space and buttons / edit panel disappear below the fold.
        footer = ttk.Frame(self)

        top = ttk.Frame(self)
        self._canvas = tk.Canvas(
            top,
            background="#f4f4f4",
            highlightthickness=0,
            scrollregion=(0, 0, 800, 400),
        )
        ys = ttk.Scrollbar(top, orient=tk.VERTICAL, command=self._canvas.yview)
        xs = ttk.Scrollbar(top, orient=tk.HORIZONTAL, command=self._canvas.xview)
        self._canvas.configure(yscrollcommand=ys.set, xscrollcommand=xs.set)
        self._canvas.grid(row=0, column=0, sticky="nsew")
        ys.grid(row=0, column=1, sticky="ns")
        xs.grid(row=1, column=0, sticky="ew")
        top.grid_rowconfigure(0, weight=1)
        top.grid_columnconfigure(0, weight=1)

        self._banner = ttk.Label(footer, foreground="#b00020")
        self._banner.pack(fill=tk.X, pady=(0, 4))

        bar = ttk.Frame(footer)
        bar.pack(fill=tk.X, pady=(0, 6))
        ttk.Button(bar, text="删除节点", command=self._delete_selected).pack(side=tk.LEFT, padx=(0, 8))
        ttk.Button(bar, text="后方插入", command=self._insert_after).pack(side=tk.LEFT)
        ttk.Label(bar, text="（点画布节点选中；点虚线框 + 添加）").pack(side=tk.LEFT, padx=12)

        edit = ttk.LabelFrame(footer, text="选中节点编辑", padding=6)
        edit.pack(fill=tk.BOTH, expand=True, pady=(0, 4))

        gh = ttk.Frame(edit)
        gh.pack(fill=tk.X)
        ttk.Label(gh, text="此步前间隔 gap (ms):").pack(side=tk.LEFT)
        self._gap_var = tk.IntVar(value=0)
        ttk.Spinbox(gh, from_=0, to=60000, textvariable=self._gap_var, width=8, command=self._apply_panel).pack(
            side=tk.LEFT, padx=6
        )
        ttk.Label(gh, text="按住 hold (ms):").pack(side=tk.LEFT, padx=(16, 0))
        self._hold_var = tk.IntVar(value=80)
        ttk.Spinbox(gh, from_=1, to=5000, textvariable=self._hold_var, width=8, command=self._apply_panel).pack(
            side=tk.LEFT, padx=6
        )

        combo_row = ttk.Frame(edit)
        combo_row.pack(fill=tk.X, pady=(6, 0))
        self._combo_var = tk.BooleanVar(value=False)
        ttk.Checkbutton(
            combo_row,
            text="两段式：第一段按住不松，延迟后再追加第二段（无需手写 &）",
            variable=self._combo_var,
            command=self._on_two_phase_toggle,
        ).pack(side=tk.LEFT)

        phases = ttk.Frame(edit)
        phases.pack(fill=tk.BOTH, expand=True, pady=(6, 0))
        phases.columnconfigure(0, weight=1, uniform="ph")
        phases.columnconfigure(1, weight=1, uniform="ph")
        phases.rowconfigure(0, weight=1)

        seg1 = ttk.LabelFrame(phases, text="第一段", padding=4)
        seg1.grid(row=0, column=0, sticky=tk.NSEW, padx=(0, 4))

        keys = ttk.Frame(seg1)
        keys.pack(fill=tk.X)
        ttk.Label(keys, text="常规按键:").pack(anchor=tk.W)
        self._btn_vars: dict[str, tk.BooleanVar] = {}
        for row in _BTN_ROWS:
            rf = ttk.Frame(keys)
            rf.pack(fill=tk.X)
            for ch in row:
                var = tk.BooleanVar(value=False)
                self._btn_vars[ch] = var
                ttk.Checkbutton(
                    rf,
                    text=_BTN_LABEL.get(ch, ch),
                    variable=var,
                    command=self._apply_panel,
                ).pack(side=tk.LEFT, padx=3, pady=1)

        hint = ttk.Label(
            keys,
            text="说明：脚本里 1=ZL，2=ZR；十字用 ^ v < >。",
            foreground="#444",
            wraplength=320,
        )
        hint.pack(anchor=tk.W, pady=(4, 0))

        sticks_wrap = ttk.Frame(seg1)
        sticks_wrap.pack(fill=tk.X, pady=(6, 0))
        sticks_wrap.columnconfigure(0, weight=1, uniform="stk1")
        sticks_wrap.columnconfigure(1, weight=1, uniform="stk1")

        lst_f = ttk.LabelFrame(sticks_wrap, text="左摇杆 → {w}{a}{s}{d}", padding=4)
        lst_f.grid(row=0, column=0, sticky=tk.NSEW, padx=(0, 4))
        self._lst_vars = {k: tk.BooleanVar(value=False) for k in ("up", "down", "left", "right")}
        lf_row = ttk.Frame(lst_f)
        lf_row.pack(fill=tk.X)
        ttk.Checkbutton(lf_row, text="↑", variable=self._lst_vars["up"], command=self._apply_panel).pack(
            side=tk.LEFT, padx=3
        )
        ttk.Checkbutton(lf_row, text="↓", variable=self._lst_vars["down"], command=self._apply_panel).pack(
            side=tk.LEFT, padx=3
        )
        ttk.Checkbutton(lf_row, text="←", variable=self._lst_vars["left"], command=self._apply_panel).pack(
            side=tk.LEFT, padx=3
        )
        ttk.Checkbutton(lf_row, text="→", variable=self._lst_vars["right"], command=self._apply_panel).pack(
            side=tk.LEFT, padx=3
        )
        ttk.Button(lf_row, text="居中", command=self._clear_left_stick).pack(side=tk.LEFT, padx=6)

        rst_f = ttk.LabelFrame(sticks_wrap, text="右摇杆 → [r]…", padding=4)
        rst_f.grid(row=0, column=1, sticky=tk.NSEW, padx=(4, 0))
        self._rst_vars = {k: tk.BooleanVar(value=False) for k in ("up", "down", "left", "right")}
        rf_row = ttk.Frame(rst_f)
        rf_row.pack(fill=tk.X)
        ttk.Checkbutton(rf_row, text="↑", variable=self._rst_vars["up"], command=self._apply_panel).pack(
            side=tk.LEFT, padx=3
        )
        ttk.Checkbutton(rf_row, text="↓", variable=self._rst_vars["down"], command=self._apply_panel).pack(
            side=tk.LEFT, padx=3
        )
        ttk.Checkbutton(rf_row, text="←", variable=self._rst_vars["left"], command=self._apply_panel).pack(
            side=tk.LEFT, padx=3
        )
        ttk.Checkbutton(rf_row, text="→", variable=self._rst_vars["right"], command=self._apply_panel).pack(
            side=tk.LEFT, padx=3
        )
        ttk.Button(rf_row, text="居中", command=self._clear_right_stick).pack(side=tk.LEFT, padx=6)

        self._two_phase_frame = ttk.LabelFrame(phases, text="第二段（追加输入）", padding=4)
        self._two_phase_frame.grid(row=0, column=1, sticky=tk.NSEW, padx=(4, 0))

        fd_row = ttk.Frame(self._two_phase_frame)
        fd_row.pack(fill=tk.X)
        ttk.Label(fd_row, text="延迟 (ms) 后再追加:").pack(side=tk.LEFT)
        self._follow_delay_var = tk.IntVar(value=100)
        sp_fd = ttk.Spinbox(
            fd_row,
            from_=0,
            to=5000,
            width=8,
            textvariable=self._follow_delay_var,
            command=self._apply_panel,
        )
        sp_fd.pack(side=tk.LEFT, padx=6)
        self._follow_sensitive.append(sp_fd)

        fmid = ttk.Frame(self._two_phase_frame)
        fmid.pack(fill=tk.BOTH, expand=True, pady=(6, 0))
        fmid.columnconfigure(0, weight=1)
        fmid.columnconfigure(1, weight=1)

        fkeys = ttk.Frame(fmid)
        fkeys.grid(row=0, column=0, columnspan=2, sticky=tk.EW)
        ttk.Label(fkeys, text="第二段按键:").pack(anchor=tk.W)
        self._fbtn_vars: dict[str, tk.BooleanVar] = {}
        for row in _BTN_ROWS:
            rf = ttk.Frame(fkeys)
            rf.pack(fill=tk.X)
            for ch in row:
                var = tk.BooleanVar(value=False)
                self._fbtn_vars[ch] = var
                w = ttk.Checkbutton(
                    rf,
                    text=_BTN_LABEL.get(ch, ch),
                    variable=var,
                    command=self._apply_panel,
                )
                w.pack(side=tk.LEFT, padx=3, pady=1)
                self._follow_sensitive.append(w)

        fst_wrap = ttk.Frame(fmid)
        fst_wrap.grid(row=1, column=0, columnspan=2, sticky=tk.EW, pady=(6, 0))
        fst_wrap.columnconfigure(0, weight=1, uniform="fst")
        fst_wrap.columnconfigure(1, weight=1, uniform="fst")

        flst_f = ttk.LabelFrame(fst_wrap, text="第二段左摇杆", padding=4)
        flst_f.grid(row=0, column=0, sticky=tk.NSEW, padx=(0, 4))
        self._flst_vars = {k: tk.BooleanVar(value=False) for k in ("up", "down", "left", "right")}
        fl_row = ttk.Frame(flst_f)
        fl_row.pack(fill=tk.X)
        for lab, k in (("↑", "up"), ("↓", "down"), ("←", "left"), ("→", "right")):
            w = ttk.Checkbutton(fl_row, text=lab, variable=self._flst_vars[k], command=self._apply_panel)
            w.pack(side=tk.LEFT, padx=3)
            self._follow_sensitive.append(w)
        wb = ttk.Button(fl_row, text="居中", command=self._clear_follow_left_stick)
        wb.pack(side=tk.LEFT, padx=6)
        self._follow_sensitive.append(wb)

        frst_f = ttk.LabelFrame(fst_wrap, text="第二段右摇杆", padding=4)
        frst_f.grid(row=0, column=1, sticky=tk.NSEW, padx=(4, 0))
        self._frst_vars = {k: tk.BooleanVar(value=False) for k in ("up", "down", "left", "right")}
        fr_row = ttk.Frame(frst_f)
        fr_row.pack(fill=tk.X)
        for lab, k in (("↑", "up"), ("↓", "down"), ("←", "left"), ("→", "right")):
            w = ttk.Checkbutton(fr_row, text=lab, variable=self._frst_vars[k], command=self._apply_panel)
            w.pack(side=tk.LEFT, padx=3)
            self._follow_sensitive.append(w)
        wb2 = ttk.Button(fr_row, text="居中", command=self._clear_follow_right_stick)
        wb2.pack(side=tk.LEFT, padx=6)
        self._follow_sensitive.append(wb2)

        fw = ttk.LabelFrame(edit, text="固件说明", padding=4)
        fw.pack(fill=tk.X, pady=(6, 0))
        ttk.Label(
            fw,
            text=(
                "当前插件脚本支持的实体键：A B X Y L R ZL(1) ZR(2) Plus(P) Minus(M) HOME(H)、十字 ↑↓←→(^ v < >)；"
                "摇杆用模拟方向块编码。"
                "不含：按下摇杆键(L3/R3)、TV、Sync — 若需要需在固件 macro_script 里再加映射。"
            ),
            foreground="#555",
            wraplength=520,
            justify=tk.LEFT,
        ).pack(anchor=tk.W)

        self._explain = tk.Label(
            edit,
            text="",
            fg="#c62828",
            bg="#f5f5f5",
            justify=tk.LEFT,
            wraplength=520,
            font=("Segoe UI", 10),
        )
        self._explain.pack(fill=tk.X, pady=(10, 0), anchor=tk.W)

        self._canvas.bind("<Configure>", lambda _e: self._redraw())
        self._canvas.bind("<Button-1>", self._on_click)

        footer.pack(side=tk.BOTTOM, fill=tk.X)
        top.pack(side=tk.TOP, fill=tk.BOTH, expand=True)

        self._redraw()

    def _refresh_key_explain(self) -> None:
        if self._fallback_script is not None:
            self._explain.config(
                text="（当前槽位脚本未解析为节点，无按键说明；请修正下方原文后点「从原文载入」）"
            )
            return
        if self._selected is None or self._selected >= len(self._steps):
            self._explain.config(text="")
            return
        self._explain.config(text=ms.describe_step_zh(self._steps[self._selected]))

    def _clear_left_stick(self) -> None:
        for v in self._lst_vars.values():
            v.set(False)
        self._apply_panel()

    def _clear_right_stick(self) -> None:
        for v in self._rst_vars.values():
            v.set(False)
        self._apply_panel()

    def _clear_follow_left_stick(self) -> None:
        for v in self._flst_vars.values():
            v.set(False)
        self._apply_panel()

    def _clear_follow_right_stick(self) -> None:
        for v in self._frst_vars.values():
            v.set(False)
        self._apply_panel()

    def _clear_follow_fields(self, st: ms.Step) -> None:
        st.mask_follow = 0
        st.follow_delay_ms = 0
        st.follow_left_x = st.follow_left_y = 0.0
        st.follow_right_x = st.follow_right_y = 0.0

    def _refresh_follow_sensitivity(self) -> None:
        on = bool(self._combo_var.get()) and self._selected is not None and self._selected < len(self._steps)
        st = "normal" if on else "disabled"
        for w in self._follow_sensitive:
            try:
                w.configure(state=st)
            except tk.TclError:
                pass

    def _on_two_phase_toggle(self) -> None:
        self._refresh_follow_sensitivity()
        self._apply_panel()

    def _emit(self) -> None:
        self._fallback_script = None
        if self._on_change:
            self._on_change()

    def set_script(self, script: str) -> None:
        self._fallback_script = None
        self._banner.config(text="")
        s = script.strip()
        if not s:
            self._steps = []
            self._selected = None
            self._force_resync_combo = True
            self._redraw()
            self._sync_panel()
            return
        parsed = ms.parse(s)
        if not parsed and s:
            self._fallback_script = script
            self._steps = []
            self._selected = None
            self._force_resync_combo = True
            self._banner.config(
                text="脚本无法解析为节点图；请用下方「宏原文」改正后再「从原文载入」。保存时将保留原文。"
            )
        else:
            self._steps = parsed
            self._selected = 0 if self._steps else None
            self._force_resync_combo = True
        self._redraw()
        self._sync_panel()

    def get_script(self) -> str:
        if self._fallback_script is not None:
            return self._fallback_script.strip()
        ss = list(self._steps)
        trim_tail(ss)
        return ms.serialize(ss)

    def is_fallback_mode(self) -> bool:
        return self._fallback_script is not None

    def reload_from_raw(self, raw: str) -> None:
        self.set_script(raw)

    def _box_bounds(self, lin_idx: int) -> tuple[float, float, float, float]:
        row = lin_idx // COLS
        col = lin_idx % COLS
        cx = MARGIN + col * CELL_W + CELL_W / 2
        cy = MARGIN + row * CELL_H + CELL_H / 2
        h = BOX / 2
        return cx - h, cy - h, cx + h, cy + h

    def _total_linear_slots(self) -> int:
        n = len(self._steps)
        if n < ms.MAX_STEPS:
            return n + 1
        return n

    def _canvas_size(self) -> tuple[int, int]:
        tls = max(1, self._total_linear_slots())
        rows = (tls + COLS - 1) // COLS
        w = MARGIN * 2 + COLS * CELL_W + ROUTE_PAD
        h = MARGIN * 2 + rows * CELL_H + ROUTE_PAD
        return int(w), int(h)

    def _redraw(self) -> None:
        self._canvas.delete("all")
        w, h = self._canvas_size()
        self._canvas.configure(scrollregion=(0, 0, w, h))

        self._hit_nodes: dict[int, tuple[float, float, float, float]] = {}
        self._hit_plus: tuple[float, float, float, float] | None = None

        if self._steps:
            x0, y0, x1, y1 = self._box_bounds(0)
            lx = max(4, MARGIN // 2)
            cy = (y0 + y1) / 2
            mx = (lx + x0) / 2
            self._canvas.create_line(lx, cy, x0, cy, fill="#555", width=2, arrow=tk.LAST)
            self._canvas.create_text(
                mx, cy - 14, text=f"{self._steps[0].pre_gap_ms} ms", fill="#0066aa", font=("Segoe UI", 9)
            )

        for i in range(len(self._steps) - 1):
            self._draw_edge(i, i + 1)

        for i in range(len(self._steps)):
            self._draw_node(i, i)

        if len(self._steps) < ms.MAX_STEPS:
            self._draw_plus(len(self._steps))

        self._sync_panel()

    def _draw_node(self, lin_idx: int, step_i: int) -> None:
        x0, y0, x1, y1 = self._box_bounds(lin_idx)
        st = self._steps[step_i]
        sel = self._selected == step_i
        outline = "#cc2200" if sel else "#333"
        width = 3 if sel else 1
        self._canvas.create_rectangle(x0, y0, x1, y1, outline=outline, width=width, fill="#ffffff")
        label = ms.node_label(st, step_i + 1)
        self._canvas.create_text(
            (x0 + x1) / 2, (y0 + y1) / 2, text=label, font=("Segoe UI", 8), justify=tk.CENTER
        )
        self._hit_nodes[step_i] = (x0, y0, x1, y1)

    def _draw_plus(self, lin_idx: int) -> None:
        x0, y0, x1, y1 = self._box_bounds(lin_idx)
        self._canvas.create_rectangle(x0, y0, x1, y1, outline="#888", width=2, dash=(4, 4), fill="#fafafa")
        self._canvas.create_text((x0 + x1) / 2, (y0 + y1) / 2, text="+", font=("Segoe UI", 18), fill="#666")
        self._hit_plus = (x0, y0, x1, y1)

    def _draw_edge(self, i_fr: int, i_to: int) -> None:
        xa0, ya0, xa1, ya1 = self._box_bounds(i_fr)
        xb0, yb0, xb1, yb1 = self._box_bounds(i_to)
        cx_a, cy_a = (xa0 + xa1) / 2, (ya0 + ya1) / 2
        cx_b, cy_b = (xb0 + xb1) / 2, (yb0 + yb1) / 2
        gap_ms = self._steps[i_to].pre_gap_ms
        hold_ms = self._steps[i_fr].hold_ms

        same_row = i_fr // COLS == i_to // COLS
        horiz_next = i_to % COLS == i_fr % COLS + 1
        if same_row and horiz_next:
            x1 = xa1
            x2 = xb0
            y = cy_a
            self._canvas.create_line(x1, y, x2, y, fill="#444", width=2, arrow=tk.LAST)
            mx = (x1 + x2) / 2
            self._canvas.create_text(mx, y - 14, text=f"{gap_ms} ms", fill="#0066aa", font=("Segoe UI", 9))
            self._canvas.create_text(mx, y + 14, text=f"{hold_ms} ms", fill="#884400", font=("Segoe UI", 9))
            return

        x_end = xa1 + ROUTE_PAD
        self._canvas.create_line(cx_a + BOX / 2, cy_a, x_end, cy_a, fill="#444", width=2)
        self._canvas.create_line(x_end, cy_a, x_end, cy_b, fill="#444", width=2, arrow=tk.LAST)
        self._canvas.create_line(x_end, cy_b, cx_b - BOX / 2, cy_b, fill="#444", width=2)
        bend_y = (cy_a + cy_b) / 2
        self._canvas.create_text(x_end + 8, bend_y - 16, text=f"{gap_ms} ms", fill="#0066aa", font=("Segoe UI", 9))
        self._canvas.create_text(x_end + 8, bend_y + 16, text=f"{hold_ms} ms", fill="#884400", font=("Segoe UI", 9))

    def _append_step(self) -> None:
        if len(self._steps) >= ms.MAX_STEPS:
            return
        st = ms.default_step()
        st.mask = ms.VPAD_BUTTON_A
        self._steps.append(st)
        self._selected = len(self._steps) - 1
        self._emit()
        self._redraw()

    def _on_click(self, ev: tk.Event) -> None:
        x = self._canvas.canvasx(ev.x)
        y = self._canvas.canvasy(ev.y)
        if self._hit_plus:
            x0, y0, x1, y1 = self._hit_plus
            if x0 <= x <= x1 and y0 <= y <= y1:
                self._append_step()
                return
        for idx, bb in self._hit_nodes.items():
            x0, y0, x1, y1 = bb
            if x0 <= x <= x1 and y0 <= y <= y1:
                self._selected = idx
                self._redraw()
                return

    def _sync_panel(self) -> None:
        if self._selected is None or self._selected >= len(self._steps):
            self._gap_var.set(0)
            self._hold_var.set(80)
            for v in self._btn_vars.values():
                v.set(False)
            for v in self._lst_vars.values():
                v.set(False)
            for v in self._rst_vars.values():
                v.set(False)
            self._combo_var.set(False)
            self._follow_delay_var.set(100)
            for v in self._fbtn_vars.values():
                v.set(False)
            for v in self._flst_vars.values():
                v.set(False)
            for v in self._frst_vars.values():
                v.set(False)
            self._synced_sel_idx = None
            self._refresh_follow_sensitivity()
            self._refresh_key_explain()
            return
        st = self._steps[self._selected]
        resync_combo = self._force_resync_combo or (self._selected != self._synced_sel_idx)
        if resync_combo:
            self._combo_var.set(ms.step_has_follow_phase(st))
            self._synced_sel_idx = self._selected
            self._force_resync_combo = False

        self._gap_var.set(st.pre_gap_ms)
        self._hold_var.set(st.hold_ms)
        for ch, var in self._btn_vars.items():
            var.set(bool(st.mask & _mask_bit(ch)))
        lu, ld, ll, lr = _boxes_from_axes(st.left_x, st.left_y)
        ru, rd, rl, rr = _boxes_from_axes(st.right_x, st.right_y)
        self._lst_vars["up"].set(lu)
        self._lst_vars["down"].set(ld)
        self._lst_vars["left"].set(ll)
        self._lst_vars["right"].set(lr)
        self._rst_vars["up"].set(ru)
        self._rst_vars["down"].set(rd)
        self._rst_vars["left"].set(rl)
        self._rst_vars["right"].set(rr)

        self._follow_delay_var.set(st.follow_delay_ms)
        for ch, var in self._fbtn_vars.items():
            var.set(bool(st.mask_follow & _mask_bit(ch)))
        flu, fld, fll, flr = _boxes_from_axes(st.follow_left_x, st.follow_left_y)
        fru, frd, frl, frr = _boxes_from_axes(st.follow_right_x, st.follow_right_y)
        self._flst_vars["up"].set(flu)
        self._flst_vars["down"].set(fld)
        self._flst_vars["left"].set(fll)
        self._flst_vars["right"].set(flr)
        self._frst_vars["up"].set(fru)
        self._frst_vars["down"].set(frd)
        self._frst_vars["left"].set(frl)
        self._frst_vars["right"].set(frr)

        self._refresh_follow_sensitivity()
        self._refresh_key_explain()

    def _apply_panel(self) -> None:
        if self._selected is None or self._selected >= len(self._steps):
            return
        st = self._steps[self._selected]
        try:
            st.pre_gap_ms = max(0, min(60000, int(self._gap_var.get())))
            st.hold_ms = max(1, min(5000, int(self._hold_var.get())))
        except (tk.TclError, ValueError):
            return
        mask = 0
        for ch, var in self._btn_vars.items():
            if var.get():
                mask |= _mask_bit(ch)
        st.mask = mask
        lx, ly = _axes_from_boxes(
            self._lst_vars["up"].get(),
            self._lst_vars["down"].get(),
            self._lst_vars["left"].get(),
            self._lst_vars["right"].get(),
        )
        rx, ry = _axes_from_boxes(
            self._rst_vars["up"].get(),
            self._rst_vars["down"].get(),
            self._rst_vars["left"].get(),
            self._rst_vars["right"].get(),
        )
        st.left_x, st.left_y = lx, ly
        st.right_x, st.right_y = rx, ry

        self._banner.config(text="")
        if not self._combo_var.get():
            self._clear_follow_fields(st)
        else:
            try:
                fd = max(0, min(5000, int(self._follow_delay_var.get())))
            except (tk.TclError, ValueError):
                fd = 0
            mf = 0
            for ch, var in self._fbtn_vars.items():
                if var.get():
                    mf |= _mask_bit(ch)
            flx, fly = _axes_from_boxes(
                self._flst_vars["up"].get(),
                self._flst_vars["down"].get(),
                self._flst_vars["left"].get(),
                self._flst_vars["right"].get(),
            )
            frx, fry = _axes_from_boxes(
                self._frst_vars["up"].get(),
                self._frst_vars["down"].get(),
                self._frst_vars["left"].get(),
                self._frst_vars["right"].get(),
            )
            has_second = mf != 0 or flx != 0.0 or fly != 0.0 or frx != 0.0 or fry != 0.0
            if has_second:
                st.follow_delay_ms = fd
                st.mask_follow = mf
                st.follow_left_x, st.follow_left_y = flx, fly
                st.follow_right_x, st.follow_right_y = frx, fry
            else:
                if ms.step_has_follow_phase(st):
                    self._clear_follow_fields(st)
                self._banner.config(
                    text="两段式已勾选：请至少选择第二段的一个按键或摇杆方向；仅改延迟不会生效。"
                )

        self._emit()
        self._redraw()

    def _delete_selected(self) -> None:
        if self._selected is None:
            return
        i = self._selected
        del self._steps[i]
        self._selected = min(i, len(self._steps) - 1) if self._steps else None
        self._emit()
        self._redraw()

    def _insert_after(self) -> None:
        if len(self._steps) >= ms.MAX_STEPS:
            return
        if self._selected is None:
            self._append_step()
            return
        i = self._selected + 1
        st = ms.default_step()
        st.mask = ms.VPAD_BUTTON_A
        self._steps.insert(i, st)
        self._selected = i
        self._emit()
        self._redraw()
