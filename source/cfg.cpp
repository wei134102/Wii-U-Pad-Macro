/*
 * WUPS config: delays, macro slots, SD sync, i18n.
 *
 * SPDX-License-Identifier: MIT
 */

#include <algorithm>
#include <cstdio>
#include <memory>
#include <string>

#include <wupsxx/category.hpp>
#include <wupsxx/init.hpp>
#include <wupsxx/logger.hpp>
#include <wupsxx/option.hpp>
#include <wupsxx/storage.hpp>
#include <wupsxx/text_item.hpp>

#include "cfg.hpp"
#include "i18n.hpp"
#include "i18n_items.hpp"
#include "macro.hpp"
#include "macro_editor_item.hpp"
#include "macro_sd.hpp"
#include "macro_sd_item.hpp"
#include "plugin_display.hpp"

using wups::category;
using wups::make_item;

namespace logger = wups::logger;

namespace cfg {

    WUPSXX_OPTION("plugin_enabled", bool, plugin_on, true);
    WUPSXX_OPTION("language", int, language, 1, 0, 1);
    WUPSXX_OPTION("post_menu_delay_ms", int, post_menu_delay_ms, 1500, 200, 5000);
    WUPSXX_OPTION("macro_slot_count", int, macro_slot_count, 4, 1, 8);
    WUPSXX_OPTION("macro_to_run", int, macro_to_run, 1, 1, 8);

    WUPSXX_OPTION("macro_slot0", std::string, macro_slot0, "A+120,50,B+120");
    WUPSXX_OPTION("macro_slot1", std::string, macro_slot1, "");
    WUPSXX_OPTION("macro_slot2", std::string, macro_slot2, "");
    WUPSXX_OPTION("macro_slot3", std::string, macro_slot3, "");
    WUPSXX_OPTION("macro_slot4", std::string, macro_slot4, "");
    WUPSXX_OPTION("macro_slot5", std::string, macro_slot5, "");
    WUPSXX_OPTION("macro_slot6", std::string, macro_slot6, "");
    WUPSXX_OPTION("macro_slot7", std::string, macro_slot7, "");

    static std::string *
    slot_ptr(int i)
        noexcept
    {
        switch (i) {
        case 0:
            return &macro_slot0.value;
        case 1:
            return &macro_slot1.value;
        case 2:
            return &macro_slot2.value;
        case 3:
            return &macro_slot3.value;
        case 4:
            return &macro_slot4.value;
        case 5:
            return &macro_slot5.value;
        case 6:
            return &macro_slot6.value;
        case 7:
            return &macro_slot7.value;
        default:
            return nullptr;
        }
    }

    static wups::option<std::string> *
    slot_option(int i)
        noexcept
    {
        switch (i) {
        case 0:
            return &macro_slot0;
        case 1:
            return &macro_slot1;
        case 2:
            return &macro_slot2;
        case 3:
            return &macro_slot3;
        case 4:
            return &macro_slot4;
        case 5:
            return &macro_slot5;
        case 6:
            return &macro_slot6;
        case 7:
            return &macro_slot7;
        default:
            return nullptr;
        }
    }


    static void
    load_all_options()
    {
        plugin_on.load();
        language.load();
        post_menu_delay_ms.load();
        macro_slot_count.load();
        macro_to_run.load();
        for (int i = 0; i < 8; ++i) {
            wups::option<std::string> *o = slot_option(i);
            if (o != nullptr) {
                o->load();
            }
        }
        i18n::set_lang_from_int(language.value);
    }


    static void
    apply_runtime_from_options()
    {
        macro::set_enabled(plugin_on.value);
        macro::set_delay_ms(static_cast<std::uint32_t>(post_menu_delay_ms.value));
        macro::arm_program(armed_macro_definition());
    }


    void
    save_macro_slot(int slot, const std::string &value)
    {
        if (slot < 0 || slot > 7) {
            return;
        }
        wups::option<std::string> *opt = slot_option(slot);
        if (opt == nullptr) {
            return;
        }
        opt->value = value;
        opt->store();
        macro_sd::export_all();
    }


    std::string
    armed_macro_definition()
    {
        int n = macro_slot_count.value;
        if (n < 1) {
            n = 1;
        }
        if (n > 8) {
            n = 8;
        }
        int sel = macro_to_run.value;
        if (sel < 1) {
            sel = 1;
        }
        if (sel > n) {
            sel = n;
        }
        std::string *p = slot_ptr(sel - 1);
        if (p == nullptr) {
            return {};
        }
        return *p;
    }


    int
    get_post_menu_delay_ms()
        noexcept
    {
        return post_menu_delay_ms.value;
    }

    int
    get_macro_slot_count()
        noexcept
    {
        return macro_slot_count.value;
    }

    int
    get_macro_to_run()
        noexcept
    {
        return macro_to_run.value;
    }

    bool
    get_plugin_enabled()
        noexcept
    {
        return plugin_on.value;
    }

    int
    get_language()
        noexcept
    {
        return language.value;
    }

    std::string
    get_macro_slot(int slot)
    {
        std::string *p = slot_ptr(slot);
        if (p == nullptr) {
            return {};
        }
        return *p;
    }

    void
    set_post_menu_delay_ms(int v)
        noexcept
    {
        post_menu_delay_ms.value = v;
    }

    void
    set_macro_slot_count(int v)
        noexcept
    {
        macro_slot_count.value = v;
    }

    void
    set_macro_to_run(int v)
        noexcept
    {
        macro_to_run.value = v;
    }

    void
    set_plugin_enabled(bool v)
        noexcept
    {
        plugin_on.value = v;
    }

    void
    set_language(int v)
        noexcept
    {
        language.value = (v != 0) ? 1 : 0;
        i18n::set_lang_from_int(language.value);
    }

    void
    set_macro_slot(int slot, const std::string &value)
    {
        std::string *p = slot_ptr(slot);
        if (p != nullptr) {
            *p = value;
        }
    }

    void
    persist_all()
    {
        plugin_on.store();
        language.store();
        post_menu_delay_ms.store();
        macro_slot_count.store();
        macro_to_run.store();
        for (int i = 0; i < 8; ++i) {
            wups::option<std::string> *o = slot_option(i);
            if (o != nullptr) {
                o->store();
            }
        }
        wups::save();
        apply_runtime_from_options();
    }


    static void
    menu_open(category &root)
    {
        logger::initialize();
        try {
            wups::reload();
            load_all_options();
            macro_sd::import_all();
            load_all_options();
        }
        catch (const std::exception &e) {
            logger::printf("menu_open: %s\n", e.what());
        }

        category cat{i18n::tr(i18n::Key::CatTitle)};

        cat.add(i18n_items::make_language(language, i18n::tr(i18n::Key::OptLanguage)));
        cat.add(i18n_items::make_bool(plugin_on, i18n::tr(i18n::Key::OptPluginEnabled)));
        cat.add(i18n_items::make_int(post_menu_delay_ms, i18n::tr(i18n::Key::OptDelay)));
        cat.add(i18n_items::make_int(macro_slot_count, i18n::tr(i18n::Key::OptSlotCount)));
        cat.add(i18n_items::make_int(macro_to_run, i18n::tr(i18n::Key::OptMacroToRun)));

        const int nslots = std::clamp(macro_slot_count.value, 1, 8);
        for (int i = 0; i < nslots; ++i) {
            cat.add(macro_editor::make_slot_category(i));
        }

        cat.add(macro_sd_item::create_export());
        cat.add(macro_sd_item::create_import());
        cat.add(make_item(i18n::tr(i18n::Key::HelpSdPath), i18n::tr(i18n::Key::HelpSdPathBody), 50));
        cat.add(make_item(i18n::tr(i18n::Key::HelpFormat), i18n::tr(i18n::Key::HelpFormatBody), 50));
        cat.add(make_item(i18n::tr(i18n::Key::HelpEdit), i18n::tr(i18n::Key::HelpEditBody), 50));
        cat.add(make_item(i18n::tr(i18n::Key::HelpAfterClose), i18n::tr(i18n::Key::HelpAfterCloseBody), 50));

        root.add(std::move(cat));
    }


    static void
    menu_close()
    {
        logger::guard guard;
        logger::finalize();
        try {
            i18n::set_lang_from_int(language.value);
            persist_all();
            macro_sd::export_all();
        }
        catch (const std::exception &e) {
            logger::printf("menu_close save: %s\n", e.what());
        }

        if (plugin_on.value) {
            macro::arm_program(armed_macro_definition());
            macro::arm_after_menu_close();
        } else {
            macro::disarm();
        }
    }


    void
    init()
        noexcept
    {
        try {
            wups::init(plugin_display::name, menu_open, menu_close);
            load_all_options();
            apply_runtime_from_options();
        }
        catch (const std::exception &e) {
            logger::printf("cfg::init: %s\n", e.what());
        }
    }


    std::uint32_t
    delay_ms()
        noexcept
    {
        return macro::delay_ms();
    }

} // namespace cfg
