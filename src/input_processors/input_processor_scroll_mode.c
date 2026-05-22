/*
 * Copyright 2026 ren-mntn
 * SPDX-License-Identifier: MIT
 *
 * Scroll-mode input processor.
 */

#define DT_DRV_COMPAT zmk_input_processor_scroll_mode

#include <zephyr/device.h>
#include <zephyr/input/input.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include <drivers/input_processor.h>

#include "../typing_mode.h"

LOG_MODULE_REGISTER(scroll_mode, CONFIG_ZMK_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

/* Pixels of trackball motion per scroll tick. */
#define SCROLL_DIVISOR_X CONFIG_ZMK_SCROLL_MODE_DIVISOR_X
#define SCROLL_DIVISOR_Y CONFIG_ZMK_SCROLL_MODE_DIVISOR_Y

/* Keyball-style axis snap: avoid diagonal scroll by locking to the
 * dominant axis based on a configurable rolling history. */
#define SNAP_BUF_LEN CONFIG_ZMK_SCROLL_MODE_SNAP_BUFFER_LEN
#define SNAP_MASK (BIT(SNAP_BUF_LEN) - 1U)

BUILD_ASSERT(SCROLL_DIVISOR_X > 0, "CONFIG_ZMK_SCROLL_MODE_DIVISOR_X must be positive");
BUILD_ASSERT(SCROLL_DIVISOR_Y > 0, "CONFIG_ZMK_SCROLL_MODE_DIVISOR_Y must be positive");
BUILD_ASSERT(SNAP_BUF_LEN > 0 && SNAP_BUF_LEN <= 31,
             "CONFIG_ZMK_SCROLL_MODE_SNAP_BUFFER_LEN must be between 1 and 31");

typedef enum {
    SNAP_NONE,
    SNAP_X,
    SNAP_Y,
} snap_state_t;

static int32_t x_accum;
static int32_t y_accum;
static snap_state_t snap_state;
static uint32_t snap_history;
static int32_t last_abs_x_in_poll;

static int32_t iabs(int32_t v) { return v < 0 ? -v : v; }

static snap_state_t update_snap(snap_state_t current, uint32_t history) {
    uint8_t x_cnt = 0;
    for (int i = 0; i < SNAP_BUF_LEN; i++) {
        if (history & BIT(i)) x_cnt++;
    }
    if (current == SNAP_X && x_cnt <= SNAP_BUF_LEN / 2 && (history & 0x03U) == 0) {
        return SNAP_Y;
    }
    if (current == SNAP_Y && x_cnt > SNAP_BUF_LEN / 2 && (history & 0x03U) == 0x03U) {
        return SNAP_X;
    }
    if (current == SNAP_NONE) {
        return (x_cnt <= SNAP_BUF_LEN / 2) ? SNAP_Y : SNAP_X;
    }
    return current;
}

static int scroll_mode_handle_event(const struct device *dev, struct input_event *event,
                                    uint32_t param1, uint32_t param2,
                                    struct zmk_input_processor_state *state) {
    ARG_UNUSED(dev);
    ARG_UNUSED(param1);
    ARG_UNUSED(param2);
    ARG_UNUSED(state);

    if (!g_is_scrolling && !g_is_fixed_scroll) {
        x_accum = 0;
        y_accum = 0;
        snap_state = SNAP_NONE;
        snap_history = 0;
        last_abs_x_in_poll = 0;
        return ZMK_INPUT_PROC_CONTINUE;
    }

    if (event->type != INPUT_EV_REL) {
        return ZMK_INPUT_PROC_CONTINUE;
    }

    if (event->code == INPUT_REL_X) {
        /* Remember for snap update that will run on the paired REL_Y. */
        last_abs_x_in_poll = iabs(event->value);

        if (IS_ENABLED(CONFIG_ZMK_SCROLL_MODE_CLEAR_SUPPRESSED_ACCUM) &&
            snap_state == SNAP_Y) {
            x_accum = 0;
            event->value = 0;
            return ZMK_INPUT_PROC_CONTINUE;
        }

        x_accum += event->value;
        int32_t ticks = x_accum / SCROLL_DIVISOR_X;
        if (ticks != 0 && snap_state != SNAP_Y) {
            x_accum -= ticks * SCROLL_DIVISOR_X;
            event->code = INPUT_REL_HWHEEL;
            event->value = ticks;
        } else {
            event->value = 0;
        }
        return ZMK_INPUT_PROC_CONTINUE;
    }

    if (event->code == INPUT_REL_Y) {
        /* Close out this poll: update snap based on which axis dominated. */
        int32_t abs_y = iabs(event->value);
        snap_history = (snap_history << 1) & SNAP_MASK;
        if (last_abs_x_in_poll > abs_y) {
            snap_history |= 1;
        }
        last_abs_x_in_poll = 0;
        snap_state = update_snap(snap_state, snap_history);

        if (IS_ENABLED(CONFIG_ZMK_SCROLL_MODE_CLEAR_SUPPRESSED_ACCUM) &&
            snap_state == SNAP_X) {
            y_accum = 0;
            event->value = 0;
            return ZMK_INPUT_PROC_CONTINUE;
        }

        y_accum += event->value;
        int32_t ticks = y_accum / SCROLL_DIVISOR_Y;
        if (ticks != 0 && snap_state != SNAP_X) {
            y_accum -= ticks * SCROLL_DIVISOR_Y;
            event->code = INPUT_REL_WHEEL;
            /* Natural scroll: trackball down -> scroll content down = wheel up. */
            event->value = IS_ENABLED(CONFIG_ZMK_SCROLL_MODE_NATURAL_SCROLL) ? -ticks : ticks;
        } else {
            event->value = 0;
        }
        return ZMK_INPUT_PROC_CONTINUE;
    }

    return ZMK_INPUT_PROC_CONTINUE;
}

static struct zmk_input_processor_driver_api scroll_mode_driver_api = {
    .handle_event = scroll_mode_handle_event,
};

#define SM_INST(n)                                                                                 \
    DEVICE_DT_INST_DEFINE(n, NULL, NULL, NULL, NULL, POST_KERNEL,                                  \
                          CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &scroll_mode_driver_api);

DT_INST_FOREACH_STATUS_OKAY(SM_INST)

#endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) */
