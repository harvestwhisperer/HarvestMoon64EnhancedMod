// Configurable message boxes: text reveal speed, line-scroll speed, and a
// master toggle for message box sound effects. All three are driven by the
// mod's config options (see mod.toml). Each feature reimplements one of the
// game's small message-box setter functions so the configured value is applied
// the next time a message box is set up.

#include "common.h"
#include "modding.h"
#include "recompconfig.h"

#include "system/message.h"        // messageBoxes, MAX_MESSAGE_BOXES, MESSAGE_BOX_*, flags
#include "system/interpolation.h"  // initializeInterpolator, Interpolator

// --- Text reveal speed --------------------------------------------------------
// Normal reading text is set up with a negative interpolation rate (vanilla -4,
// i.e. one character every 4 frames). calculateInterpolatorStep() turns a rate
// of -N into "print one character every N frames", so a smaller magnitude is
// faster. Special/instant modes (naming screen echo, etc.) pass a positive rate
// and are left untouched.
//
// Config "text_reveal_speed" is a 1..7 slider where 4 == vanilla and higher is
// faster: framesPerChar = 8 - speed, so rate = speed - 8 maps 4 -> -4 (vanilla),
// 7 -> -1 (fastest), 1 -> -7 (slowest).
static s16 applyConfiguredRevealRate(s16 rate) {

    u32 speed;

    // Only override the normal character-by-character reading rate.
    if (rate >= 0) {
        return rate;
    }

    speed = recomp_get_config_u32("text_reveal_speed");

    if (speed < 1) {
        speed = 1;
    } else if (speed > 7) {
        speed = 7;
    }

    return (s16)speed - 8;

}

RECOMP_PATCH bool setMessageBoxInterpolationWithFlags(u16 index, s16 rate, u8 interpolationMode) {

    bool result = FALSE;

    rate = applyConfiguredRevealRate(rate);

    if (index < MAX_MESSAGE_BOXES) {

        if (messageBoxes[index].flags & MESSAGE_BOX_ACTIVE) {

            messageBoxes[index].unk_7C = rate;
            initializeInterpolator((Interpolator*)&messageBoxes[index].unk_64, rate, 0);

            messageBoxes[index].flags &= ~(MESSAGE_BOX_INTERPOLATION_MODE_1 | MESSAGE_BOX_INTERPOLATION_MODE_2);

            switch (interpolationMode) {

                case 0:
                    break;

                case 1:
                    messageBoxes[index].flags |= MESSAGE_BOX_INTERPOLATION_MODE_1;
                    break;

                case 2:
                    messageBoxes[index].flags |= MESSAGE_BOX_INTERPOLATION_MODE_2;
                    break;

                case 3:
                    messageBoxes[index].flags |= (MESSAGE_BOX_INTERPOLATION_MODE_1 | MESSAGE_BOX_INTERPOLATION_MODE_2);
                    break;

            }

            result = TRUE;

        }

    }

    return result;

}

// --- Line-scroll speed --------------------------------------------------------
// defaultScrollSpeed is the per-frame step of the scroll interpolator used when
// the box scrolls up to reveal the next line. Higher == faster. We treat the
// config value as a multiplier on the game's chosen base speed (1 == vanilla) so
// the relative differences between boxes are preserved.
RECOMP_PATCH bool setMessageBoxScrollSpeed(u16 index, s16 speed) {

    bool result = FALSE;

    u32 multiplier = recomp_get_config_u32("line_scroll_speed");

    if (multiplier < 1) {
        multiplier = 1;
    } else if (multiplier > 6) {
        multiplier = 6;
    }

    speed *= (s16)multiplier;

    if (index < MAX_MESSAGE_BOXES) {

        if (messageBoxes[index].flags & MESSAGE_BOX_ACTIVE) {
            messageBoxes[index].defaultScrollSpeed = speed;
            result = TRUE;
        }
    }

    return result;

}

// --- Message box sound effects ------------------------------------------------
// characterPrintSfx is the per-character text blip, and the engine also uses it
// as the gate for the close/confirm sounds (a box only plays any sound when
// characterPrintSfx != 0xFF). Forcing all three SFX indices to 0xFF therefore
// silences every message box sound. Config "message_sfx": 0 == Enabled,
// 1 == Disabled.
RECOMP_PATCH bool setMessageBoxSfx(u16 index, u32 characterPrintSfx, u32 waitSfx, u32 closeSfx) {

    bool result = FALSE;

    if (recomp_get_config_u32("message_sfx") != 0) {
        characterPrintSfx = 0xFF;
        waitSfx = 0xFF;
        closeSfx = 0xFF;
    }

    if (index < MAX_MESSAGE_BOXES) {

        if (messageBoxes[index].flags & MESSAGE_BOX_ACTIVE) {

            messageBoxes[index].characterPrintSfx = characterPrintSfx;
            messageBoxes[index].unk_74 = waitSfx;
            messageBoxes[index].messageCloseSfx = closeSfx;

            result = TRUE;

        }

    }

    return result;

}
