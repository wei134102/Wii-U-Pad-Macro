/*
 * SPDX-License-Identifier: MIT
 */

#include <cstdio>
#include <optional>

#include <wupsxx/cafe_glyphs.h>
#include <wupsxx/item.hpp>
#include <wupsxx/option.hpp>

#include "i18n.hpp"
#include "i18n_items.hpp"

namespace {

    template<typename T>
    class labeled_var_item : public wups::item {

    protected:
        T &variable;
        const T default_value;
        std::optional<T> old_value;

        labeled_var_item(const std::string &label, wups::option<T> &opt) :
            item{label},
            variable{opt.value},
            default_value{opt.default_value}
        {}

        bool
        on_focus_request(bool)
            const override
        {
            return true;
        }

        void
        on_focus_changed()
            override
        {
            if (has_focus()) {
                old_value = variable;
            } else if (old_value.has_value() && variable != *old_value) {
                old_value.reset();
            }
        }

        void
        restore_default()
            override
        {
            variable = default_value;
        }

        wups::focus_status
        on_input(const wups::simple_pad_data &input)
            override
        {
            if (input.buttons_d & WUPS_CONFIG_BUTTON_B) {
                return wups::focus_status::lose;
            }
            return wups::focus_status::keep;
        }
    };

    class translated_bool_item : public labeled_var_item<bool> {

        std::string on_label;
        std::string off_label;

    public:
        translated_bool_item(const char *label, wups::option<bool> &opt) :
            labeled_var_item{label, opt},
            on_label{i18n::tr(i18n::Key::On)},
            off_label{i18n::tr(i18n::Key::Off)}
        {}

        void
        get_display(char *buf, std::size_t size)
            const override
        {
            std::snprintf(buf, size, "%s", variable ? on_label.c_str() : off_label.c_str());
        }

        void
        get_focused_display(char *buf, std::size_t size)
            const override
        {
            const char *left  = variable ? CAFE_GLYPH_BTN_LEFT : CAFE_GLYPH_BTN_DPAD;
            const char *right = variable ? CAFE_GLYPH_BTN_DPAD : CAFE_GLYPH_BTN_RIGHT;
            const char *str   = variable ? on_label.c_str() : off_label.c_str();
            std::snprintf(buf, size, "%s %s %s", left, str, right);
        }

        wups::focus_status
        on_input(const wups::simple_pad_data &input)
            override
        {
            if (variable) {
                if (input.pressed_or_long_held(WUPS_CONFIG_BUTTON_LEFT)) {
                    variable = false;
                }
            } else if (input.pressed_or_long_held(WUPS_CONFIG_BUTTON_RIGHT)) {
                variable = true;
            }
            return labeled_var_item::on_input(input);
        }
    };

    class translated_int_item : public labeled_var_item<int> {

        const int min_value;
        const int max_value;
        const int fast_step;
        const int slow_step;

    public:
        translated_int_item(const char *label, wups::option<int> &opt) :
            labeled_var_item{label, opt},
            min_value{opt.min_value},
            max_value{opt.max_value},
            fast_step{10},
            slow_step{1}
        {}

        void
        get_display(char *buf, std::size_t size)
            const override
        {
            std::snprintf(buf, size, "%d", variable);
        }

        void
        get_focused_display(char *buf, std::size_t size)
            const override
        {
            const char *slow_left  = "";
            const char *slow_right = "";
            const char *fast_left  = "";
            const char *fast_right = "";
            if (variable > min_value) {
                slow_left = CAFE_GLYPH_BTN_LEFT " ";
                fast_left = CAFE_GLYPH_BTN_L;
            }
            if (variable < max_value) {
                slow_right = " " CAFE_GLYPH_BTN_RIGHT;
                fast_right = CAFE_GLYPH_BTN_R;
            }
            std::snprintf(buf, size, "%s%s%d%s%s", fast_left, slow_left, variable, slow_right, fast_right);
        }

        wups::focus_status
        on_input(const wups::simple_pad_data &input)
            override
        {
            if (input.pressed_or_long_held(WUPS_CONFIG_BUTTON_LEFT)) {
                variable -= slow_step;
            }
            if (input.pressed_or_long_held(WUPS_CONFIG_BUTTON_RIGHT)) {
                variable += slow_step;
            }
            if (input.pressed_or_long_held(WUPS_CONFIG_BUTTON_L)) {
                variable -= fast_step;
            }
            if (input.pressed_or_long_held(WUPS_CONFIG_BUTTON_R)) {
                variable += fast_step;
            }
            if (variable < min_value) {
                variable = min_value;
            }
            if (variable > max_value) {
                variable = max_value;
            }
            return labeled_var_item::on_input(input);
        }
    };

    class language_picker_item : public labeled_var_item<int> {

    public:
        language_picker_item(const char *label, wups::option<int> &opt) :
            labeled_var_item{label, opt}
        {}

        void
        get_display(char *buf, std::size_t size)
            const override
        {
            const i18n::Lang lang = (variable != 0) ? i18n::Lang::ZH : i18n::Lang::EN;
            std::snprintf(buf, size, "%s", i18n::language_name(lang));
        }

        void
        get_focused_display(char *buf, std::size_t size)
            const override
        {
            const i18n::Lang lang = (variable != 0) ? i18n::Lang::ZH : i18n::Lang::EN;
            std::snprintf(buf, size,
                          CAFE_GLYPH_BTN_LEFT " %s " CAFE_GLYPH_BTN_RIGHT,
                          i18n::language_name(lang));
        }

        wups::focus_status
        on_input(const wups::simple_pad_data &input)
            override
        {
            if (input.pressed_or_long_held(WUPS_CONFIG_BUTTON_LEFT) && variable > 0) {
                --variable;
            }
            if (input.pressed_or_long_held(WUPS_CONFIG_BUTTON_RIGHT) && variable < 1) {
                ++variable;
            }
            i18n::set_lang_from_int(variable);
            return labeled_var_item::on_input(input);
        }
    };

} // namespace


namespace i18n_items {

    std::unique_ptr<wups::item>
    make_bool(wups::option<bool> &opt, const char *label)
    {
        return std::make_unique<translated_bool_item>(label, opt);
    }

    std::unique_ptr<wups::item>
    make_int(wups::option<int> &opt, const char *label)
    {
        return std::make_unique<translated_int_item>(label, opt);
    }

    std::unique_ptr<wups::item>
    make_language(wups::option<int> &opt, const char *label)
    {
        return std::make_unique<language_picker_item>(label, opt);
    }

} // namespace i18n_items
