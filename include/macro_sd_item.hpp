/*
 * Export / import SD bank buttons.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <memory>
#include <string>
#include <thread>

#include <wupsxx/button_item.hpp>


struct macro_sd_item : wups::button_item {

    const bool do_export;

    std::jthread worker{};
    std::string result_msg{};


    macro_sd_item(const std::string &label, bool export_not_import);


    static std::unique_ptr<macro_sd_item>
    create_export();

    static std::unique_ptr<macro_sd_item>
    create_import();


    void
    on_started()
        override;

    void
    on_finished()
        override;

    void
    on_cancel()
        override;
};
