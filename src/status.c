#include <focus/status.h>

#include <stdio.h>

/* ASCII only, which is the whole rule: a byte with the high bit set is part of
 * a UTF-8 sequence and must come through untouched. Spelled out rather than
 * calling toupper(), whose behaviour above 0x7F is locale-dependent — that is
 * the bug the derived tree's layer label carried for its entire life, and a
 * range check here means no guard is needed at the call site. */
static char upper(char c) { return (c >= 'a' && c <= 'z') ? (char)(c - 'a' + 'A') : c; }

static void layer_text(const struct focus_status_state *state, char *out, size_t len) {
    if (state->layer_name && *state->layer_name) {
        snprintf(out, len, "%s", state->layer_name);
    } else {
        /* No display-name in the keymap. The index is still worth showing: it
         * is what the keymap's own layer list is ordered by. */
        snprintf(out, len, "L%u", state->layer_index);
    }

    if (!state->layer_uppercase) {
        return;
    }

    for (size_t i = 0; i < len && out[i]; i++) {
        out[i] = upper(out[i]);
    }
}

static void profile_text(const struct focus_status_state *state, struct focus_status_view *v) {
    static const char *const circled[FOCUS_PROFILE_CIRCLED_MAX] = {"①", "②", "③", "④", "⑤"};

    /* USB wins over the profile index. Plugged in is plugged in, whichever
     * profile happens to be selected underneath. */
    if (state->usb) {
        v->profile_form = FOCUS_PROFILE_TEXT;
        snprintf(v->profile, sizeof(v->profile), "USB");
        return;
    }

    if (state->profile_index < FOCUS_PROFILE_CIRCLED_MAX) {
        v->profile_form = FOCUS_PROFILE_CIRCLED;
        snprintf(v->profile, sizeof(v->profile), "%s", circled[state->profile_index]);
        return;
    }

    /* More BLE profiles than there are circled digits — possible, because ZMK
     * derives the count from CONFIG_BT_MAX_PAIRED. Fall back to the old spaced
     * form rather than baking twenty glyphs for a case nobody configures.
     * Spaced because "B6" reads cramped at this size. */
    v->profile_form = FOCUS_PROFILE_TEXT;
    snprintf(v->profile, sizeof(v->profile), "B %u", state->profile_index + 1);
}

/* GACS, matching the order the glyphs are drawn in and the order a Mac chord is
 * conventionally written. Each entry is the pair of HID bits that lights it:
 * left and right collapse, because the question is whether the modifier is
 * held, not which hand held it. */
static void mod_roles(uint8_t mods, enum focus_role *out) {
    static const uint8_t bits[FOCUS_MOD_COUNT] = {
        [FOCUS_MOD_GUI] = FOCUS_HID_LGUI | FOCUS_HID_RGUI,
        [FOCUS_MOD_ALT] = FOCUS_HID_LALT | FOCUS_HID_RALT,
        [FOCUS_MOD_CTRL] = FOCUS_HID_LCTL | FOCUS_HID_RCTL,
        [FOCUS_MOD_SHIFT] = FOCUS_HID_LSFT | FOCUS_HID_RSFT,
    };

    for (int i = 0; i < FOCUS_MOD_COUNT; i++) {
        out[i] = (mods & bits[i]) ? FOCUS_ROLE_ACCENT : FOCUS_ROLE_IDLE;
    }
}

struct focus_status_view focus_status_view_of(const struct focus_status_state *state) {
    struct focus_status_view v = {0};

    layer_text(state, v.layer, sizeof(v.layer));
    v.layer_role = FOCUS_ROLE_ACCENT;

    mod_roles(state->mods, v.mods);

    profile_text(state, &v);
    v.profile_role = FOCUS_ROLE_GREY;

    v.battery_count = state->battery_count < FOCUS_BATTERY_MAX ? state->battery_count
                                                               : FOCUS_BATTERY_MAX;
    for (uint8_t i = 0; i < v.battery_count; i++) {
        uint8_t level = state->battery[i];

        snprintf(v.battery[i].text, sizeof(v.battery[i].text), "%u", level);
        v.battery[i].role = level < FOCUS_BATTERY_LOW_PCT ? FOCUS_ROLE_LOW : FOCUS_ROLE_OK;
    }

    return v;
}
