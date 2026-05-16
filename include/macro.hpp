/*
 * GamePad macro: merge synthetic presses into hooked VPADRead.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

#include <vpad/input.h>

namespace macro {

    void
    set_delay_ms(std::uint32_t ms)
        noexcept;

    void
    set_enabled(bool enabled)
        noexcept;

    std::uint32_t
    delay_ms()
        noexcept;

    /** While true, on_vpad_read does not modify buffers (used during OSScreen record). */
    void
    set_injection_suspended(bool suspended)
        noexcept;

    /** Load steps from UTF-8 script (comma format). Safe to call from menu thread. */
    void
    arm_program(const char *utf8, std::size_t len)
        noexcept;

    inline void
    arm_program(const std::string &s)
        noexcept
    {
        arm_program(s.data(), s.size());
    }

    /** Call from WUPS menu_close after saving options. */
    void
    arm_after_menu_close()
        noexcept;

    /** Cancel pending playback (plugin disabled). */
    void
    disarm()
        noexcept;

    void
    on_vpad_read(VPADChan chan,
                 VPADStatus *buffers,
                 std::int32_t read_count,
                 VPADReadError *out_error)
        noexcept;

} // namespace macro
