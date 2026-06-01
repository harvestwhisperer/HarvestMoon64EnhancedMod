// Automatically water crops if planted during rain

#include "common.h"
#include "modding.h"

#include "game/items.h"           // ToolUse, toolUse
#include "game/groundObjects.h"   // GROUND_OBJECT_*, *_PLANTED_STAGE_1, INVALID_GROUND_OBJECT
#include "game/level.h"           // gBaseMapIndex, groundObjectsGridX/Z
#include "game/player.h"          // getOffsetTileCoordinates
#include "game/time.h"            // gSeason, SPRING/SUMMER/AUTUMN
#include "game/weather.h"         // gWeather, RAIN

#include "system/entity.h"        // entities
#include "system/graphic.h"       // convertScreenDirectionToWorldDirection, DIRECTION_NE
#include "system/mapController.h" // MAIN_MAP_INDEX

#include "assetIndices/entities.h" // ENTITY_PLAYER
#include "assetIndices/maps.h"     // GREENHOUSE, FARM
#include "assetIndices/sfxs.h"     // SEEDS_SFX

extern u8 toolSweepOffsetsLeft[];
extern u8 toolSweepOffsetsRight[];

extern void playSfx(u16 index);

static u8 applyRainToSownCrop(u8 groundObjectIndex) {

    if (gWeather == RAIN && gBaseMapIndex == FARM
        && (getGroundObjectToolInteractionFlags(groundObjectIndex) & GROUND_OBJECT_WATERABLE)) {
        groundObjectIndex++;
    }

    return groundObjectIndex;

}

RECOMP_PATCH void useTurnipSeeds(void) {

    u8 direction;
    u8 temp;
    u8 groundObjectIndex;

    Vec3f vec;

    direction = convertScreenDirectionToWorldDirection(entities[ENTITY_PLAYER].direction, MAIN_MAP_INDEX);

    if (direction < DIRECTION_NE) {
        temp = toolSweepOffsetsLeft[toolUse.stepIndex];
    } else {
        temp = toolSweepOffsetsRight[toolUse.stepIndex];
    }

    vec = getOffsetTileCoordinates(0.0f, temp);

    if ((getGroundObjectToolInteractionFlags(getGroundObjectIndexFromPlayerPosition(0.0f, temp)) & GROUND_OBJECT_PLANTABLE) && vec.y != 65535.0f) {

        if (gSeason == SPRING || gBaseMapIndex == GREENHOUSE) {
            groundObjectIndex = applyRainToSownCrop(TURNIP_PLANTED_STAGE_1);
        } else {
            groundObjectIndex = INVALID_GROUND_OBJECT;
        }

        addGroundObjectToMap(gBaseMapIndex, groundObjectIndex, (u8)vec.x - groundObjectsGridX, (u8)vec.z - groundObjectsGridZ);

    }

    if (toolUse.stepIndex == 0) {
        playSfx(SEEDS_SFX);
    }

    toolUse.stepIndex++;

    if (toolUse.stepIndex == 9) {
        toolUse.toolUseState = 0;
    }

}

RECOMP_PATCH void usePotatoSeeds(void) {

    u8 direction;
    u8 temp;
    u8 groundObjectIndex;

    Vec3f vec;

    direction = convertScreenDirectionToWorldDirection(entities[ENTITY_PLAYER].direction, MAIN_MAP_INDEX);

    if (direction < DIRECTION_NE) {
        temp = toolSweepOffsetsLeft[toolUse.stepIndex];
    } else {
        temp = toolSweepOffsetsRight[toolUse.stepIndex];
    }

    vec = getOffsetTileCoordinates(0.0f, temp);

    if ((getGroundObjectToolInteractionFlags(getGroundObjectIndexFromPlayerPosition(0.0f, temp)) & GROUND_OBJECT_PLANTABLE) && vec.y != 65535.0f) {

        if (gSeason == SPRING || gBaseMapIndex == GREENHOUSE) {
          groundObjectIndex = applyRainToSownCrop(POTATO_PLANTED_STAGE_1);
        } else {
          groundObjectIndex = INVALID_GROUND_OBJECT;
        }

        addGroundObjectToMap(gBaseMapIndex, groundObjectIndex, (u8)vec.x - groundObjectsGridX, (u8)vec.z - groundObjectsGridZ);

    }

    if (toolUse.stepIndex == 0) {
        playSfx(SEEDS_SFX);
    }

    toolUse.stepIndex++;

    if (toolUse.stepIndex == 9) {
        toolUse.toolUseState = 0;
    }

}

RECOMP_PATCH void useCabbageSeeds(void) {

    u8 direction;
    u8 temp;
    u8 groundObjectIndex;

    Vec3f vec;

    direction = convertScreenDirectionToWorldDirection(entities[ENTITY_PLAYER].direction, MAIN_MAP_INDEX);

    if (direction < DIRECTION_NE) {
        temp = toolSweepOffsetsLeft[toolUse.stepIndex];
    } else {
        temp = toolSweepOffsetsRight[toolUse.stepIndex];
    }

    vec = getOffsetTileCoordinates(0.0f, temp);

    if ((getGroundObjectToolInteractionFlags(getGroundObjectIndexFromPlayerPosition(0.0f, temp)) & GROUND_OBJECT_PLANTABLE) && vec.y != 65535.0f) {

        if (gSeason == SPRING || gBaseMapIndex == GREENHOUSE) {
          groundObjectIndex = applyRainToSownCrop(CABBAGE_PLANTED_STAGE_1);
        } else {
          groundObjectIndex = INVALID_GROUND_OBJECT;
        }

        addGroundObjectToMap(gBaseMapIndex, groundObjectIndex, (u8)vec.x - groundObjectsGridX, (u8)vec.z - groundObjectsGridZ);

    }

    if (toolUse.stepIndex == 0) {
        playSfx(SEEDS_SFX);
    }

    toolUse.stepIndex++;

    if (toolUse.stepIndex == 9) {
        toolUse.toolUseState = 0;
    }

}

RECOMP_PATCH void useTomatoSeeds(void) {

    u8 direction;
    u8 temp;
    u8 groundObjectIndex;

    Vec3f vec;

    direction = convertScreenDirectionToWorldDirection(entities[ENTITY_PLAYER].direction, MAIN_MAP_INDEX);

    if (direction < DIRECTION_NE) {
        temp = toolSweepOffsetsLeft[toolUse.stepIndex];
    } else {
        temp = toolSweepOffsetsRight[toolUse.stepIndex];
    }

    vec = getOffsetTileCoordinates(0.0f, temp);

    if ((getGroundObjectToolInteractionFlags(getGroundObjectIndexFromPlayerPosition(0.0f, temp)) & GROUND_OBJECT_PLANTABLE) && vec.y != 65535.0f) {

        if (gSeason == SUMMER || gBaseMapIndex == GREENHOUSE) {
          groundObjectIndex = applyRainToSownCrop(TOMATO_PLANTED_STAGE_1);
        } else {
          groundObjectIndex = INVALID_GROUND_OBJECT;
        }

        addGroundObjectToMap(gBaseMapIndex, groundObjectIndex, (u8)vec.x - groundObjectsGridX, (u8)vec.z - groundObjectsGridZ);

    }

    if (toolUse.stepIndex == 0) {
        playSfx(SEEDS_SFX);
    }

    toolUse.stepIndex++;

    if (toolUse.stepIndex == 9) {
        toolUse.toolUseState = 0;
    }

}

RECOMP_PATCH void useCornSeeds(void) {

    u8 direction;
    u8 temp;
    u8 groundObjectIndex;

    Vec3f vec;

    direction = convertScreenDirectionToWorldDirection(entities[ENTITY_PLAYER].direction, MAIN_MAP_INDEX);

    if (direction < DIRECTION_NE) {
        temp = toolSweepOffsetsLeft[toolUse.stepIndex];
    } else {
        temp = toolSweepOffsetsRight[toolUse.stepIndex];
    }

    vec = getOffsetTileCoordinates(0.0f, temp);

    if ((getGroundObjectToolInteractionFlags(getGroundObjectIndexFromPlayerPosition(0.0f, temp)) & GROUND_OBJECT_PLANTABLE) && vec.y != 65535.0f) {

        if (gSeason == SUMMER || gBaseMapIndex == GREENHOUSE) {
            groundObjectIndex = applyRainToSownCrop(CORN_PLANTED_STAGE_1);
        } else {
            groundObjectIndex = INVALID_GROUND_OBJECT;
        }

        addGroundObjectToMap(gBaseMapIndex, groundObjectIndex, (u8)vec.x - groundObjectsGridX, (u8)vec.z - groundObjectsGridZ);
    }

    if (toolUse.stepIndex == 0) {
        playSfx(SEEDS_SFX);
    }

    toolUse.stepIndex++;

    if (toolUse.stepIndex == 9) {
        toolUse.toolUseState = 0;
    }

}

RECOMP_PATCH void useEggplantSeeds(void) {

    u8 direction;
    u8 temp;
    u8 groundObjectIndex;

    Vec3f vec;

    direction = convertScreenDirectionToWorldDirection(entities[ENTITY_PLAYER].direction, MAIN_MAP_INDEX);

    if (direction < DIRECTION_NE) {
        temp = toolSweepOffsetsLeft[toolUse.stepIndex];
    }
    else {
        temp = toolSweepOffsetsRight[toolUse.stepIndex];
    }

    vec = getOffsetTileCoordinates(0.0f, temp);

    if ((getGroundObjectToolInteractionFlags(getGroundObjectIndexFromPlayerPosition(0.0f, temp)) & GROUND_OBJECT_PLANTABLE) && vec.y != 65535.0f) {

        if (gSeason == AUTUMN || gBaseMapIndex == GREENHOUSE) {
            groundObjectIndex = applyRainToSownCrop(EGGPLANT_PLANTED_STAGE_1);
        } else {
            groundObjectIndex = INVALID_GROUND_OBJECT;
        }

        addGroundObjectToMap(gBaseMapIndex, groundObjectIndex, (u8)vec.x - groundObjectsGridX, (u8)vec.z - groundObjectsGridZ);

    }

    if (toolUse.stepIndex == 0) {
        playSfx(SEEDS_SFX);
    }

    toolUse.stepIndex++;

    if (toolUse.stepIndex == 9) {
        toolUse.toolUseState = 0;
    }

}

RECOMP_PATCH void useStrawberrySeeds(void) {

    u8 direction;
    u8 temp;
    u8 groundObjectIndex;

    Vec3f vec;

    direction = convertScreenDirectionToWorldDirection(entities[ENTITY_PLAYER].direction, MAIN_MAP_INDEX);

    if (direction < DIRECTION_NE) {
        temp = toolSweepOffsetsLeft[toolUse.stepIndex];
    }
    else {
        temp = toolSweepOffsetsRight[toolUse.stepIndex];
    }

    vec = getOffsetTileCoordinates(0.0f, temp);

    if ((getGroundObjectToolInteractionFlags(getGroundObjectIndexFromPlayerPosition(0.0f, temp)) & GROUND_OBJECT_PLANTABLE) && vec.y != 65535.0f) {

        // leaving for reference
        //groundObjectIndex = ((-(gBaseMapIndex != 0x56) & ~0x28) | 0x44);

        groundObjectIndex = applyRainToSownCrop((gBaseMapIndex == GREENHOUSE) ? STRAWBERRY_PLANTED_STAGE_1 : INVALID_GROUND_OBJECT);

        addGroundObjectToMap(gBaseMapIndex, groundObjectIndex, (u8)vec.x - groundObjectsGridX, (u8)vec.z - groundObjectsGridZ);

    }

    if (toolUse.stepIndex == 0) {
        playSfx(SEEDS_SFX);
    }

    toolUse.stepIndex++;

    if (toolUse.stepIndex == 9) {
        toolUse.toolUseState = 0;
    }

}

RECOMP_PATCH void useMoonDropSeeds(void) {

    u8 direction;
    u8 temp;
    u8 groundObjectIndex;

    Vec3f vec;

    direction = convertScreenDirectionToWorldDirection(entities[ENTITY_PLAYER].direction, MAIN_MAP_INDEX);

    if (direction < DIRECTION_NE) {
        temp = toolSweepOffsetsLeft[toolUse.stepIndex];
    }
    else {
        temp = toolSweepOffsetsRight[toolUse.stepIndex];
    }

    vec = getOffsetTileCoordinates(0.0f, temp);

    if ((getGroundObjectToolInteractionFlags(getGroundObjectIndexFromPlayerPosition(0.0f, temp)) & GROUND_OBJECT_PLANTABLE) && vec.y != 65535.0f) {

        if ((gSeason == SPRING || gSeason == SUMMER) || gBaseMapIndex == GREENHOUSE) {
            groundObjectIndex = applyRainToSownCrop(MOONDROP_PLANTED_STAGE_1);
        } else {
            groundObjectIndex = INVALID_GROUND_OBJECT;
        }

        addGroundObjectToMap(gBaseMapIndex, groundObjectIndex, (u8)vec.x - groundObjectsGridX, (u8)vec.z - groundObjectsGridZ);

    }

    if (toolUse.stepIndex == 0) {
        playSfx(SEEDS_SFX);
    }

    toolUse.stepIndex++;

    if (toolUse.stepIndex == 9) {
        toolUse.toolUseState = 0;
    }

}

RECOMP_PATCH void usePinkCatMintSeeds(void) {

    u8 direction;
    u8 temp;
    u8 groundObjectIndex;

    Vec3f vec;

    direction = convertScreenDirectionToWorldDirection(entities[ENTITY_PLAYER].direction, MAIN_MAP_INDEX);

    if (direction < DIRECTION_NE) {
        temp = toolSweepOffsetsLeft[toolUse.stepIndex];
    } else {
        temp = toolSweepOffsetsRight[toolUse.stepIndex];
    }

    vec = getOffsetTileCoordinates(0.0f, temp);

    if ((getGroundObjectToolInteractionFlags(getGroundObjectIndexFromPlayerPosition(0.0f, temp)) & GROUND_OBJECT_PLANTABLE) && vec.y != 65535.0f) {

        // FIXME: should be range
        if ((gSeason - 1 < 2U) || gBaseMapIndex == GREENHOUSE) {
            groundObjectIndex = applyRainToSownCrop(PINK_CAT_MINT_PLANTED_STAGE_1);
        } else {
            groundObjectIndex = INVALID_GROUND_OBJECT;
        }

        addGroundObjectToMap(gBaseMapIndex, groundObjectIndex, (u8)vec.x - groundObjectsGridX, (u8)vec.z - groundObjectsGridZ);
    }

    if (toolUse.stepIndex == 0) {
        playSfx(SEEDS_SFX);
    }

    toolUse.stepIndex++;

    if (toolUse.stepIndex == 9) {
        toolUse.toolUseState = 0;
    }

}
