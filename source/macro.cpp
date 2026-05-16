/*
 * Macro playback inside hooked VPADRead (volatile timing state, no libatomic).
 *
 * SPDX-License-Identifier: MIT
 */


#include <string>

#include <coreinit/time.h>
#include <vpad/input.h>

#include "macro.hpp"
#include "macro_script.hpp"

namespace {

    constexpr std::uint32_t default_delay_ms = 1500;

    volatile std::uint32_t g_delay_ms = default_delay_ms;

    volatile bool g_injection_suspended = false;
    volatile bool g_enabled             = true;

    macro_script::Step g_steps[macro_script::max_steps];
    volatile int g_step_count = 0;

    /** 0 = idle, 1 = waiting post-menu delay, 2 = running step machine */
    volatile int g_play = 0;
    volatile int g_step_idx = 0;
    /** 0 gap, 1 down pulse, 2 hold, 3 up pulse */
    volatile int g_sub = 0;
    volatile std::uint64_t g_phase_start = 0;
    volatile std::uint64_t g_arm_deadline = 0;

    inline std::uint64_t
    ms_to_ticks(std::uint32_t ms)
        noexcept
    {
        return OSMillisecondsToTicks(ms);
    }

    void
    apply_step_stick(VPADStatus &s, const macro_script::Step &st)
        noexcept
    {
        if (st.left_x != 0.f || st.left_y != 0.f) {
            s.leftStick.x = st.left_x;
            s.leftStick.y = st.left_y;
            if (st.left_y > 0.5f) {
                s.hold |= VPAD_STICK_L_EMULATION_UP;
            }
            if (st.left_y < -0.5f) {
                s.hold |= VPAD_STICK_L_EMULATION_DOWN;
            }
            if (st.left_x < -0.5f) {
                s.hold |= VPAD_STICK_L_EMULATION_LEFT;
            }
            if (st.left_x > 0.5f) {
                s.hold |= VPAD_STICK_L_EMULATION_RIGHT;
            }
        }

        if (st.right_x != 0.f || st.right_y != 0.f) {
            s.rightStick.x = st.right_x;
            s.rightStick.y = st.right_y;
            if (st.right_y > 0.5f) {
                s.hold |= VPAD_STICK_R_EMULATION_UP;
            }
            if (st.right_y < -0.5f) {
                s.hold |= VPAD_STICK_R_EMULATION_DOWN;
            }
            if (st.right_x < -0.5f) {
                s.hold |= VPAD_STICK_R_EMULATION_LEFT;
            }
            if (st.right_x > 0.5f) {
                s.hold |= VPAD_STICK_R_EMULATION_RIGHT;
            }
        }
    }

    void
    release_step_stick(VPADStatus &s, const macro_script::Step &st)
        noexcept
    {
        if (st.left_x != 0.f || st.left_y != 0.f) {
            s.leftStick.x = 0.f;
            s.leftStick.y = 0.f;
            s.release |= VPAD_STICK_L_EMULATION_UP | VPAD_STICK_L_EMULATION_DOWN
                         | VPAD_STICK_L_EMULATION_LEFT | VPAD_STICK_L_EMULATION_RIGHT;
        }

        if (st.right_x != 0.f || st.right_y != 0.f) {
            s.rightStick.x = 0.f;
            s.rightStick.y = 0.f;
            s.release |= VPAD_STICK_R_EMULATION_UP | VPAD_STICK_R_EMULATION_DOWN
                         | VPAD_STICK_R_EMULATION_LEFT | VPAD_STICK_R_EMULATION_RIGHT;
        }
    }

    inline float
    pick_stick(float primary, float follow)
        noexcept
    {
        if (follow > 0.5f || follow < -0.5f) {
            return follow;
        }
        return primary;
    }

    void
    apply_merged_sticks(VPADStatus &s, const macro_script::Step &st)
        noexcept
    {
        macro_script::Step tmp{};
        tmp.left_x  = pick_stick(st.left_x, st.follow_left_x);
        tmp.left_y  = pick_stick(st.left_y, st.follow_left_y);
        tmp.right_x = pick_stick(st.right_x, st.follow_right_x);
        tmp.right_y = pick_stick(st.right_y, st.follow_right_y);
        apply_step_stick(s, tmp);
    }

    void
    release_combined_sticks(VPADStatus &s, const macro_script::Step &st)
        noexcept
    {
        const bool l = st.left_x != 0.f || st.left_y != 0.f || st.follow_left_x != 0.f
                       || st.follow_left_y != 0.f;
        const bool r = st.right_x != 0.f || st.right_y != 0.f || st.follow_right_x != 0.f
                       || st.follow_right_y != 0.f;
        if (l) {
            s.leftStick.x = 0.f;
            s.leftStick.y = 0.f;
            s.release |= VPAD_STICK_L_EMULATION_UP | VPAD_STICK_L_EMULATION_DOWN
                         | VPAD_STICK_L_EMULATION_LEFT | VPAD_STICK_L_EMULATION_RIGHT;
        }
        if (r) {
            s.rightStick.x = 0.f;
            s.rightStick.y = 0.f;
            s.release |= VPAD_STICK_R_EMULATION_UP | VPAD_STICK_R_EMULATION_DOWN
                         | VPAD_STICK_R_EMULATION_LEFT | VPAD_STICK_R_EMULATION_RIGHT;
        }
    }

} // namespace


namespace macro {

    void
    set_delay_ms(std::uint32_t ms)
        noexcept
    {
        if (ms < 200u) {
            ms = 200u;
        }
        if (ms > 5000u) {
            ms = 5000u;
        }
        g_delay_ms = ms;
    }


    std::uint32_t
    delay_ms()
        noexcept
    {
        return g_delay_ms;
    }


    void
    set_enabled(bool enabled)
        noexcept
    {
        __sync_synchronize();
        g_enabled = enabled;
        __sync_synchronize();
    }


    void
    disarm()
        noexcept
    {
        __sync_synchronize();
        g_play = 0;
        __sync_synchronize();
    }


    void
    set_injection_suspended(bool suspended)
        noexcept
    {
        __sync_synchronize();
        g_injection_suspended = suspended;
        __sync_synchronize();
    }


    void
    arm_program(const char *utf8, std::size_t len)
        noexcept
    {
        std::string s;
        if (utf8 != nullptr && len > 0) {
            s.assign(utf8, utf8 + len);
        }

        macro_script::Step local[macro_script::max_steps];
        std::size_t n = 0;
        macro_script::parse(s, local, &n);

        __sync_synchronize();
        g_play       = 0;
        g_step_count = 0;
        __sync_synchronize();

        if (n == 0) {
            return;
        }

        for (std::size_t i = 0; i < n; ++i) {
            g_steps[i] = local[i];
        }

        __sync_synchronize();
        g_step_count = static_cast<int>(n);
        __sync_synchronize();
    }


    void
    arm_after_menu_close()
        noexcept
    {
        if (!g_enabled) {
            disarm();
            return;
        }
        const std::uint64_t now = OSGetTime();
        const std::uint64_t start = now + ms_to_ticks(g_delay_ms);
        __sync_synchronize();
        g_step_idx      = 0;
        g_sub           = 0;
        g_phase_start   = now;
        g_arm_deadline  = start;
        g_play          = (g_step_count > 0) ? 1 : 0;
        __sync_synchronize();
    }


    void
    on_vpad_read(VPADChan chan,
                  VPADStatus *buffers,
                  std::int32_t read_count,
                  VPADReadError *out_error)
        noexcept
    {
        if (g_injection_suspended || !g_enabled) {
            return;
        }

        if (chan != VPAD_CHAN_0 || read_count <= 0 || buffers == nullptr) {
            return;
        }

        if (out_error != nullptr && *out_error != VPAD_READ_SUCCESS) {
            return;
        }

        __sync_synchronize();
        const int play = g_play;
        const int n    = g_step_count;
        if (play == 0 || n <= 0) {
            return;
        }

        const std::uint64_t now = OSGetTime();

        if (play == 1) {
            if (now < g_arm_deadline) {
                return;
            }
            __sync_synchronize();
            g_step_idx    = 0;
            g_phase_start = now;
            g_play        = 2;
            g_sub         = (g_steps[0].pre_gap_ms == 0u) ? 1 : 0;
            __sync_synchronize();
            return;
        }

        VPADStatus &s = buffers[0];

        const int idx = g_step_idx;
        if (idx < 0 || idx >= n) {
            __sync_synchronize();
            g_play = 0;
            __sync_synchronize();
            return;
        }

        const macro_script::Step &st = g_steps[idx];
        const std::uint64_t t0       = g_phase_start;
        const std::uint64_t elapsed  = now - t0;
        const int sub                  = g_sub;

        if (sub == 0) {
            /* gap before this step */
            if (elapsed >= ms_to_ticks(st.pre_gap_ms)) {
                __sync_synchronize();
                g_phase_start = now;
                g_sub         = 1;
                __sync_synchronize();
            }
            return;
        }

        if (!macro_script::step_has_follow_phase(st)) {
            if (sub == 1) {
                s.trigger |= st.mask;
                s.hold |= st.mask;
                apply_step_stick(s, st);
                __sync_synchronize();
                g_phase_start = now;
                g_sub         = 2;
                __sync_synchronize();
                return;
            }

            if (sub == 2) {
                s.hold |= st.mask;
                apply_step_stick(s, st);
                if (elapsed >= ms_to_ticks(st.hold_ms)) {
                    __sync_synchronize();
                    g_phase_start = now;
                    g_sub         = 3;
                    __sync_synchronize();
                }
                return;
            }

            if (sub == 3) {
                s.release |= st.mask;
                s.hold &= ~st.mask;
                release_step_stick(s, st);
                __sync_synchronize();

                const int next = idx + 1;
                if (next >= n) {
                    g_play = 0;
                    __sync_synchronize();
                    return;
                }

                g_step_idx    = next;
                g_sub         = 0;
                g_phase_start = now;
                __sync_synchronize();

                if (g_steps[next].pre_gap_ms == 0u) {
                    g_sub = 1;
                }
                __sync_synchronize();
            }
            return;
        }

        /* Staggered: hold primary through delay, then OR mask_follow, then combined hold. */
        if (sub == 1) {
            s.trigger |= st.mask;
            s.hold |= st.mask;
            apply_step_stick(s, st);
            __sync_synchronize();
            g_phase_start = now;
            g_sub         = 2;
            __sync_synchronize();
            return;
        }

        if (sub == 2) {
            s.hold |= st.mask;
            apply_step_stick(s, st);
            if (elapsed >= ms_to_ticks(st.follow_delay_ms)) {
                __sync_synchronize();
                g_phase_start = now;
                g_sub         = 3;
                __sync_synchronize();
            }
            return;
        }

        if (sub == 3) {
            s.trigger |= st.mask_follow;
            s.hold |= st.mask | st.mask_follow;
            apply_merged_sticks(s, st);
            __sync_synchronize();
            g_phase_start = now;
            g_sub         = 4;
            __sync_synchronize();
            return;
        }

        if (sub == 4) {
            s.hold |= st.mask | st.mask_follow;
            apply_merged_sticks(s, st);
            if (elapsed >= ms_to_ticks(st.hold_ms)) {
                __sync_synchronize();
                g_phase_start = now;
                g_sub         = 5;
                __sync_synchronize();
            }
            return;
        }

        if (sub == 5) {
            const std::uint32_t full = st.mask | st.mask_follow;
            s.release |= full;
            s.hold &= ~full;
            release_combined_sticks(s, st);
            __sync_synchronize();

            const int next = idx + 1;
            if (next >= n) {
                g_play = 0;
                __sync_synchronize();
                return;
            }

            g_step_idx    = next;
            g_sub         = 0;
            g_phase_start = now;
            __sync_synchronize();

            if (g_steps[next].pre_gap_ms == 0u) {
                g_sub = 1;
            }
            __sync_synchronize();
        }
        return;
    }

} // namespace macro
