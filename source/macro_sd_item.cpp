/*
 * SPDX-License-Identifier: MIT
 */

#include "i18n.hpp"
#include "macro_sd.hpp"
#include "macro_sd_item.hpp"

macro_sd_item::macro_sd_item(const std::string &label, bool export_not_import) :
    button_item{label},
    do_export{export_not_import}
{}


std::unique_ptr<macro_sd_item>
macro_sd_item::create_export()
{
    return std::make_unique<macro_sd_item>(i18n::tr(i18n::Key::ExportSd), true);
}


std::unique_ptr<macro_sd_item>
macro_sd_item::create_import()
{
    return std::make_unique<macro_sd_item>(i18n::tr(i18n::Key::ImportSd), false);
}


void
macro_sd_item::on_started()
{
    status_msg = do_export ? i18n::tr(i18n::Key::Exporting) : i18n::tr(i18n::Key::Importing);

    worker = std::jthread{[this](std::stop_token tok) {
        (void)tok;
        if (do_export) {
            result_msg = macro_sd::export_all() ? i18n::tr(i18n::Key::ExportOk)
                                                : i18n::tr(i18n::Key::ExportFail);
        } else {
            result_msg = macro_sd::import_all() ? i18n::tr(i18n::Key::ImportOk)
                                                : i18n::tr(i18n::Key::ImportFail);
        }
        current_state = state::stopped;
    }};
}


void
macro_sd_item::on_finished()
{
    if (worker.joinable()) {
        worker.join();
    }
    status_msg = result_msg;
}


void
macro_sd_item::on_cancel()
{
    if (worker.joinable()) {
        worker.request_stop();
    }
}
