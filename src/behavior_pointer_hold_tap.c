// SPDX-License-Identifier: MIT
// A small, self-contained alternative to ZMK's hold-tap for pointer buttons.

#define DT_DRV_COMPAT modula_behavior_pointer_hold_tap

#include <zephyr/device.h>
#include <zephyr/input/input.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/dt-bindings/input/input-event-codes.h>

#include <drivers/behavior.h>
#include <zmk/behavior.h>
#include <zmk/event_manager.h>
#include <zmk/events/position_state_changed.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

enum pht_flavor {
    PHT_HOLD_PREFERRED,
    PHT_BALANCED,
    PHT_TAP_PREFERRED,
    PHT_TAP_UNLESS_INTERRUPTED,
};

enum pht_status { PHT_UNUSED, PHT_UNDECIDED, PHT_TAP, PHT_HOLD };

struct pht_config {
    int tapping_term_ms;
    int quick_tap_ms;
    int require_prior_idle_ms;
    enum pht_flavor flavor;
    bool hold_while_undecided;
    const char *hold_behavior_dev;
    const char *tap_behavior_dev;
};

struct pht_active {
    int32_t position;
    uint32_t hold_param;
    uint32_t tap_param;
    int64_t timestamp;
    enum pht_status status;
    bool hold_pressed;
    const struct pht_config *config;
    struct k_work_delayable timer;
};

static struct pht_active active[CONFIG_MODULA_BEHAVIOR_POINTER_HOLD_TAP_MAX_HELD];
static int32_t last_tapped_position = INT32_MIN;
static int64_t last_tapped_timestamp = INT32_MIN;

static struct pht_active *find_active(int32_t position) {
    for (size_t i = 0; i < ARRAY_SIZE(active); i++) {
        if (active[i].status != PHT_UNUSED && active[i].position == position) {
            return &active[i];
        }
    }
    return NULL;
}

static int invoke(const struct pht_active *pht, bool tap, bool pressed) {
    struct zmk_behavior_binding binding = {
        .behavior_dev = tap ? pht->config->tap_behavior_dev : pht->config->hold_behavior_dev,
        .param1 = tap ? pht->tap_param : pht->hold_param,
    };
    struct zmk_behavior_binding_event event = {
        .position = pht->position,
        .timestamp = pht->timestamp,
    };
    return zmk_behavior_invoke_binding(&binding, event, pressed);
}

static void decide(struct pht_active *pht, enum pht_status status) {
    if (pht == NULL || pht->status != PHT_UNDECIDED) {
        return;
    }
    pht->status = status;
    if (status == PHT_TAP) {
        if (pht->hold_pressed) {
            invoke(pht, false, false);
            pht->hold_pressed = false;
        }
        last_tapped_position = pht->position;
        last_tapped_timestamp = pht->timestamp;
        invoke(pht, true, true);
    } else if (!pht->hold_pressed) {
        invoke(pht, false, true);
        pht->hold_pressed = true;
    }
}

static void timer_handler(struct k_work *work) {
    struct k_work_delayable *delayable = k_work_delayable_from_work(work);
    struct pht_active *pht = CONTAINER_OF(delayable, struct pht_active, timer);
    decide(pht, pht->config->flavor == PHT_TAP_UNLESS_INTERRUPTED ? PHT_TAP : PHT_HOLD);
}

static bool quick_tap(const struct pht_active *pht) {
    if (last_tapped_timestamp + pht->config->require_prior_idle_ms > pht->timestamp) {
        return true;
    }
    return last_tapped_position == pht->position &&
           last_tapped_timestamp + pht->config->quick_tap_ms > pht->timestamp;
}

static int pht_pressed(struct zmk_behavior_binding *binding,
                       struct zmk_behavior_binding_event event) {
    const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);
    const struct pht_config *config = dev->config;
    struct pht_active *pht = NULL;

    for (size_t i = 0; i < ARRAY_SIZE(active); i++) {
        if (active[i].status == PHT_UNUSED) {
            pht = &active[i];
            break;
        }
    }
    if (pht == NULL) {
        LOG_ERR("No free pointer hold-tap slots");
        return ZMK_BEHAVIOR_OPAQUE;
    }

    pht->position = event.position;
    pht->hold_param = binding->param1;
    pht->tap_param = binding->param2;
    pht->timestamp = event.timestamp;
    pht->status = PHT_UNDECIDED;
    pht->hold_pressed = false;
    pht->config = config;
    if (quick_tap(pht)) {
        decide(pht, PHT_TAP);
    } else {
        if (config->hold_while_undecided) {
            invoke(pht, false, true);
            pht->hold_pressed = true;
        }
        int32_t left = MAX(0, (int32_t)(event.timestamp + config->tapping_term_ms - k_uptime_get()));
        k_work_schedule(&pht->timer, K_MSEC(left));
    }
    return ZMK_BEHAVIOR_OPAQUE;
}

static int pht_released(struct zmk_behavior_binding *binding,
                        struct zmk_behavior_binding_event event) {
    struct pht_active *pht = find_active(event.position);
    if (pht == NULL) {
        return ZMK_BEHAVIOR_OPAQUE;
    }
    k_work_cancel_delayable(&pht->timer);
    if (pht->status == PHT_UNDECIDED) {
        decide(pht, PHT_TAP);
    }
    invoke(pht, pht->status == PHT_TAP, false);
    pht->status = PHT_UNUSED;
    pht->hold_pressed = false;
    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api pht_api = {
    .binding_pressed = pht_pressed,
    .binding_released = pht_released,
};

static int position_listener(const zmk_event_t *eh) {
    struct zmk_position_state_changed *event = as_zmk_position_state_changed(eh);
    if (event == NULL) {
        return ZMK_EV_EVENT_BUBBLE;
    }
    for (size_t i = 0; i < ARRAY_SIZE(active); i++) {
        struct pht_active *pht = &active[i];
        if (pht->status != PHT_UNDECIDED || pht->position == event->position) {
            continue;
        }
        if ((event->state && (pht->config->flavor == PHT_HOLD_PREFERRED ||
                              pht->config->flavor == PHT_TAP_UNLESS_INTERRUPTED)) ||
            (!event->state && pht->config->flavor == PHT_BALANCED)) {
            decide(pht, PHT_HOLD);
        }
    }
    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(pointer_hold_tap, position_listener);
ZMK_SUBSCRIPTION(pointer_hold_tap, zmk_position_state_changed);

static void pointer_motion_listener(struct input_event *event) {
    if (event->type != INPUT_EV_REL || event->value == 0 ||
        (event->code != INPUT_REL_X && event->code != INPUT_REL_Y)) {
        return;
    }
    for (size_t i = 0; i < ARRAY_SIZE(active); i++) {
        decide(&active[i], PHT_TAP);
    }
}

INPUT_CALLBACK_DEFINE(NULL, pointer_motion_listener);

static int pht_init(const struct device *dev) {
    ARG_UNUSED(dev);
    static bool initialized;
    if (!initialized) {
        for (size_t i = 0; i < ARRAY_SIZE(active); i++) {
            active[i].status = PHT_UNUSED;
            k_work_init_delayable(&active[i].timer, timer_handler);
        }
        initialized = true;
    }
    return 0;
}

#define PHT_INST(n)                                                                                \
    static const struct pht_config pht_config_##n = {                                             \
        .tapping_term_ms = DT_INST_PROP(n, tapping_term_ms),                                     \
        .quick_tap_ms = DT_INST_PROP(n, quick_tap_ms),                                           \
        .require_prior_idle_ms = DT_INST_PROP(n, require_prior_idle_ms),                         \
        .flavor = DT_ENUM_IDX(DT_DRV_INST(n), flavor),                                           \
        .hold_while_undecided = DT_INST_PROP(n, hold_while_undecided),                            \
        .hold_behavior_dev = DEVICE_DT_NAME(DT_INST_PHANDLE_BY_IDX(n, bindings, 0)),             \
        .tap_behavior_dev = DEVICE_DT_NAME(DT_INST_PHANDLE_BY_IDX(n, bindings, 1)),              \
    };                                                                                            \
    BEHAVIOR_DT_INST_DEFINE(n, pht_init, NULL, NULL, &pht_config_##n, POST_KERNEL,               \
                            CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &pht_api);

DT_INST_FOREACH_STATUS_OKAY(PHT_INST)

#endif
