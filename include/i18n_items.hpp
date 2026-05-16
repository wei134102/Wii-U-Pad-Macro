/*
 * Config items with translated labels (libwupsxx wrappers).
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <memory>
#include <string>

#include <wupsxx/bool_item.hpp>
#include <wupsxx/int_item.hpp>
#include <wupsxx/option.hpp>

namespace i18n_items {

    std::unique_ptr<wups::item>
    make_bool(wups::option<bool> &opt, const char *label);

    std::unique_ptr<wups::item>
    make_int(wups::option<int> &opt, const char *label);

    /** 0 = English, 1 = Chinese; updates i18n on change. */
    std::unique_ptr<wups::item>
    make_language(wups::option<int> &opt, const char *label);

} // namespace i18n_items
