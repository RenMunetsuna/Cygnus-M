/*
 * Copyright 2026 ren-mntn
 * SPDX-License-Identifier: MIT
 *
 * OS-aware middle-click or layer toggle behavior.
 *
 * param1: layer to toggle when the active BT profile is not 0
 */

#define DT_DRV_COMPAT zmk_behavior_os_aware_mclk_or_tog

#include <stdbool.h>

#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <drivers/behavior.h>

#define IS_CENTRAL (IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL) || !IS_ENABLED(CONFIG_ZMK_SPLIT))

#if IS_CENTRAL
#include <zmk/behavior.h>
#include <zmk/endpoints.h>
#include <zmk/hid.h>
#include <zmk/keymap.h>
#if IS_ENABLED(CONFIG_ZMK_BLE)
#include <zmk/ble.h>
#endif
#endif

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

#if IS_CENTRAL
#define OS_MCLK_TOG_MAX_POSITIONS 256
#define OS_MCLK_BUTTON 2

static bool mouse_pressed[OS_MCLK_TOG_MAX_POSITIONS];

static bool is_mac_profile(void) {
#if IS_ENABLED(CONFIG_ZMK_BLE)
    return zmk_ble_active_profile_index() == 0;
#else
    return true;
#endif
}

static void press_middle_click(uint32_t position) {
    zmk_hid_mouse_button_press(OS_MCLK_BUTTON);
    zmk_endpoints_send_mouse_report();
    if (position < OS_MCLK_TOG_MAX_POSITIONS) {
        mouse_pressed[position] = true;
    }
}

static bool release_middle_click(uint32_t position) {
    if (position >= OS_MCLK_TOG_MAX_POSITIONS || !mouse_pressed[position]) {
        return false;
    }

    zmk_hid_mouse_button_release(OS_MCLK_BUTTON);
    zmk_endpoints_send_mouse_report();
    mouse_pressed[position] = false;
    return true;
}
#endif

static int on_press(struct zmk_behavior_binding *binding,
                    struct zmk_behavior_binding_event event) {
#if IS_CENTRAL
    if (is_mac_profile()) {
        press_middle_click(event.position);
        return 0;
    }

    return zmk_keymap_layer_toggle(binding->param1);
#else
    return 0;
#endif
}

static int on_release(struct zmk_behavior_binding *binding,
                      struct zmk_behavior_binding_event event) {
    ARG_UNUSED(binding);

#if IS_CENTRAL
    release_middle_click(event.position);
#else
    ARG_UNUSED(event);
#endif
    return 0;
}

static const struct behavior_driver_api behavior_os_aware_mclk_or_tog_driver_api = {
    .binding_pressed = on_press,
    .binding_released = on_release,
};

#define OS_MCLK_TOG_INST(n)                                                                        \
    BEHAVIOR_DT_INST_DEFINE(n, NULL, NULL, NULL, NULL, POST_KERNEL,                                \
                            CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,                                   \
                            &behavior_os_aware_mclk_or_tog_driver_api);

DT_INST_FOREACH_STATUS_OKAY(OS_MCLK_TOG_INST)

#endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) */
