#include "common.h"

#include "assetIndices/sfxs.h"
#include "buffers/overlayScreensBuffer.h"
#include "game/gameFile.h"
#include "modding.h"
#include "system/audio.h"
#include "system/controller.h"
#include "system/globalSprites.h"
#include "system/sprite.h"

#include "mod_config.h"

extern void playSfx(u16 index);

#define LOAD_GAME_RANKING_BUTTON 0xAD
#define LOAD_GAME_RANKING_BUTTON_ANIMATION 6

extern u8 _loadGameScreenTextureSegmentRomStart;
extern u8 _loadGameScreenTextureSegmentRomEnd;
extern u8 _loadGameScreenAssetsIndexSegmentRomStart;
extern u8 _loadGameScreenAssetsIndexSegmentRomEnd;

static void clear_button_repeat(u32 button_mask) {
    controllers[CONTROLLER_1].buttonRepeat &= ~button_mask;
    gControllers[CONTROLLER_1].buttonRepeat &= ~button_mask;
}

RECOMP_HOOK_RETURN("loadDiarySelectScreen")
void hm64_leaderboard_load_diary_select_screen_return(void) {
    if (!config_enabled("enable_leaderboard")) {
        return;
    }

    dmaSprite(
        LOAD_GAME_RANKING_BUTTON,
        (u32)&_loadGameScreenTextureSegmentRomStart,
        (u32)&_loadGameScreenTextureSegmentRomEnd,
        (u32)&_loadGameScreenAssetsIndexSegmentRomStart,
        (u32)&_loadGameScreenAssetsIndexSegmentRomEnd,
        NULL,
        NULL,
        (u8*)OVERLAY_SCREEN_TEXTURE_BUFFER,
        NULL,
        (u16*)OVERLAY_SCREEN_PALETTE_BUFFER,
        (AnimationFrameMetadata*)OVERLAY_SCREEN_ANIMATION_FRAME_METADATA_BUFFER,
        (u8*)OVERLAY_SCREEN_TEXTURE_TO_PALETTE_LOOKUP_BUFFER,
        NULL,
        0,
        FALSE
    );

    setSpriteViewSpacePosition(LOAD_GAME_RANKING_BUTTON, 0.0f, 0.0f, 8.0f);
    setSpriteColor(LOAD_GAME_RANKING_BUTTON, 0, 0, 0, 0);
    setSpriteBlendMode(LOAD_GAME_RANKING_BUTTON, SPRITE_BLEND_ALPHA_DECAL);
    setBilinearFiltering(LOAD_GAME_RANKING_BUTTON, TRUE);
    startSpriteAnimation(LOAD_GAME_RANKING_BUTTON, LOAD_GAME_RANKING_BUTTON_ANIMATION, 0);
}

RECOMP_HOOK("gameSelectCallback")
void hm64_leaderboard_game_select_hook(void) {
    if (!config_enabled("enable_leaderboard")) {
        return;
    }

    if (loadGameScreenContext.action != LOAD_GAME_ACTION_SELECT_COLUMN) {
        return;
    }

    if (!checkButtonRepeat(CONTROLLER_1, BUTTON_STICK_SOUTHWEST)) {
        return;
    }

    if (loadGameScreenContext.actionColumnHighlighted == 1) {
        loadGameScreenContext.actionColumnHighlighted = 2;
        playSfx(MOVE_CURSOR);
    }

    if (loadGameScreenContext.actionColumnHighlighted >= 2) {
        clear_button_repeat(BUTTON_STICK_SOUTHWEST);
    }
}
