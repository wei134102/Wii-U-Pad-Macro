/*
 * Macro script: comma tokens — number = gap (ms), else KEYS+hold_ms.
 * Buttons: A B X Y L R 1(ZL) 2(ZR) P(+) M(-) H ^ v < > (dpad).
 * Sticks: {w}{a}{s}{d} left; [r]{w}… right (WASD). Legacy {^}{v}{<}{>} ok.
 * Staggered hold (one comma step): PRIMARY&delay_ms&FOLLOW+hold_ms — hold PRIMARY
 * (and its sticks) for delay_ms without releasing, then add FOLLOW (keys + sticks),
 * then hold merged sticks (per axis: FOLLOW wins if deflected) and PRIMARY|FOLLOW
 * buttons for hold_ms, then release all.
 * describe_step_zh() matches PC_Edieted/macro_script_py.describe_step_zh — update both together.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace macro_script {

    constexpr std::size_t max_steps = 32;

    struct Step {
        std::uint32_t pre_gap_ms = 0;
        std::uint32_t mask         = 0;
        std::uint32_t hold_ms      = 100;
        /** Second-segment buttons (OR'd in after `follow_delay_ms`). */
        std::uint32_t mask_follow    = 0;
        std::uint32_t follow_delay_ms = 0;
        float follow_left_x        = 0.f;
        float follow_left_y        = 0.f;
        float follow_right_x       = 0.f;
        float follow_right_y       = 0.f;
        float left_x               = 0.f;
        float left_y               = 0.f;
        float right_x              = 0.f;
        float right_y              = 0.f;
    };

    void
    parse(const std::string &s,
          Step *out_steps,
          std::size_t *out_count)
        noexcept;

    bool
    step_has_input(const Step &st)
        noexcept;

    /** True if this step uses PRIMARY&delay&FOLLOW stagger (second segment has keys and/or sticks). */
    bool
    step_has_follow_phase(const Step &st)
        noexcept;

    std::string
    serialize(const Step *steps, std::size_t count);

    std::string
    encode_mask(std::uint32_t mask);

    std::string
    encode_stick(bool right_stick, float x, float y);

    std::string
    encode_step(std::uint32_t mask,
                float left_x,
                float left_y,
                float right_x,
                float right_y);

    /** Chinese explanation for one step (mask + sticks + timing); mirrors PC editor. */
    std::string
    describe_step_zh(const Step &st);

} // namespace macro_script
