/*
 * SPDX-License-Identifier: MIT
 */

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <string>

#include <vpad/input.h>

#include "macro_script.hpp"

namespace {

    void
    trim_inplace(std::string &s)
    {
        while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front()))) {
            s.erase(0, 1);
        }
        while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back()))) {
            s.pop_back();
        }
    }

    bool
    all_digits(const std::string &t)
    {
        if (t.empty()) {
            return false;
        }
        for (char c : t) {
            if (!std::isdigit(static_cast<unsigned char>(c))) {
                return false;
            }
        }
        return true;
    }

    std::uint32_t
    char_mask(char c)
    {
        switch (c) {
        case 'A':
            return VPAD_BUTTON_A;
        case 'B':
            return VPAD_BUTTON_B;
        case 'X':
            return VPAD_BUTTON_X;
        case 'Y':
            return VPAD_BUTTON_Y;
        case 'L':
            return VPAD_BUTTON_L;
        case 'R':
            return VPAD_BUTTON_R;
        case '1':
            return VPAD_BUTTON_ZL;
        case '2':
            return VPAD_BUTTON_ZR;
        case 'P':
            return VPAD_BUTTON_PLUS;
        case 'M':
            return VPAD_BUTTON_MINUS;
        case 'H':
            return VPAD_BUTTON_HOME;
        case '^':
            return VPAD_BUTTON_UP;
        case 'v':
            return VPAD_BUTTON_DOWN;
        case '<':
            return VPAD_BUTTON_LEFT;
        case '>':
            return VPAD_BUTTON_RIGHT;
        default:
            return 0u;
        }
    }

    void
    apply_stick_dir(char dir, bool right, macro_script::Step &st)
    {
        float &x = right ? st.right_x : st.left_x;
        float &y = right ? st.right_y : st.left_y;
        switch (dir) {
        case 'w':
        case 'W':
        case '^':
            y = 1.f;
            break;
        case 's':
        case 'S':
        case 'v':
            y = -1.f;
            break;
        case 'a':
        case 'A':
        case '<':
            x = -1.f;
            break;
        case 'd':
        case 'D':
        case '>':
            x = 1.f;
            break;
        default:
            break;
        }
    }

    bool
    decode_token(const std::string &keypart, macro_script::Step &st)
        noexcept
    {
        st.mask     = 0;
        st.left_x   = 0.f;
        st.left_y   = 0.f;
        st.right_x  = 0.f;
        st.right_y  = 0.f;

        bool right_stick = false;
        std::size_t i    = 0;

        while (i < keypart.size()) {
            if (keypart[i] == '[' && i + 2 < keypart.size() && keypart[i + 1] == 'r'
                && keypart[i + 2] == ']') {
                right_stick = true;
                i += 3;
                continue;
            }

            if (keypart[i] == '{') {
                const std::size_t end = keypart.find('}', i + 1);
                if (end == std::string::npos || end == i + 1) {
                    return false;
                }
                apply_stick_dir(keypart[i + 1], right_stick, st);
                right_stick = false;
                i           = end + 1;
                continue;
            }

            const std::uint32_t bit = char_mask(keypart[i]);
            if (bit == 0u) {
                return false;
            }
            st.mask |= bit;
            ++i;
        }

        return macro_script::step_has_input(st);
    }

    bool
    parse_stagger_keypart(const std::string &keypart,
                          macro_script::Step &st)
        noexcept
    {
        for (std::size_t i = 0; i < keypart.size(); ++i) {
            if (keypart[i] != '&') {
                continue;
            }
            const std::size_t j = keypart.find('&', i + 1);
            if (j == std::string::npos) {
                return false;
            }
            std::string mid = keypart.substr(i + 1, j - i - 1);
            if (!all_digits(mid)) {
                continue;
            }
            std::string k1 = keypart.substr(0, i);
            std::string k2 = keypart.substr(j + 1);
            trim_inplace(k1);
            trim_inplace(k2);
            if (k1.empty() || k2.empty()) {
                return false;
            }
            if (!decode_token(k1, st)) {
                return false;
            }
            macro_script::Step st2{};
            if (!decode_token(k2, st2)) {
                return false;
            }
            if (!macro_script::step_has_input(st2)) {
                return false;
            }
            std::uint32_t fd = static_cast<std::uint32_t>(std::strtoul(mid.c_str(), nullptr, 10));
            if (fd > 5000u) {
                fd = 5000u;
            }
            st.mask_follow       = st2.mask;
            st.follow_left_x     = st2.left_x;
            st.follow_left_y     = st2.left_y;
            st.follow_right_x    = st2.right_x;
            st.follow_right_y    = st2.right_y;
            st.follow_delay_ms   = fd;
            return true;
        }
        return false;
    }

} // namespace


namespace macro_script {

    bool
    step_has_input(const Step &st)
        noexcept
    {
        return st.mask != 0u || st.mask_follow != 0u || st.left_x != 0.f || st.left_y != 0.f
               || st.right_x != 0.f || st.right_y != 0.f || st.follow_left_x != 0.f
               || st.follow_left_y != 0.f || st.follow_right_x != 0.f || st.follow_right_y != 0.f;
    }

    bool
    step_has_follow_phase(const Step &st)
        noexcept
    {
        return st.mask_follow != 0u || st.follow_left_x != 0.f || st.follow_left_y != 0.f
               || st.follow_right_x != 0.f || st.follow_right_y != 0.f;
    }

    void
    parse(const std::string &s,
          Step *out_steps,
          std::size_t *out_count)
        noexcept
    {
        *out_count = 0;
        if (out_steps == nullptr) {
            return;
        }

        std::uint32_t pending_gap = 0;
        std::size_t pos           = 0;

        while (pos < s.size() && *out_count < max_steps) {
            std::size_t comma = s.find(',', pos);
            if (comma == std::string::npos) {
                comma = s.size();
            }
            std::string tok = s.substr(pos, comma - pos);
            trim_inplace(tok);
            pos = comma + 1;

            if (tok.empty()) {
                continue;
            }

            if (all_digits(tok)) {
                pending_gap += static_cast<std::uint32_t>(std::strtoul(tok.c_str(), nullptr, 10));
                if (pending_gap > 60000u) {
                    pending_gap = 60000u;
                }
                continue;
            }

            const std::size_t plus = tok.rfind('+');
            std::uint32_t hold_ms  = 120;
            std::string keypart    = tok;

            if (plus != std::string::npos && plus + 1 < tok.size()) {
                const std::string tail = tok.substr(plus + 1);
                if (all_digits(tail)) {
                    keypart = tok.substr(0, plus);
                    trim_inplace(keypart);
                    hold_ms = static_cast<std::uint32_t>(std::strtoul(tail.c_str(), nullptr, 10));
                    if (hold_ms < 1u) {
                        hold_ms = 1u;
                    }
                    if (hold_ms > 5000u) {
                        hold_ms = 5000u;
                    }
                }
            }

            Step st{};
            const bool has_amp = keypart.find('&') != std::string::npos;
            if (has_amp) {
                if (!parse_stagger_keypart(keypart, st)) {
                    *out_count = 0;
                    return;
                }
            } else {
                if (!decode_token(keypart, st)) {
                    *out_count = 0;
                    return;
                }
            }

            st.pre_gap_ms       = pending_gap;
            st.hold_ms          = hold_ms;
            out_steps[*out_count] = st;
            pending_gap         = 0;
            ++(*out_count);
        }
    }

    static int
    mask_rank(std::uint32_t bit)
    {
        if (bit == VPAD_BUTTON_A) {
            return 0;
        }
        if (bit == VPAD_BUTTON_B) {
            return 1;
        }
        if (bit == VPAD_BUTTON_X) {
            return 2;
        }
        if (bit == VPAD_BUTTON_Y) {
            return 3;
        }
        if (bit == VPAD_BUTTON_L) {
            return 4;
        }
        if (bit == VPAD_BUTTON_R) {
            return 5;
        }
        if (bit == VPAD_BUTTON_ZL) {
            return 6;
        }
        if (bit == VPAD_BUTTON_ZR) {
            return 7;
        }
        if (bit == VPAD_BUTTON_PLUS) {
            return 8;
        }
        if (bit == VPAD_BUTTON_MINUS) {
            return 9;
        }
        if (bit == VPAD_BUTTON_HOME) {
            return 10;
        }
        if (bit == VPAD_BUTTON_UP) {
            return 11;
        }
        if (bit == VPAD_BUTTON_DOWN) {
            return 12;
        }
        if (bit == VPAD_BUTTON_LEFT) {
            return 13;
        }
        if (bit == VPAD_BUTTON_RIGHT) {
            return 14;
        }
        return 99;
    }

    static char
    mask_char(std::uint32_t bit)
    {
        if (bit == VPAD_BUTTON_A) {
            return 'A';
        }
        if (bit == VPAD_BUTTON_B) {
            return 'B';
        }
        if (bit == VPAD_BUTTON_X) {
            return 'X';
        }
        if (bit == VPAD_BUTTON_Y) {
            return 'Y';
        }
        if (bit == VPAD_BUTTON_L) {
            return 'L';
        }
        if (bit == VPAD_BUTTON_R) {
            return 'R';
        }
        if (bit == VPAD_BUTTON_ZL) {
            return '1';
        }
        if (bit == VPAD_BUTTON_ZR) {
            return '2';
        }
        if (bit == VPAD_BUTTON_PLUS) {
            return 'P';
        }
        if (bit == VPAD_BUTTON_MINUS) {
            return 'M';
        }
        if (bit == VPAD_BUTTON_HOME) {
            return 'H';
        }
        if (bit == VPAD_BUTTON_UP) {
            return '^';
        }
        if (bit == VPAD_BUTTON_DOWN) {
            return 'v';
        }
        if (bit == VPAD_BUTTON_LEFT) {
            return '<';
        }
        if (bit == VPAD_BUTTON_RIGHT) {
            return '>';
        }
        return '\0';
    }

    std::string
    encode_mask(std::uint32_t mask)
    {
        std::uint32_t bits[16];
        int n = 0;
        for (std::uint32_t m = mask; m != 0u && n < 16; m &= m - 1u) {
            const std::uint32_t bit = m & (0u - m);
            bits[n++] = bit;
        }
        std::sort(bits, bits + n, [](std::uint32_t a, std::uint32_t b) {
            return mask_rank(a) < mask_rank(b);
        });
        std::string out;
        for (int i = 0; i < n; ++i) {
            const char ch = mask_char(bits[i]);
            if (ch != '\0') {
                out += ch;
            }
        }
        return out;
    }

    std::string
    encode_stick(bool right_stick, float x, float y)
    {
        std::string body;
        if (y > 0.5f) {
            body += "{w}";
        }
        if (y < -0.5f) {
            body += "{s}";
        }
        if (x < -0.5f) {
            body += "{a}";
        }
        if (x > 0.5f) {
            body += "{d}";
        }
        if (body.empty()) {
            return body;
        }
        if (right_stick) {
            return std::string("[r]") + body;
        }
        return body;
    }

    std::string
    encode_step(std::uint32_t mask,
                float left_x,
                float left_y,
                float right_x,
                float right_y)
    {
        std::string out = encode_mask(mask);
        out += encode_stick(false, left_x, left_y);
        out += encode_stick(true, right_x, right_y);
        return out;
    }

    static void
    append_stick_zh(std::string &out, float x, float y, bool is_right)
        noexcept
    {
        const bool up    = y > 0.5f;
        const bool down  = y < -0.5f;
        const bool left  = x < -0.5f;
        const bool right = x > 0.5f;
        if (!up && !down && !left && !right) {
            return;
        }
        if (!out.empty()) {
            out += reinterpret_cast<const char *>(u8"、");
        }
        out += is_right ? reinterpret_cast<const char *>(u8"右摇杆模拟")
                        : reinterpret_cast<const char *>(u8"左摇杆模拟");
        out += reinterpret_cast<const char *>(u8"：");
        const char *dir = nullptr;
        if (up && left) {
            dir = reinterpret_cast<const char *>(u8"左上");
        } else if (up && right) {
            dir = reinterpret_cast<const char *>(u8"右上");
        } else if (down && left) {
            dir = reinterpret_cast<const char *>(u8"左下");
        } else if (down && right) {
            dir = reinterpret_cast<const char *>(u8"右下");
        } else if (up) {
            dir = reinterpret_cast<const char *>(u8"上");
        } else if (down) {
            dir = reinterpret_cast<const char *>(u8"下");
        } else if (left) {
            dir = reinterpret_cast<const char *>(u8"左");
        } else if (right) {
            dir = reinterpret_cast<const char *>(u8"右");
        }
        if (dir != nullptr) {
            out += dir;
        }
        out += reinterpret_cast<const char *>(u8"（脚本 ");
        out += encode_stick(is_right, x, y);
        out += reinterpret_cast<const char *>(u8"）");
    }

    static const char *
    mask_bit_zh(std::uint32_t bit)
        noexcept
    {
        switch (bit) {
        case VPAD_BUTTON_A:
            return reinterpret_cast<const char *>(u8"A键");
        case VPAD_BUTTON_B:
            return reinterpret_cast<const char *>(u8"B键");
        case VPAD_BUTTON_X:
            return reinterpret_cast<const char *>(u8"X键");
        case VPAD_BUTTON_Y:
            return reinterpret_cast<const char *>(u8"Y键");
        case VPAD_BUTTON_L:
            return reinterpret_cast<const char *>(u8"L键");
        case VPAD_BUTTON_R:
            return reinterpret_cast<const char *>(u8"R键");
        case VPAD_BUTTON_ZL:
            return reinterpret_cast<const char *>(u8"ZL（脚本字符 1）");
        case VPAD_BUTTON_ZR:
            return reinterpret_cast<const char *>(u8"ZR（脚本字符 2）");
        case VPAD_BUTTON_PLUS:
            return reinterpret_cast<const char *>(u8"+（Plus，脚本 P）");
        case VPAD_BUTTON_MINUS:
            return reinterpret_cast<const char *>(u8"-（Minus，脚本 M）");
        case VPAD_BUTTON_HOME:
            return reinterpret_cast<const char *>(u8"HOME（脚本 H）");
        case VPAD_BUTTON_UP:
            return reinterpret_cast<const char *>(u8"十字键↑（脚本 ^）");
        case VPAD_BUTTON_DOWN:
            return reinterpret_cast<const char *>(u8"十字键↓（脚本 v）");
        case VPAD_BUTTON_LEFT:
            return reinterpret_cast<const char *>(u8"十字键←（脚本 <）");
        case VPAD_BUTTON_RIGHT:
            return reinterpret_cast<const char *>(u8"十字键→（脚本 >）");
        default:
            return nullptr;
        }
    }

    std::string
    describe_step_zh(const Step &st)
    {
        auto timing_line = [&st]() {
            std::string t;
            t += reinterpret_cast<const char *>(u8"时间：执行前间隔 ");
            t += std::to_string(st.pre_gap_ms);
            t += reinterpret_cast<const char *>(u8" ms；");
            if (step_has_follow_phase(st)) {
                t += reinterpret_cast<const char *>(u8"先仅按住第一组 ");
                t += std::to_string(st.follow_delay_ms);
                t += reinterpret_cast<const char *>(u8" ms（不松手）；再追加第二组后合并按住 ");
                t += std::to_string(st.hold_ms);
                t += reinterpret_cast<const char *>(u8" ms");
            } else {
                t += reinterpret_cast<const char *>(u8"按住 ");
                t += std::to_string(st.hold_ms);
                t += reinterpret_cast<const char *>(u8" ms");
            }
            return t;
        };

        std::string keys;
        std::uint32_t bits[16];
        int n = 0;
        for (std::uint32_t m = st.mask; m != 0u && n < 16;) {
            const std::uint32_t low = m & static_cast<std::uint32_t>(0u - m);
            bits[n++] = low;
            m ^= low;
        }
        std::sort(bits, bits + n, [](std::uint32_t a, std::uint32_t b) {
            return mask_rank(a) < mask_rank(b);
        });
        for (int i = 0; i < n; ++i) {
            const char *z = mask_bit_zh(bits[i]);
            if (z != nullptr) {
                if (!keys.empty()) {
                    keys += reinterpret_cast<const char *>(u8"、");
                }
                keys += z;
            }
        }

        append_stick_zh(keys, st.left_x, st.left_y, false);
        append_stick_zh(keys, st.right_x, st.right_y, true);

        if (step_has_follow_phase(st)) {
            keys += reinterpret_cast<const char *>(u8"；随后（仍不松开第一组）追加");
            std::uint32_t fbits[16];
            int fn = 0;
            for (std::uint32_t m = st.mask_follow; m != 0u && fn < 16;) {
                const std::uint32_t low = m & static_cast<std::uint32_t>(0u - m);
                fbits[fn++] = low;
                m ^= low;
            }
            std::sort(fbits, fbits + fn, [](std::uint32_t a, std::uint32_t b) {
                return mask_rank(a) < mask_rank(b);
            });
            for (int i = 0; i < fn; ++i) {
                const char *z = mask_bit_zh(fbits[i]);
                if (z != nullptr) {
                    keys += reinterpret_cast<const char *>(u8"、");
                    keys += z;
                }
            }
            append_stick_zh(keys, st.follow_left_x, st.follow_left_y, false);
            append_stick_zh(keys, st.follow_right_x, st.follow_right_y, true);
        }

        std::string head = reinterpret_cast<const char *>(u8"【本步含义】");
        if (keys.empty()) {
            if (st.pre_gap_ms > 0u) {
                head += reinterpret_cast<const char *>(u8"（仅间隔，无按键输入）");
            } else {
                head += reinterpret_cast<const char *>(u8"（无按键/摇杆输入）");
            }
        } else {
            head += keys;
        }
        head += reinterpret_cast<const char *>(u8"。");
        head += timing_line();
        std::string tok =
            encode_step(st.mask, st.left_x, st.left_y, st.right_x, st.right_y);
        if (step_has_follow_phase(st)) {
            tok += '&';
            tok += std::to_string(st.follow_delay_ms);
            tok += '&';
            tok += encode_step(st.mask_follow,
                               st.follow_left_x,
                               st.follow_left_y,
                               st.follow_right_x,
                               st.follow_right_y);
        }
        if (!tok.empty()) {
            head += reinterpret_cast<const char *>(u8"；脚本片段 ");
            head += tok;
            head += '+';
            head += std::to_string(st.hold_ms);
        }
        return head;
    }

    std::string
    serialize(const Step *steps, std::size_t count)
    {
        std::string script;
        if (steps == nullptr) {
            return script;
        }
        for (std::size_t i = 0; i < count; ++i) {
            const Step &st = steps[i];
            if (!step_has_input(st) && st.pre_gap_ms == 0u) {
                continue;
            }
            if (st.pre_gap_ms > 0u) {
                if (!script.empty()) {
                    script += ',';
                }
                script += std::to_string(st.pre_gap_ms);
            }
            if (step_has_input(st)) {
                std::string tok =
                    encode_step(st.mask, st.left_x, st.left_y, st.right_x, st.right_y);
                if (step_has_follow_phase(st)) {
                    tok += '&';
                    tok += std::to_string(st.follow_delay_ms);
                    tok += '&';
                    tok += encode_step(st.mask_follow,
                                       st.follow_left_x,
                                       st.follow_left_y,
                                       st.follow_right_x,
                                       st.follow_right_y);
                }
                if (!tok.empty()) {
                    if (!script.empty()) {
                        script += ',';
                    }
                    script += tok;
                    script += '+';
                    script += std::to_string(st.hold_ms);
                }
            }
        }
        return script;
    }

} // namespace macro_script
