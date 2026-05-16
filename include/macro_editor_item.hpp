/*
 * WUPS menu: in-menu macro graph editor + slot actions.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <memory>
#include <string>

#include <wupsxx/category.hpp>
#include <wupsxx/item.hpp>

#include "macro_script.hpp"

namespace macro_editor {

    /** Node graph editor inside the WUPS config list (no separate OSScreen). */
    class graph_editor_item : public wups::item {

    public:
        const int slot_index;

        explicit graph_editor_item(int slot, const std::string &label);

        static std::unique_ptr<graph_editor_item>
        create(int slot, const std::string &label);

        void
        get_display(char *buf, std::size_t size)
            const override;

        void
        get_focused_display(char *buf, std::size_t size)
            const override;

        void
        on_focus_changed()
            override;

        void
        on_close()
            override;

        wups::focus_status
        on_input(const wups::simple_pad_data &input)
            override;

    private:
        void
        load_from_slot();

        void
        save_to_slot();

        macro_script::Step steps[macro_script::max_steps]{};
        int step_count = 0;
        int sel = 0;
        int cur_field = 0; // field enum in .cpp
        int key_bi = 0;
    };

    class action_item : public wups::item {

    public:
        enum class kind {
            clear,
            undo_last,
        };

        const int slot_index;
        const kind act;
        const std::string display_label;

        action_item(int slot, kind act, const std::string &label);

        static std::unique_ptr<action_item>
        create(int slot, kind act, const std::string &label);

        void
        get_display(char *buf, std::size_t size)
            const override;

        wups::focus_status
        on_input(const wups::simple_pad_data &input)
            override;
    };

    wups::category
    make_slot_category(int slot_index);

} // namespace macro_editor
