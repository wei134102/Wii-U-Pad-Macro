# SPDX-License-Identifier: MIT
"""macro_script parse/serialize — matches Wii-U-Pad-Macro firmware (macro_script.cpp)."""

from __future__ import annotations

from dataclasses import dataclass

MAX_STEPS = 32

# devkitPro wut include/vpad/input.h VPADButtons (match firmware)
VPAD_BUTTON_A = 0x8000
VPAD_BUTTON_B = 0x4000
VPAD_BUTTON_X = 0x2000
VPAD_BUTTON_Y = 0x1000
VPAD_BUTTON_LEFT = 0x0800
VPAD_BUTTON_RIGHT = 0x0400
VPAD_BUTTON_UP = 0x0200
VPAD_BUTTON_DOWN = 0x0100
VPAD_BUTTON_ZL = 0x0080
VPAD_BUTTON_ZR = 0x0040
VPAD_BUTTON_L = 0x0020
VPAD_BUTTON_R = 0x0010
VPAD_BUTTON_PLUS = 0x0008
VPAD_BUTTON_MINUS = 0x0004
VPAD_BUTTON_HOME = 0x0002

_CHAR_TO_MASK: dict[str, int] = {
    "A": VPAD_BUTTON_A,
    "B": VPAD_BUTTON_B,
    "X": VPAD_BUTTON_X,
    "Y": VPAD_BUTTON_Y,
    "L": VPAD_BUTTON_L,
    "R": VPAD_BUTTON_R,
    "1": VPAD_BUTTON_ZL,
    "2": VPAD_BUTTON_ZR,
    "P": VPAD_BUTTON_PLUS,
    "M": VPAD_BUTTON_MINUS,
    "H": VPAD_BUTTON_HOME,
    "^": VPAD_BUTTON_UP,
    "v": VPAD_BUTTON_DOWN,
    "<": VPAD_BUTTON_LEFT,
    ">": VPAD_BUTTON_RIGHT,
}


def trim_inplace(s: str) -> str:
    return s.strip()


def all_digits(t: str) -> bool:
    return bool(t) and all(c.isdigit() for c in t)


@dataclass
class Step:
    pre_gap_ms: int = 0
    mask: int = 0
    hold_ms: int = 100
    mask_follow: int = 0
    follow_delay_ms: int = 0
    follow_left_x: float = 0.0
    follow_left_y: float = 0.0
    follow_right_x: float = 0.0
    follow_right_y: float = 0.0
    left_x: float = 0.0
    left_y: float = 0.0
    right_x: float = 0.0
    right_y: float = 0.0


def step_has_input(st: Step) -> bool:
    return (
        st.mask != 0
        or st.mask_follow != 0
        or st.left_x != 0.0
        or st.left_y != 0.0
        or st.right_x != 0.0
        or st.right_y != 0.0
        or st.follow_left_x != 0.0
        or st.follow_left_y != 0.0
        or st.follow_right_x != 0.0
        or st.follow_right_y != 0.0
    )


def step_has_follow_phase(st: Step) -> bool:
    return (
        st.mask_follow != 0
        or st.follow_left_x != 0.0
        or st.follow_left_y != 0.0
        or st.follow_right_x != 0.0
        or st.follow_right_y != 0.0
    )


def apply_stick_dir(ch: str, right: bool, st: Step) -> None:
    x_name = "right_x" if right else "left_x"
    y_name = "right_y" if right else "left_y"
    x = getattr(st, x_name)
    y = getattr(st, y_name)
    if ch in "wW^":
        y = 1.0
    elif ch in "sSv":
        y = -1.0
    elif ch in "aA<":
        x = -1.0
    elif ch in "dD>":
        x = 1.0
    setattr(st, x_name, x)
    setattr(st, y_name, y)


def decode_token(keypart: str, st: Step) -> bool:
    st.mask = 0
    st.left_x = st.left_y = st.right_x = st.right_y = 0.0
    right_stick = False
    i = 0
    kp = keypart
    while i < len(kp):
        if i + 2 < len(kp) and kp[i] == "[" and kp[i + 1] == "r" and kp[i + 2] == "]":
            right_stick = True
            i += 3
            continue
        if kp[i] == "{":
            end = kp.find("}", i + 1)
            if end == -1 or end == i + 1:
                return False
            apply_stick_dir(kp[i + 1], right_stick, st)
            right_stick = False
            i = end + 1
            continue
        bit = _CHAR_TO_MASK.get(kp[i], 0)
        if bit == 0:
            return False
        st.mask |= bit
        i += 1
    return step_has_input(st)


def parse_stagger_keypart(keypart: str, st: Step) -> bool:
    i = 0
    while i < len(keypart):
        if keypart[i] != "&":
            i += 1
            continue
        j = keypart.find("&", i + 1)
        if j == -1:
            return False
        mid = keypart[i + 1 : j]
        if not all_digits(mid):
            i += 1
            continue
        k1 = trim_inplace(keypart[:i])
        k2 = trim_inplace(keypart[j + 1 :])
        if not k1 or not k2:
            return False
        if not decode_token(k1, st):
            return False
        st2 = Step()
        if not decode_token(k2, st2):
            return False
        if not step_has_input(st2):
            return False
        fd = min(5000, int(mid))
        st.mask_follow = st2.mask
        st.follow_left_x = st2.left_x
        st.follow_left_y = st2.left_y
        st.follow_right_x = st2.right_x
        st.follow_right_y = st2.right_y
        st.follow_delay_ms = fd
        return True
    return False


def parse(script: str) -> list[Step]:
    out: list[Step] = []
    pending_gap = 0
    pos = 0
    s = script
    while pos < len(s) and len(out) < MAX_STEPS:
        comma = s.find(",", pos)
        if comma == -1:
            comma = len(s)
        tok = trim_inplace(s[pos:comma])
        pos = comma + 1
        if not tok:
            continue
        if all_digits(tok):
            pending_gap = min(60000, pending_gap + int(tok))
            continue
        plus = tok.rfind("+")
        hold_ms = 120
        keypart = tok
        if plus != -1 and plus + 1 < len(tok):
            tail = tok[plus + 1 :]
            if all_digits(tail):
                keypart = trim_inplace(tok[:plus])
                hold_ms = int(tail)
                hold_ms = max(1, min(5000, hold_ms))
        st = Step()
        if "&" in keypart:
            if not parse_stagger_keypart(keypart, st):
                return []
        else:
            if not decode_token(keypart, st):
                return []
        st.pre_gap_ms = pending_gap
        st.hold_ms = hold_ms
        out.append(st)
        pending_gap = 0
    return out


def _mask_rank(bit: int) -> int:
    order = [
        VPAD_BUTTON_A,
        VPAD_BUTTON_B,
        VPAD_BUTTON_X,
        VPAD_BUTTON_Y,
        VPAD_BUTTON_L,
        VPAD_BUTTON_R,
        VPAD_BUTTON_ZL,
        VPAD_BUTTON_ZR,
        VPAD_BUTTON_PLUS,
        VPAD_BUTTON_MINUS,
        VPAD_BUTTON_HOME,
        VPAD_BUTTON_UP,
        VPAD_BUTTON_DOWN,
        VPAD_BUTTON_LEFT,
        VPAD_BUTTON_RIGHT,
    ]
    try:
        return order.index(bit)
    except ValueError:
        return 99


def _mask_char(bit: int) -> str | None:
    m = {
        VPAD_BUTTON_A: "A",
        VPAD_BUTTON_B: "B",
        VPAD_BUTTON_X: "X",
        VPAD_BUTTON_Y: "Y",
        VPAD_BUTTON_L: "L",
        VPAD_BUTTON_R: "R",
        VPAD_BUTTON_ZL: "1",
        VPAD_BUTTON_ZR: "2",
        VPAD_BUTTON_PLUS: "P",
        VPAD_BUTTON_MINUS: "M",
        VPAD_BUTTON_HOME: "H",
        VPAD_BUTTON_UP: "^",
        VPAD_BUTTON_DOWN: "v",
        VPAD_BUTTON_LEFT: "<",
        VPAD_BUTTON_RIGHT: ">",
    }
    return m.get(bit)


def encode_mask(mask: int) -> str:
    bits: list[int] = []
    m = mask
    while m != 0 and len(bits) < 16:
        low = m & -m
        bits.append(low)
        m ^= low
    bits.sort(key=_mask_rank)
    out = []
    for b in bits:
        ch = _mask_char(b)
        if ch:
            out.append(ch)
    return "".join(out)


def encode_stick(right_stick: bool, x: float, y: float) -> str:
    body = ""
    if y > 0.5:
        body += "{w}"
    if y < -0.5:
        body += "{s}"
    if x < -0.5:
        body += "{a}"
    if x > 0.5:
        body += "{d}"
    if not body:
        return ""
    if right_stick:
        return "[r]" + body
    return body


def encode_step(mask: int, lx: float, ly: float, rx: float, ry: float) -> str:
    return encode_mask(mask) + encode_stick(False, lx, ly) + encode_stick(True, rx, ry)


def _mask_bit_zh(bit: int) -> str | None:
    m = {
        VPAD_BUTTON_A: "A键",
        VPAD_BUTTON_B: "B键",
        VPAD_BUTTON_X: "X键",
        VPAD_BUTTON_Y: "Y键",
        VPAD_BUTTON_L: "L键",
        VPAD_BUTTON_R: "R键",
        VPAD_BUTTON_ZL: "ZL（脚本字符 1）",
        VPAD_BUTTON_ZR: "ZR（脚本字符 2）",
        VPAD_BUTTON_PLUS: "+（Plus，脚本 P）",
        VPAD_BUTTON_MINUS: "-（Minus，脚本 M）",
        VPAD_BUTTON_HOME: "HOME（脚本 H）",
        VPAD_BUTTON_UP: "十字键↑（脚本 ^）",
        VPAD_BUTTON_DOWN: "十字键↓（脚本 v）",
        VPAD_BUTTON_LEFT: "十字键←（脚本 <）",
        VPAD_BUTTON_RIGHT: "十字键→（脚本 >）",
    }
    return m.get(bit)


def _append_stick_zh(keys: str, x: float, y: float, is_right: bool) -> str:
    up = y > 0.5
    down = y < -0.5
    left = x < -0.5
    right = x > 0.5
    if not (up or down or left or right):
        return keys
    sep = "、" if keys else ""
    side = "右摇杆模拟" if is_right else "左摇杆模拟"
    if up and left:
        dcn = "左上"
    elif up and right:
        dcn = "右上"
    elif down and left:
        dcn = "左下"
    elif down and right:
        dcn = "右下"
    elif up:
        dcn = "上"
    elif down:
        dcn = "下"
    elif left:
        dcn = "左"
    else:
        dcn = "右"
    frag = encode_stick(is_right, x, y)
    return keys + sep + side + "：" + dcn + "（脚本 " + frag + "）"


def describe_step_zh(st: Step) -> str:
    """Chinese legend for one step — keep in sync with macro_script::describe_step_zh (firmware)."""
    bits: list[int] = []
    m = st.mask
    while m != 0 and len(bits) < 16:
        low = m & -m
        bits.append(low)
        m ^= low
    bits.sort(key=_mask_rank)

    keys = ""
    for b in bits:
        z = _mask_bit_zh(b)
        if z:
            keys += "、" if keys else ""
            keys += z

    keys = _append_stick_zh(keys, st.left_x, st.left_y, False)
    keys = _append_stick_zh(keys, st.right_x, st.right_y, True)

    if step_has_follow_phase(st):
        keys += "；随后（仍不松开第一组）追加"
        fbits: list[int] = []
        m = st.mask_follow
        while m != 0 and len(fbits) < 16:
            low = m & -m
            fbits.append(low)
            m ^= low
        fbits.sort(key=_mask_rank)
        for b in fbits:
            z = _mask_bit_zh(b)
            if z:
                keys += "、" + z
        keys = _append_stick_zh(keys, st.follow_left_x, st.follow_left_y, False)
        keys = _append_stick_zh(keys, st.follow_right_x, st.follow_right_y, True)

    timing = f"时间：执行前间隔 {st.pre_gap_ms} ms；"
    if step_has_follow_phase(st):
        timing += (
            f"先仅按住第一组 {st.follow_delay_ms} ms（不松手）；"
            f"再追加第二组后合并按住 {st.hold_ms} ms"
        )
    else:
        timing += f"按住 {st.hold_ms} ms"

    head = "【本步含义】"
    if not keys:
        head += "（仅间隔，无按键输入）" if st.pre_gap_ms > 0 else "（无按键/摇杆输入）"
    else:
        head += keys
    head += "。" + timing

    tok = encode_step(st.mask, st.left_x, st.left_y, st.right_x, st.right_y)
    if step_has_follow_phase(st):
        tok += f"&{st.follow_delay_ms}&{encode_step(st.mask_follow, st.follow_left_x, st.follow_left_y, st.follow_right_x, st.follow_right_y)}"
    if tok:
        head += f"；脚本片段 {tok}+{st.hold_ms}"
    return head


def serialize(steps: list[Step]) -> str:
    parts: list[str] = []
    for st in steps:
        if not step_has_input(st) and st.pre_gap_ms == 0:
            continue
        if st.pre_gap_ms > 0:
            parts.append(str(st.pre_gap_ms))
        if step_has_input(st):
            tok = encode_step(st.mask, st.left_x, st.left_y, st.right_x, st.right_y)
            if step_has_follow_phase(st):
                tok += f"&{st.follow_delay_ms}&{encode_step(st.mask_follow, st.follow_left_x, st.follow_left_y, st.follow_right_x, st.follow_right_y)}"
            if tok:
                parts.append(f"{tok}+{st.hold_ms}")
    return ",".join(parts)


def node_label(st: Step, index_one_based: int) -> str:
    if step_has_input(st):
        tok = encode_step(st.mask, st.left_x, st.left_y, st.right_x, st.right_y)
        if step_has_follow_phase(st):
            tok += f"&{st.follow_delay_ms}&{encode_step(st.mask_follow, st.follow_left_x, st.follow_left_y, st.follow_right_x, st.follow_right_y)}"
        short = tok[:7] + ("…" if len(tok) > 7 else "")
        return f"{index_one_based}\n{short}"
    if st.pre_gap_ms > 0:
        return f"{index_one_based}\n{st.pre_gap_ms}ms"
    return f"{index_one_based}\n—"


def default_step() -> Step:
    return Step(
        pre_gap_ms=0,
        mask=0,
        hold_ms=80,
        mask_follow=0,
        follow_delay_ms=0,
        follow_left_x=0.0,
        follow_left_y=0.0,
        follow_right_x=0.0,
        follow_right_y=0.0,
        left_x=0.0,
        left_y=0.0,
        right_x=0.0,
        right_y=0.0,
    )
