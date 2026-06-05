#include "common.h"
#include "modding.h"

#include "system/audio.h"         
#include "system/cutscene.h"      
#include "system/entity.h"        
#include "system/map.h"           
#include "system/mapController.h" 

#include "game/cutscenes.h"       
#include "game/game.h"            
#include "game/gameAudio.h"       
#include "game/level.h"           
#include "game/time.h"            
#include "game/transition.h"      
#include "game/weather.h"         

#include "mainLoop.h"             

#include "assetIndices/maps.h"    
#include "assetIndices/sfxs.h"    
#include "mod_config.h"           

extern u8 levelToMusicMappings[][8];

static u8 getMusicIndexForMap(u8 mapIndex, u8 season, u8 hour) {

    u32 nightOffset;

    if (gWeather == RAIN) {
        return RAIN_SFX;
    }

    if (gWeather == TYPHOON) {
        return TYPHOON_SFX;
    }

    if (hour >= 6 || mapIndex == BEACH) {

        nightOffset = 0;

        if (17 < hour && hour < 24) {
            nightOffset = 4;
        }

        return levelToMusicMappings[mapIndex][nightOffset + (season - 1)];

    }

    return 0xFF;
}

// arg0 = unused
RECOMP_PATCH void handleExitLevel(u16 arg0, u16 callbackIndex) {

    u8 nextMapIndex;
    u8 currentMusicIndex;
    u8 nextMusicIndex;
    s16 rate = fade_out_rate();

    setMapControllerRGBAWithTransition(MAIN_MAP_INDEX, 0, 0, 0, 0, rate);
    setEntitiesRGBAWithTransition(0, 0, 0, 0, rate);

    nextMapIndex = getMapForSpawnPoint(gSpawnPointIndex);
    currentMusicIndex = getMusicIndexForMap(gBaseMapIndex, gSeason, gHour);
    nextMusicIndex = getMusicIndexForMap(nextMapIndex, gSeason, gHour);

    // Only stop the current track if the next map uses a different one.
    if (!config_enabled("enable_music_persist") || currentMusicIndex != nextMusicIndex) {
        stopAudioSequenceWithDefaultFadeOut(gCurrentAudioSequenceIndex);
    }

    gameLoopContext.callbackIndex = callbackIndex;

    resetGlobalLighting();

    if (gameLoopContext.callbackIndex) {
        setMainLoopCallbackFunctionIndex(EXIT_LEVEL);
        pauseGameplay();
    }
}

RECOMP_PATCH void exitLevelCallback(void) {

    u8 nextMapIndex;
    u8 currentMusicIndex;
    u8 nextMusicIndex;
    u8 skipAudioWait;

    handleCutsceneCompletion();

    nextMapIndex = getMapForSpawnPoint(gSpawnPointIndex);
    currentMusicIndex = getMusicIndexForMap(gBaseMapIndex, gSeason, gHour);
    nextMusicIndex = getMusicIndexForMap(nextMapIndex, gSeason, gHour);

    skipAudioWait = config_enabled("enable_music_persist") && (currentMusicIndex == nextMusicIndex);

    if (checkMapRGBADone(MAIN_MAP_INDEX) && (skipAudioWait || checkDefaultSequenceChannelOpen(gCurrentAudioSequenceIndex))) {

        setEntitiesRGBA(0, 0, 0, 0);
        setMapControllerRGBA(MAIN_MAP_INDEX, 0, 0, 0, 0);
        deactivateCutsceneExecutors();
        initializeCutsceneExecutors();

        gCutsceneCompletionFlags = 0;
        gCutsceneFlags = 0;

        setMainLoopCallbackFunctionIndex(gameLoopContext.callbackIndex);
    }
}
