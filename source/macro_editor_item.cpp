/*
 * SPDX-License-Identifier: MIT
 */

#include <algorithm>
#include <cstdio>
#include <string>

#include <vpad/input.h>
#include <wups/config.h>
#include <wupsxx/cafe_glyphs.h>
#include <wupsxx/input.hpp>

#include "cfg.hpp"
#include "i18n.hpp"
#include "macro_editor_item.hpp"
#include "macro_script.hpp"

namespace {

    void
    clear_slot(int slot)
    {
        cfg::save_macro_slot(slot, "");
    }

    void
    undo_slot(int slot)
    {
        std::string script = cfg::get_macro_slot(slot);
        if (script.empty()) {
            return;
        }
        const std::size_t pos = script.rfind(',');
        if (pos == std::string::npos) {
            script.clear();
        } else {
            script.resize(pos);
        }
        cfg::save_macro_slot(slot, script);
    }

    constexpr int k_cols = 6;

    constexpr int k_btn_rows = 3;
    constexpr int k_btn_cols = 4;

    constexpr std::uint32_t k_btn_bits[12] = {
        VPAD_BUTTON_A,
        VPAD_BUTTON_B,
        VPAD_BUTTON_X,
        VPAD_BUTTON_Y,
        VPAD_BUTTON_L,
        VPAD_BUTTON_R,
        VPAD_BUTTON_ZL,
        VPAD_BUTTON_ZR,
        VPAD_BUTTON_UP,
        VPAD_BUTTON_DOWN,
        VPAD_BUTTON_LEFT,
        VPAD_BUTTON_RIGHT,
    };

    constexpr char k_btn_chars[12] = {
        'A', 'B', 'X', 'Y', 'L', 'R', '1', '2', '^', 'v', '<', '>',
    };

    enum class field : int {
        gap = 0,
        hold,
        keys,
        left_stick,
        right_stick,
    };

    void
    stick_enum_to_xy(int e, float *x, float *y)
        noexcept
    {
        *x = 0.f;
        *y = 0.f;
        switch (e) {
        case 1:
            *y = 1.f;
            break;
        case 2:
            *x = -1.f;
            break;
        case 3:
            *y = -1.f;
            break;
        case 4:
            *x = 1.f;
            break;
        case 5:
            *y = 1.f;
            *x = -1.f;
            break;
        case 6:
            *y = 1.f;
            *x = 1.f;
            break;
        case 7:
            *y = -1.f;
            *x = -1.f;
            break;
        case 8:
            *y = -1.f;
            *x = 1.f;
            break;
        default:
            break;
        }
    }

    int
    stick_xy_to_enum(float x, float y)
        noexcept
    {
        const bool up    = y > 0.5f;
        const bool down  = y < -0.5f;
        const bool left  = x < -0.5f;
        const bool right = x > 0.5f;
        if (up && left) {
            return 5;
        }
        if (up && right) {
            return 6;
        }
        if (down && left) {
            return 7;
        }
        if (down && right) {
            return 8;
        }
        if (up) {
            return 1;
        }
        if (left) {
            return 2;
        }
        if (down) {
            return 3;
        }
        if (right) {
            return 4;
        }
        return 0;
    }

    const char *
    stick_label(int e)
        noexcept
    {
        switch (e) {
        case 0:
            return "-";
        case 1:
            return "W";
        case 2:
            return "A";
        case 3:
            return "S";
        case 4:
            return "D";
        case 5:
            return "WA";
        case 6:
            return "WD";
        case 7:
            return "SA";
        case 8:
            return "SD";
        default:
            return "?";
        }
    }

    void
    set_stick_enum(macro_script::Step &st, bool right, int e)
    {
        float x = 0.f;
        float y = 0.f;
        stick_enum_to_xy(e, &x, &y);
        if (right) {
            st.right_x = x;
            st.right_y = y;
        } else {
            st.left_x = x;
            st.left_y = y;
        }
    }

    int
    get_stick_enum(const macro_script::Step &st, bool right)
        noexcept
    {
        return stick_xy_to_enum(right ? st.right_x : st.left_x, right ? st.right_y : st.left_y);
    }

    int
    btn_index_from_cell(int br, int bc)
        noexcept
    {
        return br * k_btn_cols + bc;
    }

    void
    cell_from_btn_index(int bi, int *br, int *bc)
        noexcept
    {
        *br = bi / k_btn_cols;
        *bc = bi % k_btn_cols;
    }

    bool
    step_meaningful(const macro_script::Step &st)
        noexcept
    {
        return macro_script::step_has_input(st) || st.pre_gap_ms > 0u;
    }

    int
    max_sel_index(int step_count)
        noexcept
    {
        if (step_count >= static_cast<int>(macro_script::max_steps)) {
            return step_count - 1;
        }
        return step_count;
    }

    macro_script::Step *
    mut_step(macro_script::Step *steps, int step_count, int sel)
        noexcept
    {
        if (sel < 0 || sel >= static_cast<int>(macro_script::max_steps)) {
            return nullptr;
        }
        if (sel < step_count) {
            return &steps[sel];
        }
        if (sel == step_count && step_count < static_cast<int>(macro_script::max_steps)) {
            return &steps[step_count];
        }
        return nullptr;
    }

    const macro_script::Step *
    ro_step(const macro_script::Step *steps, int step_count, int sel)
        noexcept
    {
        if (sel < 0 || sel >= static_cast<int>(macro_script::max_steps)) {
            return nullptr;
        }
        if (sel < step_count) {
            return &steps[sel];
        }
        if (sel == step_count && step_count < static_cast<int>(macro_script::max_steps)) {
            return &steps[step_count];
        }
        return nullptr;
    }

    void
    trim_tail(macro_script::Step *steps, int *step_count)
    {
        while (*step_count > 0 && !step_meaningful(steps[*step_count - 1])) {
            --(*step_count);
        }
    }

    void
    shift_right(macro_script::Step *steps, int step_count, int pos)
    {
        for (int i = step_count; i > pos; --i) {
            steps[i] = steps[i - 1];
        }
    }

    void
    insert_after(macro_script::Step *steps, int *step_count, int *sel)
    {
        if (*step_count >= static_cast<int>(macro_script::max_steps)) {
            return;
        }
        const int pos = *sel + 1;
        if (pos >= *step_count) {
            steps[*step_count]          = macro_script::Step{};
            steps[*step_count].hold_ms = 80;
            ++(*step_count);
            *sel = *step_count - 1;
            return;
        }
        shift_right(steps, *step_count, pos);
        steps[pos]          = macro_script::Step{};
        steps[pos].hold_ms = 80;
        ++(*step_count);
        *sel = pos;
    }

    void
    erase_at(macro_script::Step *steps, int *step_count, int *sel)
    {
        if (*sel < 0 || *sel >= *step_count) {
            return;
        }
        for (int i = *sel; i < *step_count - 1; ++i) {
            steps[i] = steps[i + 1];
        }
        steps[*step_count - 1] = macro_script::Step{};
        --(*step_count);
        if (*sel >= *step_count) {
            *sel = std::max(0, *step_count - 1);
        }
    }

    void
    fmt_node(char *buf, std::size_t sz, const macro_script::Step &st, int idx, bool is_new)
    {
        if (is_new) {
            std::snprintf(buf, sz, "+");
            return;
        }
        if (macro_script::step_has_input(st)) {
            std::string tok =
                macro_script::encode_step(st.mask, st.left_x, st.left_y, st.right_x, st.right_y);
            if (macro_script::step_has_follow_phase(st)) {
                tok.push_back('&');
                tok += std::to_string(static_cast<unsigned>(st.follow_delay_ms));
                tok.push_back('&');
                tok += macro_script::encode_step(st.mask_follow,
                                                st.follow_left_x,
                                                st.follow_left_y,
                                                st.follow_right_x,
                                                st.follow_right_y);
            }
            std::snprintf(buf, sz, "%d:%.5s", idx + 1, tok.c_str());
        } else if (st.pre_gap_ms > 0u) {
            std::snprintf(buf, sz, "%d:%u", idx + 1, static_cast<unsigned>(st.pre_gap_ms));
        } else {
            std::snprintf(buf, sz, "%d:-", idx + 1);
        }
    }

} // namespace


namespace macro_editor {

    graph_editor_item::graph_editor_item(int slot, const std::string &label) :
        item{label},
        slot_index{slot}
    {}

    std::unique_ptr<graph_editor_item>
    graph_editor_item::create(int slot, const std::string &label)
    {
        return std::make_unique<graph_editor_item>(slot, label);
    }

    void
    graph_editor_item::load_from_slot()
    {
        step_count = 0;
        sel        = 0;
        cur_field  = static_cast<int>(field::gap);
        key_bi = 0;
        std::size_t n = 0;
        macro_script::parse(cfg::get_macro_slot(slot_index), steps, &n);
        step_count = static_cast<int>(std::min(n, macro_script::max_steps));
        sel = std::clamp(sel, 0, std::max(0, max_sel_index(step_count)));
    }

    void
    graph_editor_item::save_to_slot()
    {
        trim_tail(steps, &step_count);
        cfg::save_macro_slot(
            slot_index,
            macro_script::serialize(steps, static_cast<std::size_t>(std::max(0, step_count))));
    }

    void
    graph_editor_item::on_focus_changed()
    {
        if (has_focus()) {
            load_from_slot();
        } else {
            save_to_slot();
        }
    }

    void
    graph_editor_item::on_close()
    {
        if (has_focus()) {
            save_to_slot();
        }
    }

    void
    graph_editor_item::get_display(char *buf, std::size_t size)
        const
    {
        if (size == 0) {
            return;
        }
        std::snprintf(buf,
                      size,
                      CAFE_GLYPH_BTN_A " %s",
                      i18n::tr(i18n::Key::MacroGraphUnfocused));
    }

    void
    graph_editor_item::get_focused_display(char *buf, std::size_t size)
        const
    {
        if (size == 0) {
            return;
        }
        const int ms = max_sel_index(step_count);
        char nodes[96];
        nodes[0] = '\0';
        int p    = 0;
        const int row0 = (sel / k_cols) * k_cols;
        for (int i = row0; i <= std::min(row0 + k_cols - 1, ms) && p < static_cast<int>(sizeof nodes) - 8;
             ++i) {
            const bool is_new = i >= step_count;
            macro_script::Step tmp{};
            const macro_script::Step &st = is_new ? tmp : steps[i];
            char nb[20];
            fmt_node(nb, sizeof nb, st, i, is_new);
            p += std::snprintf(nodes + p,
                               sizeof nodes - static_cast<std::size_t>(p),
                               "%s%s",
                               (i > row0) ? ">" : "",
                               nb);
        }

        const macro_script::Step *st = ro_step(steps, step_count, sel);
        const char *fn[] = {
            i18n::tr(i18n::Key::EditGap),
            i18n::tr(i18n::Key::EditHold),
            i18n::tr(i18n::Key::EditKeys),
            i18n::tr(i18n::Key::EditLStick),
            i18n::tr(i18n::Key::EditRStick),
        };
        const int fi = std::clamp(cur_field, 0, 4);

        if (st != nullptr) {
            char kpick[40] = {};
            int kp         = 0;
            for (int i = 0; i < 12 && kp < static_cast<int>(sizeof kpick) - 4; ++i) {
                const bool on = (st->mask & k_btn_bits[i]) != 0u;
                const bool cur = (i == key_bi);
                kp += std::snprintf(kpick + kp,
                                    sizeof kpick - static_cast<std::size_t>(kp),
                                    "%c%c%c",
                                    cur ? '[' : (on ? '*' : '.'),
                                    k_btn_chars[i],
                                    cur ? ']' : (on ? '*' : '.'));
            }
            std::snprintf(buf,
                          size,
                          "[%s] #%d/%d %s%u %s%u |%s| L%s R%s |%s %s| %s",
                          nodes,
                          sel + 1,
                          ms + 1,
                          i18n::tr(i18n::Key::EditGap),
                          static_cast<unsigned>(st->pre_gap_ms),
                          i18n::tr(i18n::Key::EditHold),
                          static_cast<unsigned>(st->hold_ms),
                          kpick,
                          stick_label(get_stick_enum(*st, false)),
                          stick_label(get_stick_enum(*st, true)),
                          fn[fi],
                          i18n::tr(i18n::Key::MacroOsNextField),
                          i18n::tr(i18n::Key::MacroOsHelp));
        } else {
            std::snprintf(buf, size, "%s", i18n::tr(i18n::Key::MacroOsHelp));
        }
    }

    wups::focus_status
    graph_editor_item::on_input(const wups::simple_pad_data &input)
    {
        wups::simple_pad_data in{input};

        if (in.buttons_d & WUPS_CONFIG_BUTTON_B) {
            trim_tail(steps, &step_count);
            save_to_slot();
            return wups::focus_status::lose;
        }

        const std::uint32_t trg = in.buttons_d;
        const std::uint32_t hld = in.buttons_h;

        const bool zl_nav = (hld & WUPS_CONFIG_BUTTON_ZL) != 0u;
        const int ms      = max_sel_index(step_count);

        if (zl_nav) {
            if (trg & WUPS_CONFIG_BUTTON_LEFT) {
                sel = std::max(0, sel - 1);
            }
            if (trg & WUPS_CONFIG_BUTTON_RIGHT) {
                sel = std::min(ms, sel + 1);
            }
            if (trg & WUPS_CONFIG_BUTTON_UP) {
                sel = std::max(0, sel - k_cols);
            }
            if (trg & WUPS_CONFIG_BUTTON_DOWN) {
                sel = std::min(ms, sel + k_cols);
            }
        } else {
            macro_script::Step *wst = mut_step(steps, step_count, sel);
            if (wst != nullptr) {
                const int step_sz =
                    ((hld & WUPS_CONFIG_BUTTON_R) != 0u) ? 10 : 1;
                const auto f = static_cast<field>(cur_field);

                if (f == field::gap) {
                    if (in.pressed_or_long_held(WUPS_CONFIG_BUTTON_L)) {
                        wst->pre_gap_ms =
                            (wst->pre_gap_ms >= static_cast<std::uint32_t>(step_sz))
                                ? wst->pre_gap_ms - static_cast<std::uint32_t>(step_sz)
                                : 0u;
                    }
                    if (in.pressed_or_long_held(WUPS_CONFIG_BUTTON_R)) {
                        wst->pre_gap_ms =
                            std::min(wst->pre_gap_ms + static_cast<std::uint32_t>(step_sz), 5000u);
                    }
                } else if (f == field::hold) {
                    if (in.pressed_or_long_held(WUPS_CONFIG_BUTTON_L)) {
                        wst->hold_ms = (wst->hold_ms > static_cast<std::uint32_t>(step_sz))
                                           ? wst->hold_ms - static_cast<std::uint32_t>(step_sz)
                                           : 1u;
                    }
                    if (in.pressed_or_long_held(WUPS_CONFIG_BUTTON_R)) {
                        wst->hold_ms =
                            std::min(wst->hold_ms + static_cast<std::uint32_t>(step_sz), 5000u);
                    }
                } else if (f == field::keys) {
                    int br = 0;
                    int bc = 0;
                    cell_from_btn_index(key_bi, &br, &bc);
                    if (trg & WUPS_CONFIG_BUTTON_UP) {
                        br = (br + k_btn_rows - 1) % k_btn_rows;
                    }
                    if (trg & WUPS_CONFIG_BUTTON_DOWN) {
                        br = (br + 1) % k_btn_rows;
                    }
                    if (trg & WUPS_CONFIG_BUTTON_LEFT) {
                        bc = (bc + k_btn_cols - 1) % k_btn_cols;
                    }
                    if (trg & WUPS_CONFIG_BUTTON_RIGHT) {
                        bc = (bc + 1) % k_btn_cols;
                    }
                    key_bi = btn_index_from_cell(br, bc);
                    if (trg & WUPS_CONFIG_BUTTON_A) {
                        wst->mask ^= k_btn_bits[key_bi];
                        wst->mask_follow      = 0u;
                        wst->follow_delay_ms  = 0u;
                        wst->follow_left_x    = 0.f;
                        wst->follow_left_y    = 0.f;
                        wst->follow_right_x   = 0.f;
                        wst->follow_right_y   = 0.f;
                    }
                } else if (f == field::left_stick) {
                    int e = get_stick_enum(*wst, false);
                    if (trg & (WUPS_CONFIG_BUTTON_UP | WUPS_CONFIG_BUTTON_RIGHT)) {
                        e = (e + 1) % 9;
                    }
                    if (trg & (WUPS_CONFIG_BUTTON_DOWN | WUPS_CONFIG_BUTTON_LEFT)) {
                        e = (e + 8) % 9;
                    }
                    set_stick_enum(*wst, false, e);
                    wst->mask_follow      = 0u;
                    wst->follow_delay_ms  = 0u;
                    wst->follow_left_x    = 0.f;
                    wst->follow_left_y    = 0.f;
                    wst->follow_right_x   = 0.f;
                    wst->follow_right_y   = 0.f;
                } else if (f == field::right_stick) {
                    int e = get_stick_enum(*wst, true);
                    if (trg & (WUPS_CONFIG_BUTTON_UP | WUPS_CONFIG_BUTTON_RIGHT)) {
                        e = (e + 1) % 9;
                    }
                    if (trg & (WUPS_CONFIG_BUTTON_DOWN | WUPS_CONFIG_BUTTON_LEFT)) {
                        e = (e + 8) % 9;
                    }
                    set_stick_enum(*wst, true, e);
                    wst->mask_follow      = 0u;
                    wst->follow_delay_ms  = 0u;
                    wst->follow_left_x    = 0.f;
                    wst->follow_left_y    = 0.f;
                    wst->follow_right_x   = 0.f;
                    wst->follow_right_y   = 0.f;
                }

                if ((trg & WUPS_CONFIG_BUTTON_ZR) != 0u) {
                    cur_field = (cur_field + 1) % 5;
                }

                if ((trg & WUPS_CONFIG_BUTTON_PLUS) != 0u) {
                    if (sel == step_count && step_count < static_cast<int>(macro_script::max_steps)) {
                        if (step_meaningful(*wst)) {
                            ++step_count;
                            steps[step_count] = macro_script::Step{};
                            sel               = step_count;
                        }
                    } else if (sel < step_count
                               && step_count < static_cast<int>(macro_script::max_steps)) {
                        insert_after(steps, &step_count, &sel);
                    }
                }
                if ((trg & WUPS_CONFIG_BUTTON_MINUS) != 0u) {
                    if (sel < step_count && step_count > 0) {
                        erase_at(steps, &step_count, &sel);
                    } else if (sel == step_count) {
                        *wst = macro_script::Step{};
                    }
                }
            }
        }

        return wups::focus_status::keep;
    }

    action_item::action_item(int slot, kind act, const std::string &label) :
        item{label},
        slot_index{slot},
        act{act},
        display_label{label}
    {}

    std::unique_ptr<action_item>
    action_item::create(int slot, kind act, const std::string &label)
    {
        return std::make_unique<action_item>(slot, act, label);
    }

    void
    action_item::get_display(char *buf, std::size_t size)
        const
    {
        std::snprintf(buf, size, CAFE_GLYPH_BTN_A " %s", display_label.c_str());
    }

    wups::focus_status
    action_item::on_input(const wups::simple_pad_data &input)
    {
        if (input.buttons_d & WUPS_CONFIG_BUTTON_A) {
            switch (act) {
            case kind::clear:
                clear_slot(slot_index);
                break;
            case kind::undo_last:
                undo_slot(slot_index);
                break;
            }
            return wups::focus_status::lose;
        }
        if (input.buttons_d & WUPS_CONFIG_BUTTON_B) {
            return wups::focus_status::lose;
        }
        return wups::focus_status::keep;
    }

    wups::category
    make_slot_category(int slot_index)
    {
        char label[64];
        std::snprintf(label, sizeof label, i18n::tr(i18n::Key::MacroEditCat), slot_index + 1);
        wups::category cat{label};

        std::snprintf(label, sizeof label, i18n::tr(i18n::Key::MacroTree), slot_index + 1);
        cat.add(graph_editor_item::create(slot_index, label));

        std::snprintf(label, sizeof label, i18n::tr(i18n::Key::MacroClear), slot_index + 1);
        cat.add(action_item::create(slot_index, action_item::kind::clear, label));

        std::snprintf(label, sizeof label, i18n::tr(i18n::Key::MacroUndo), slot_index + 1);
        cat.add(action_item::create(slot_index, action_item::kind::undo_last, label));

        return cat;
    }

} // namespace macro_editor
