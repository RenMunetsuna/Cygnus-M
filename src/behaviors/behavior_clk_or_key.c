/*
 * Copyright 2026 ren-mntn
 * SPDX-License-Identifier: MIT
 *
 * Click-or-key custom behavior with scroll-mode integration.
 *
 * param1: HID keycode (encoded) to emit in typing mode
 * param2: action in mouse mode:
 *   0 = left click (LCLK)
 *   1 = right click (RCLK)
 *   2 = middle click (MCLK)
 *   0xFF = scroll trigger (momentary drag-scroll; no button press)
 *
 * Keyball44 drag-scroll behavior (mouse mode):
 *   - Scroll trigger press   -> enter scroll mode (momentary)
 *   - Scroll trigger release -> exit scroll mode (unless fixed)
 *   - LCLK press in scroll   -> hold pinch modifier for Figma pinch-zoom
 *   - RCLK press in scroll   -> fix scroll mode (locks until canceled)
 *   - (Fixed scroll is canceled by any A-Z key in typing_mode listener.)
 */

#define DT_DRV_COMPAT zmk_behavior_clk_or_key

#include <stdbool.h>

#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <drivers/behavior.h>

#include "../typing_mode.h"

#define IS_CENTRAL (IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL) || !IS_ENABLED(CONFIG_ZMK_SPLIT))

#if IS_CENTRAL
#include <zmk/behavior.h>
#include <zmk/event_manager.h>
#include <zmk/events/keycode_state_changed.h>
#include <zmk/hid.h>
#include <zmk/endpoints.h>
#include <dt-bindings/zmk/keys.h>
#include <dt-bindings/zmk/modifiers.h>
#endif

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

#define COK_BUTTON_LEFT 0
#define COK_BUTTON_RIGHT 1
#define COK_SCROLL_TRIGGER 0xFF
#define COK_TRACKED_MOUSE_BUTTONS 8

#if IS_CENTRAL
#if IS_ENABLED(CONFIG_ZMK_CLK_OR_KEY_PINCH_MODIFIER_LCTL)
#define COK_PINCH_MODIFIER MOD_LCTL
#elif IS_ENABLED(CONFIG_ZMK_CLK_OR_KEY_PINCH_MODIFIER_LSFT)
#define COK_PINCH_MODIFIER MOD_LSFT
#elif IS_ENABLED(CONFIG_ZMK_CLK_OR_KEY_PINCH_MODIFIER_LALT)
#define COK_PINCH_MODIFIER MOD_LALT
#else
#define COK_PINCH_MODIFIER MOD_LGUI
#endif

typedef enum {
    COK_PRESS_TYPING_KEY,
    COK_PRESS_SCROLL_TRIGGER,
    COK_PRESS_PINCH_MODIFIER,
    COK_PRESS_FIXED_SCROLL,
    COK_PRESS_MOUSE_BUTTON,
} cok_press_action_t;

typedef struct {
    bool pinch_modifier_pressed;
    bool mouse_button_pressed[COK_TRACKED_MOUSE_BUTTONS];
} cok_state_t;

static cok_state_t cok_state;

static bool tracked_mouse_button(uint32_t button) {
    return button < COK_TRACKED_MOUSE_BUTTONS;
}

static cok_press_action_t select_press_action(uint32_t button) {
    if (button == COK_BUTTON_LEFT && g_is_scrolling) {
        return COK_PRESS_PINCH_MODIFIER;
    }
    if (g_is_typing_mode) {
        return COK_PRESS_TYPING_KEY;
    }
    if (button == COK_SCROLL_TRIGGER) {
        return COK_PRESS_SCROLL_TRIGGER;
    }
    if (button == COK_BUTTON_RIGHT && g_is_scrolling) {
        return COK_PRESS_FIXED_SCROLL;
    }
    return COK_PRESS_MOUSE_BUTTON;
}

static void press_pinch_modifier(void) {
    if (cok_state.pinch_modifier_pressed) {
        return;
    }
    zmk_hid_register_mods(COK_PINCH_MODIFIER);
    zmk_endpoints_send_report(HID_USAGE_KEY);
    cok_state.pinch_modifier_pressed = true;
}

static void release_pinch_modifier(void) {
    if (!cok_state.pinch_modifier_pressed) {
        return;
    }
    zmk_hid_unregister_mods(COK_PINCH_MODIFIER);
    zmk_endpoints_send_report(HID_USAGE_KEY);
    cok_state.pinch_modifier_pressed = false;
}

static void press_mouse_button(uint32_t button) {
    zmk_hid_mouse_button_press(button);
    zmk_endpoints_send_mouse_report();
    if (tracked_mouse_button(button)) {
        cok_state.mouse_button_pressed[button] = true;
    }
}

static bool release_tracked_mouse_button(uint32_t button) {
    if (!tracked_mouse_button(button) || !cok_state.mouse_button_pressed[button]) {
        return false;
    }
    zmk_hid_mouse_button_release(button);
    zmk_endpoints_send_mouse_report();
    cok_state.mouse_button_pressed[button] = false;
    return true;
}
#endif

void clk_or_key_release_pinch_modifier(void) {
#if IS_CENTRAL
    release_pinch_modifier();
#endif
}

static int on_press(struct zmk_behavior_binding *binding,
                    struct zmk_behavior_binding_event event) {
#if IS_CENTRAL
    uint32_t key_encoded = binding->param1;
    uint32_t button = binding->param2;

    switch (select_press_action(button)) {
    case COK_PRESS_PINCH_MODIFIER:
        press_pinch_modifier();
        return 0;

    case COK_PRESS_TYPING_KEY:
        if (g_current_pressed_key != 0 && g_current_pressed_key != key_encoded) {
            raise_zmk_keycode_state_changed_from_encoded(g_current_pressed_key, false,
                                                         event.timestamp);
        }
        g_current_pressed_key = key_encoded;
        return raise_zmk_keycode_state_changed_from_encoded(key_encoded, true, event.timestamp);

    case COK_PRESS_SCROLL_TRIGGER:
        /* Mouse mode: record for vowel auto-complete. */
        g_last_keycode = key_encoded & 0xFF;
        g_typing_timer = event.timestamp;
        scroll_mode_set(true);
        return 0;

    case COK_PRESS_FIXED_SCROLL:
        /* Mouse mode: record for vowel auto-complete. */
        g_last_keycode = key_encoded & 0xFF;
        g_typing_timer = event.timestamp;
        scroll_mode_set_fixed(true);
        return 0;

    case COK_PRESS_MOUSE_BUTTON:
        break;
    }

    /* Mouse mode: record for vowel auto-complete. */
    g_last_keycode = key_encoded & 0xFF;
    g_typing_timer = event.timestamp;

    press_mouse_button(button);
#endif
    return 0;
}

static int on_release(struct zmk_behavior_binding *binding,
                      struct zmk_behavior_binding_event event) {
#if IS_CENTRAL
    uint32_t key_encoded = binding->param1;
    uint32_t button = binding->param2;

    bool handled = false;
    if (button == COK_BUTTON_LEFT && cok_state.pinch_modifier_pressed) {
        release_pinch_modifier();
        handled = true;
    }
    if (release_tracked_mouse_button(button)) {
        handled = true;
    }
    if (handled) {
        return 0;
    }

    if (g_is_typing_mode && g_current_pressed_key != 0) {
        if (g_current_pressed_key == key_encoded) {
            g_current_pressed_key = 0;
        }
        return raise_zmk_keycode_state_changed_from_encoded(key_encoded, false, event.timestamp);
    }

    if (button == COK_SCROLL_TRIGGER) {
        if (!g_is_fixed_scroll) {
            scroll_mode_set(false);
        } else {
            release_pinch_modifier();
        }
        return 0;
    }

    /* RCLK press-that-locked-fixed-scroll has no release action. */
    if (g_is_fixed_scroll && button == COK_BUTTON_RIGHT) {
        return 0;
    }

    zmk_hid_mouse_button_release(button);
    zmk_endpoints_send_mouse_report();
#endif
    return 0;
}

static const struct behavior_driver_api behavior_clk_or_key_driver_api = {
    .binding_pressed = on_press,
    .binding_released = on_release,
};

#define COK_INST(n)                                                                                \
    BEHAVIOR_DT_INST_DEFINE(n, NULL, NULL, NULL, NULL, POST_KERNEL,                                \
                            CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,                                   \
                            &behavior_clk_or_key_driver_api);

DT_INST_FOREACH_STATUS_OKAY(COK_INST)

#endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) */

#if !DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)
void clk_or_key_release_pinch_modifier(void) {}
#endif
