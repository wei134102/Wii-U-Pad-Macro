/*
 * Replace VPADRead to merge demo macro (same idea as hid_to_vpad patcher).
 *
 * SPDX-License-Identifier: MIT
 */

#include <vpad/input.h>

#include <wups/function_patching.h>

#include "macro.hpp"

DECL_FUNCTION(int32_t, VPADRead, VPADChan chan, VPADStatus *buffers, uint32_t count, VPADReadError *outError)
{
    const int32_t r = real_VPADRead(chan, buffers, count, outError);
    macro::on_vpad_read(chan, buffers, r, outError);
    return r;
}

WUPS_MUST_REPLACE(VPADRead, WUPS_LOADER_LIBRARY_VPAD, VPADRead);
