/*
 * GamePad Macro demo — WUPS plugin (Aroma).
 *
 * SPDX-License-Identifier: MIT
 */

#include <wups.h>

#include <wupsxx/logger.hpp>

#include "cfg.hpp"
#include "plugin_display.hpp"


WUPS_PLUGIN_NAME("GamePad Macro (demo)");
WUPS_PLUGIN_VERSION(PLUGIN_VERSION);
WUPS_PLUGIN_DESCRIPTION(
    "GamePad macros: SD slots, comma script, full-screen graph editor; runs after menu close.");
WUPS_PLUGIN_AUTHOR("based on hid_to_vpad / controller_patcher hook pattern");
WUPS_PLUGIN_LICENSE("MIT");

WUPS_USE_WUT_DEVOPTAB();
WUPS_USE_STORAGE("GamePad_Macro_Demo");


INITIALIZE_PLUGIN()
{
    wups::logger::set_prefix(plugin_display::name);
    wups::logger::guard guard;
    cfg::init();
}


DEINITIALIZE_PLUGIN()
{}
