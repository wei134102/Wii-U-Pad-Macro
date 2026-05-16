/*
 * Export / import macro bank on SD card for sharing.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <string>

namespace macro_sd {

    /** Device path prefix (Wii U FS). */
    inline constexpr const char k_dir[] = "fs:/vol/external01/wiiu/gamepad_macro";
    /** User-facing path for help text. */
    inline constexpr const char k_dir_user[] = "sd:/wiiu/gamepad_macro";
    inline constexpr const char k_bank_file[] = "fs:/vol/external01/wiiu/gamepad_macro/macros.ini";

    bool
    ensure_directory()
        noexcept;

    /** Write macros.ini and macro1.txt..macro8.txt. */
    bool
    export_all()
        noexcept;

    /** Read macros.ini (+ per-slot files). Returns false if bank missing. */
    bool
    import_all()
        noexcept;

    std::string
    last_error()
        noexcept;

} // namespace macro_sd
