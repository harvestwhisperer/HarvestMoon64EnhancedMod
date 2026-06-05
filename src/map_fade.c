#include "common.h"
#include "modding.h"

#include "system/map.h"           
#include "system/mapController.h" 

#include "game/game.h"           
#include "game/gameStatus.h"      
#include "game/cutscenes.h"       
#include "game/cutsceneCompletionFlags.h" 
#include "game/level.h"           
#include "game/time.h"            

#include "mainLoop.h"             

#include "mod_config.h"           

// Map fade-in: replaces the hardcoded `setLevelLighting(8, 1)` with a configurable rate.
RECOMP_PATCH void setMapAudioAndLighting(void) {

    if (gCutsceneCompletionFlags & CUTSCENE_COMPLETION_OWN_AUDIO_LIGHTING) {
        setMainLoopCallbackFunctionIndex(MAIN_GAME);
    } else {

        if (!checkDailyEventBit(CUTSCENE_AUDIO_OVERRIDE)) {
            setLevelAudio(gBaseMapIndex, gSeason, gHour);
        }

        setLevelLighting(fade_in_rate(), 1);

    }

}

// Movement unblock: when enabled, hand control back to the gameplay callback immediately
// instead of waiting for the map fade-in to finish (the original `checkMapRGBADone` gate).
RECOMP_PATCH void levelLoadCallback(void) {

    handleCutsceneCompletion();

    if (fade_unblock_movement() || checkMapRGBADone(MAIN_MAP_INDEX)) {
        setMainLoopCallbackFunctionIndex(gameLoopContext.callbackIndex);
    }

}
