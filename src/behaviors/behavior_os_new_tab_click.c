/*
 * Copyright 2026 ren-mntn
 * SPDX-License-Identifier: MIT
 *
 * OS-aware browser new-tab click behavior.
 *
 * Profile 0 (Mac): Command + left click.
 * Other profiles:  Control + left click.
 */

#define DT_DRV_COMPAT zmk_behavior_os_new_tab_click

#include <stdbool.h>

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <drivers/behavior.h>

#define IS_CENTRAL (IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL) || !IS_ENABLED(CONFIG_ZMK_SPLIT))

#if IS_CENTRAL
#include <zmk/behavior.h>
#include <zmk/endpoints.h>
#include <zmk/hid.h>
#if IS_ENABLED(CONFIG_ZMK_BLE)
#include <zmk/ble.h>
#endif
#include <dt-bindings/zmk/keys.h>
#include <dt-bindings/zmk/modifiers.h>
#endif

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

#if IS_CENTRAL
#define OS_NTC_LEFT_BUTTON 0

static zmk_mod_flags_t pick_modifier(void) {
#if IS_ENABLED(CONFIG_ZMK_BLE)
    return (zmk_ble_active_profile_index() == 0) ? MOD_LGUI : MOD_LCTL;
#else
    return MOD_LGUI;
#endif
}

static void tap_modified_left_click(zmk_mod_flags_t modifier) {
    bool already_pressed = (zmk_hid_get_explicit_mods() & modifier) != 0;

    if (!already_pressed) {
        zmk_hid_register_mods(modifier);
        zmk_endpoints_send_report(HID_USAGE_KEY);
    }

    if (CONFIG_ZMK_OS_NEW_TAB_CLICK_MOD_SETTLE_MS > 0) {
        k_sleep(K_MSEC(CONFIG_ZMK_OS_NEW_TAB_CLICK_MOD_SETTLE_MS));
    }

    zmk_hid_mouse_button_press(OS_NTC_LEFT_BUTTON);
    zmk_endpoints_send_mouse_report();

    if (CONFIG_ZMK_OS_NEW_TAB_CLICK_HOLD_MS > 0) {
        k_sleep(K_MSEC(CONFIG_ZMK_OS_NEW_TAB_CLICK_HOLD_MS));
    }

    zmk_hid_mouse_button_release(OS_NTC_LEFT_BUTTON);
    zmk_endpoints_send_mouse_report();

    if (CONFIG_ZMK_OS_NEW_TAB_CLICK_MOD_SETTLE_MS > 0) {
        k_sleep(K_MSEC(CONFIG_ZMK_OS_NEW_TAB_CLICK_MOD_SETTLE_MS));
    }

    if (!already_pressed) {
        zmk_hid_unregister_mods(modifier);
        zmk_endpoints_send_report(HID_USAGE_KEY);
    }
}
#endif

static int on_press(struct zmk_behavior_binding *binding,
                    struct zmk_behavior_binding_event event) {
    ARG_UNUSED(binding);
    ARG_UNUSED(event);

#if IS_CENTRAL
    tap_modified_left_click(pick_modifier());
#endif
    return 0;
}

static int on_release(struct zmk_behavior_binding *binding,
                      struct zmk_behavior_binding_event event) {
    ARG_UNUSED(binding);
    ARG_UNUSED(event);
    return 0;
}

static const struct behavior_driver_api behavior_os_new_tab_click_driver_api = {
    .binding_pressed = on_press,
    .binding_released = on_release,
};

#define OS_NTC_INST(n)                                                                             \
    BEHAVIOR_DT_INST_DEFINE(n, NULL, NULL, NULL, NULL, POST_KERNEL,                                \
                            CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,                                   \
                            &behavior_os_new_tab_click_driver_api);

DT_INST_FOREACH_STATUS_OKAY(OS_NTC_INST)

#endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) */
