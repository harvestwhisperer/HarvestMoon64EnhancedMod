#include "common.h"
#include "modding.h"

#include "system/audio.h"         // gCurrentAudioSequenceIndex
#include "system/cutscene.h"      // deactivateCutsceneExecutors, initializeCutsceneExecutors
#include "system/entity.h"        // setEntitiesRGBA, setEntitiesRGBAWithTransition
#include "system/map.h"           // checkMapRGBADone
#include "system/mapController.h" // MAIN_MAP_INDEX, setMapControllerRGBA(WithTransition)

#include "game/cutscenes.h"       // gCutsceneFlags, gCutsceneCompletionFlags, handleCutsceneCompletion
#include "game/game.h"            // gameLoopContext, resetGlobalLighting, setMainLoopCallbackFunctionIndex
#include "game/gameAudio.h"       // stopAudioSequenceWithDefaultFadeOut, checkDefaultSequenceChannelOpen
#include "game/level.h"           // getMapForSpawnPoint, gBaseMapIndex, gSpawnPointIndex
#include "game/time.h"            // gSeason, gHour
#include "game/transition.h"      // pauseGameplay
#include "game/weather.h"         // RAIN, TYPHOON, gWeather

#include "mainLoop.h"             // MAP_LOAD, EXIT_LEVEL

#include "assetIndices/maps.h"    // BEACH
#include "assetIndices/sfxs.h"    // RAIN_SFX, TYPHOON_SFX

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

    setMapControllerRGBAWithTransition(MAIN_MAP_INDEX, 0, 0, 0, 0, 8);
    setEntitiesRGBAWithTransition(0, 0, 0, 0, 8);

    nextMapIndex = getMapForSpawnPoint(gSpawnPointIndex);
    currentMusicIndex = getMusicIndexForMap(gBaseMapIndex, gSeason, gHour);
    nextMusicIndex = getMusicIndexForMap(nextMapIndex, gSeason, gHour);

    // Only stop the current track if the next map uses a different one.
    if (currentMusicIndex != nextMusicIndex) {
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

    skipAudioWait = (currentMusicIndex == nextMusicIndex);

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
