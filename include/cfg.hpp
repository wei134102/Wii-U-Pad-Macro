/*
 * WUPS config (libwupsxx).
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <cstdint>
#include <string>

namespace cfg {

    void
    init()
        noexcept;

    std::uint32_t
    delay_ms()
        noexcept;

    void
    save_macro_slot(int slot, const std::string &value);

    std::string
    armed_macro_definition();

    /** For macro_sd import/export. */
    int
    get_post_menu_delay_ms()
        noexcept;

    int
    get_macro_slot_count()
        noexcept;

    int
    get_macro_to_run()
        noexcept;

    bool
    get_plugin_enabled()
        noexcept;

    int
    get_language()
        noexcept;

    std::string
    get_macro_slot(int slot);

    void
    set_post_menu_delay_ms(int v)
        noexcept;

    void
    set_macro_slot_count(int v)
        noexcept;

    void
    set_macro_to_run(int v)
        noexcept;

    void
    set_plugin_enabled(bool v)
        noexcept;

    void
    set_language(int v)
        noexcept;

    void
    set_macro_slot(int slot, const std::string &value);

    void
    persist_all();

} // namespace cfg
