// Make the ocarina key item playable.

#include "common.h"   // pulls in <libmus.h> for musConfig
#include "modding.h"
#include "recompconfig.h"

#include "system/audio.h"       // setSfx, setSfxVolume, initializeAudio, gCurrentAudioSequenceIndex, gAudioSequenceVolume
#include "system/controller.h"  // checkButtonPressed, CONTROLLER_1, BUTTON_*
#include "system/entity.h"      // setEntityAnimation, checkEntityAnimationStateChanged

#include "game/player.h"        // gPlayer, PLAYER_RIDING_HORSE, checkHaveKeyItem, playerIdleCounter
#include "game/items.h"         // OCARINA
#include "game/gameAudio.h"     // setAudioSequenceVolume

#include "assetIndices/entities.h" // ENTITY_PLAYER

#include "data/audio/sfx.h"     // sfxList, musPriorityList

// --- IDs the dev-qol branch added to the game headers (absent on master) ------

// Player action / animation handler slots.
#define PLAYING_OCARINA 35
#define ANIM_OCARINA    36

// The ocarina-performance player animation scripts already exist in the ROM;
// master just leaves them unnamed (PLAYER_ANIMATION_<n>).
#define PLAYER_ANIMATION_OCARINA_1 471 // raise to mouth
#define PLAYER_ANIMATION_OCARINA_2 411 // hold / idle
#define PLAYER_ANIMATION_OCARINA_3 412 // head-bob on note

// SFX ids for the eight notes (one octave, major scale). SFX_TYPE playback reads
// the effect list at id+1, so id 104 maps to effect index 105, and so on.
#define OCARINA_NOTE_1 104
#define OCARINA_NOTE_2 105
#define OCARINA_NOTE_3 106
#define OCARINA_NOTE_4 107
#define OCARINA_NOTE_5 108
#define OCARINA_NOTE_6 109
#define OCARINA_NOTE_7 110
#define OCARINA_NOTE_8 111

#define OCARINA_NO_NOTE 0xFF

// Background music is ducked to this fraction of its volume while playing.
#define OCARINA_MUSIC_DUCK_DIVISOR 2

// --- Audio injection ----------------------------------------------------------
// sfx.sfx bytecode (`play instr=0x33 vol=0x7F note=<n> length=0x30`). Index 104 is a pad
// so that note id N lands on effect-list index N+1, matching SFX_TYPE playback.

static char ocarinaPad[]   = { 0x80, 0x00, 0x00, 0x00 };
static char ocarinaNote1[] = { 0x81, 0x33, 0x84, 0x01, 0x73, 0x01, 0x73, 0x01, 0x73, 0x02, 0x9C, 0x7F, 0xA6, 0x7F, 0x30, 0x30, 0x80, 0x00, 0x00, 0x00 };
static char ocarinaNote2[] = { 0x81, 0x33, 0x84, 0x01, 0x73, 0x01, 0x73, 0x01, 0x73, 0x02, 0x9C, 0x7F, 0xA6, 0x7F, 0x32, 0x30, 0x80, 0x00, 0x00, 0x00 };
static char ocarinaNote3[] = { 0x81, 0x33, 0x84, 0x01, 0x73, 0x01, 0x73, 0x01, 0x73, 0x02, 0x9C, 0x7F, 0xA6, 0x7F, 0x34, 0x30, 0x80, 0x00, 0x00, 0x00 };
static char ocarinaNote4[] = { 0x81, 0x33, 0x84, 0x01, 0x73, 0x01, 0x73, 0x01, 0x73, 0x02, 0x9C, 0x7F, 0xA6, 0x7F, 0x35, 0x30, 0x80, 0x00, 0x00, 0x00 };
static char ocarinaNote5[] = { 0x81, 0x33, 0x84, 0x01, 0x73, 0x01, 0x73, 0x01, 0x73, 0x02, 0x9C, 0x7F, 0xA6, 0x7F, 0x37, 0x30, 0x80, 0x00, 0x00, 0x00 };
static char ocarinaNote6[] = { 0x81, 0x33, 0x84, 0x01, 0x73, 0x01, 0x73, 0x01, 0x73, 0x02, 0x9C, 0x7F, 0xA6, 0x7F, 0x39, 0x30, 0x80, 0x00, 0x00, 0x00 };
static char ocarinaNote7[] = { 0x81, 0x33, 0x84, 0x01, 0x73, 0x01, 0x73, 0x01, 0x73, 0x02, 0x9C, 0x7F, 0xA6, 0x7F, 0x3B, 0x30, 0x80, 0x00, 0x00, 0x00 };
static char ocarinaNote8[] = { 0x81, 0x33, 0x84, 0x01, 0x73, 0x01, 0x73, 0x01, 0x73, 0x02, 0x9C, 0x7F, 0xA6, 0x7F, 0x3C, 0x30, 0x80, 0x00, 0x00, 0x00 };

#define VANILLA_SFX_COUNT  104 // sfx0..sfx103 in the original ROM
#define OCARINA_PAD_INDEX  104
#define EXTENDED_SFX_COUNT 113 // pad (104) + eight notes (105..112)
#define OCARINA_NOTE_PRIORITY 100

static char *extendedFxList[EXTENDED_SFX_COUNT];
static int extendedPriorityList[EXTENDED_SFX_COUNT];

// Repoint the mus library's effect and priority lists at extended copies
// before initializeAudio hands them to nuAuStlInit. Runs once at boot.
RECOMP_HOOK("initializeAudio")
void hm64_ocarina_extend_sfx(musConfig *config) {

    int i;

    char *notes[] = {
        ocarinaPad,
        ocarinaNote1, ocarinaNote2, ocarinaNote3, ocarinaNote4,
        ocarinaNote5, ocarinaNote6, ocarinaNote7, ocarinaNote8,
    };

    for (i = 0; i < VANILLA_SFX_COUNT; i++) {
        extendedFxList[i] = sfxList[i];
        extendedPriorityList[i] = musPriorityList[i];
    }

    for (i = 0; i < EXTENDED_SFX_COUNT - OCARINA_PAD_INDEX; i++) {
        extendedFxList[OCARINA_PAD_INDEX + i] = notes[i];
        extendedPriorityList[OCARINA_PAD_INDEX + i] = OCARINA_NOTE_PRIORITY;
    }

    config->fxs = extendedFxList;
    config->priority = extendedPriorityList;

}

// --- Player action helpers (static inlines mirrored from player.c) -------------

static inline void ocarinaStartAction(u8 action, u8 animationHandler) {
    gPlayer.actionHandler = action;
    gPlayer.actionPhaseFrameCounter = 0;
    gPlayer.actionPhase = 0;
    gPlayer.animationHandler = animationHandler;
}

static inline void ocarinaResetAction(void) {
    gPlayer.actionPhaseFrameCounter = 0;
    gPlayer.actionPhase = 0;
    gPlayer.actionHandler = 0;
    gPlayer.animationHandler = 0;
}

static u8 getOcarinaNoteForButton(void) {
    if (checkButtonPressed(CONTROLLER_1, BUTTON_C_LEFT))  return OCARINA_NOTE_1;
    if (checkButtonPressed(CONTROLLER_1, BUTTON_C_DOWN))  return OCARINA_NOTE_2;
    if (checkButtonPressed(CONTROLLER_1, BUTTON_C_UP))    return OCARINA_NOTE_3;
    if (checkButtonPressed(CONTROLLER_1, BUTTON_C_RIGHT)) return OCARINA_NOTE_4;
    if (checkButtonPressed(CONTROLLER_1, BUTTON_D_LEFT))  return OCARINA_NOTE_5;
    if (checkButtonPressed(CONTROLLER_1, BUTTON_D_DOWN))  return OCARINA_NOTE_6;
    if (checkButtonPressed(CONTROLLER_1, BUTTON_D_UP))    return OCARINA_NOTE_7;
    if (checkButtonPressed(CONTROLLER_1, BUTTON_D_RIGHT)) return OCARINA_NOTE_8;
    return OCARINA_NO_NOTE;
}

// Play a note directly through the low-level SFX API. This is exactly what
// playSfx() does for an SFX_TYPE entry, but skips the vanilla audioTypeTable /
// volumesTable lookups (which don't cover the note indices in the stock ROM).
static void playOcarinaNote(u8 note) {
    setSfx(note + 1);
    setSfxVolume(note + 1, 128);
}

// --- Per-frame action handler (dispatched while PLAYING_OCARINA) ---------------

void handleOcarinaAction(void) {

    u8 note;

    // B puts the ocarina away and restores the music volume.
    if (gPlayer.actionPhase >= 2 && checkButtonPressed(CONTROLLER_1, BUTTON_B)) {
        setAudioSequenceVolume(gCurrentAudioSequenceIndex, gAudioSequenceVolume);
        ocarinaResetAction();
        return;
    }

    if (gPlayer.actionTimer) {
        gPlayer.actionTimer--;
    }

    // Accept notes while holding (phase 2) or already playing (phase 4).
    if ((gPlayer.actionPhase == 2 || gPlayer.actionPhase == 4) && !gPlayer.actionTimer) {
        note = getOcarinaNoteForButton();
        if (note != OCARINA_NO_NOTE) {
            playOcarinaNote(note);
            gPlayer.actionPhase = 3; // request the head-bob (re-triggers if already playing)
        }
    }

}

// --- Animation state machine (dispatched while ANIM_OCARINA) -------------------

void handleOcarinaAnimation(void) {

    switch (gPlayer.actionPhase) {

        case 0: // take out ocarina
            setEntityAnimation(ENTITY_PLAYER, PLAYER_ANIMATION_OCARINA_1);
            gPlayer.actionPhase = 1;
            break;

        case 1: // raise finished
            if (checkEntityAnimationStateChanged(ENTITY_PLAYER)) {
                setEntityAnimation(ENTITY_PLAYER, PLAYER_ANIMATION_OCARINA_2);
                gPlayer.actionPhase = 2;
            }
            break;

        case 2: // holding, idle
            break;

        case 3: // note played
            setEntityAnimation(ENTITY_PLAYER, PLAYER_ANIMATION_OCARINA_3);
            gPlayer.actionPhase = 4;
            break;

        case 4: // return to holding pose
            if (checkEntityAnimationStateChanged(ENTITY_PLAYER)) {
                setEntityAnimation(ENTITY_PLAYER, PLAYER_ANIMATION_OCARINA_2);
                gPlayer.actionPhase = 2;
            }
            break;

    }

}

// --- Hooks wiring the handlers into the player update loop ---------------------

// updatePlayerAction()'s switch has no PLAYING_OCARINA case in the stock ROM, so
// run our handler at entry when that action is active. (handlePlayerInput is only
// called by the original when actionHandler == 0, so there's no double-dispatch.)
RECOMP_HOOK("updatePlayerAction")
void hm64_ocarina_action_dispatch(void) {
    if (gPlayer.actionHandler == PLAYING_OCARINA) {
        handleOcarinaAction();
    }
}

// Same idea for the animation dispatch (stock switch falls through to default).
RECOMP_HOOK("handlePlayerAnimation")
void hm64_ocarina_animation_dispatch(void) {
    if (gPlayer.animationHandler == ANIM_OCARINA) {
        handleOcarinaAnimation();
        playerIdleCounter = 0;
    }
}

// Start the action on D-Up, but only if the player is still idle after the stock
// input handling ran (i.e. nothing else claimed the input this frame). This is
// the equivalent of the dev-qol `if (!set)` insertion without reimplementing the
// ~250-line handlePlayerInput.
RECOMP_HOOK_RETURN("handlePlayerInput")
void hm64_ocarina_input(void) {

    if (recomp_get_config_u32("enable_ocarina") != 0) {
        return;
    }

    if (gPlayer.actionHandler == 0
        && !(gPlayer.flags & PLAYER_RIDING_HORSE)
        && checkButtonPressed(CONTROLLER_1, BUTTON_D_UP)
        && checkHaveKeyItem(OCARINA)) {

        ocarinaStartAction(PLAYING_OCARINA, ANIM_OCARINA);
        gPlayer.actionTimer = 0; // clear any leftover note delay

        // duck the background music while playing
        setAudioSequenceVolume(gCurrentAudioSequenceIndex, gAudioSequenceVolume / OCARINA_MUSIC_DUCK_DIVISOR);

    }

}
