// Map loading optimizations.
//
// On maps with many ground objects, map transitions are slow because the engine 
// rebuilds the entire ground-object render state once per object instead of once 
// per map, re-read map data from ROM on every transition, and recomputed each ground 
// object's terrain height every frame. This mod removes that redundant work.
//
// Three independent optimizations:
//   1. Redundant rebuild fix. addGroundObjectToMap() rebuilds the ground-object
//      linked list (setGroundObjects) and tile texture mappings
//      (setGridToTileTextureMappings) every call. setMapGroundObjects() calls it
//      once per occupied tile during a load, so those O(grid) rebuilds ran dozens
//      of times. A "bulk loading" flag suppresses the per-object rebuilds; the
//      single rebuild the lazy updateGroundObjects() path performs covers it.
//   2. Map-data / ground-object ROM-read caching. dmaMapAssets() now skips the
//      map-data DMA when the same map is reloaded, and loadGroundObjects() skips
//      re-reading the (shared, map-independent) ground-object asset segment after
//      the first load.
//   3. Terrain-height render cache. setGroundObjects() precomputes each ground
//      object's terrain height once; renderGroundObjects() reads the cache instead
//      of calling getTerrainHeightAtPosition() per object per frame.

#include "common.h"
#include "modding.h"

#include "system/map.h"            
#include "system/mapController.h"  
#include "game/groundObjects.h"    
#include "game/level.h"            

#include "mainproc.h"              

#define GROUND_OBJECT_GRID_WIDTH  20
#define GROUND_OBJECT_GRID_HEIGHT 24
#define GROUND_OBJECT_GRID_SIZE   (GROUND_OBJECT_GRID_WIDTH * GROUND_OBJECT_GRID_HEIGHT)

#define MAP_CONTROLLER_DATA_CACHED            0x20
#define MAP_CONTROLLER_GROUND_OBJECTS_CACHED  0x40

extern MapDataAddress mapDataAddresses[];
extern GroundObjectInfo groundObjectsInfo[];
extern Gfx groundObjectBitmapsDisplayList[2][0x1000];
extern u8 gridIndexToTileIndexX[GROUND_OBJECT_GRID_SIZE];
extern u8 gridIndexToTileIndexZ[GROUND_OBJECT_GRID_SIZE];

extern void setMapGroundObjects(u8 mapIndex);
extern void loadLevelGroundObjects(u16 mapIndex);
extern void initializeMapAdditionsForLevel(u16 levelIndex);
extern void loadLevelMapObjects(u16 levelIndex);
extern void setAdditionalMapAdditionsForLevel(u16 mapIndex);
extern void setWeatherSprites(void);

extern Gfx *prepareGroundObjectBitmap(Gfx *dl, GroundObjectBitmap *sprite);
extern Gfx *renderGroundObject(Gfx *dl, MainMap *map, GroundObjectBitmap *bitmap, u16 vtxIndex);
extern void addGroundObjectToSceneGraph(MainMap *map, f32 x, f32 y, f32 z, f32 cameraOffsetX, f32 cameraOffsetZ, Gfx *dl);

// Per-grid terrain heights, precomputed by setGroundObjects() and consumed by
// renderGroundObjects(). Only meaningful for terrain-sampling maps (groundObjects.y == 0).
static f32 groundObjectTerrainHeightCache[GROUND_OBJECT_GRID_SIZE];

// Set while setMapGroundObjects() bulk-adds a map's ground objects.
static bool gBulkLoadingGroundObjects = FALSE;

static inline u8 *getAddress(u32 offsets[], u32 i) {
    return (u8 *)offsets + offsets[i];
}

// --- Optimization 1: suppress per-object rebuilds during a bulk load ---------

RECOMP_HOOK("setMapGroundObjects")
void hm64_map_loading_bulk_load_begin(void) {
    gBulkLoadingGroundObjects = TRUE;
}

RECOMP_HOOK_RETURN("setMapGroundObjects")
void hm64_map_loading_bulk_load_end(void) {
    gBulkLoadingGroundObjects = FALSE;
}

RECOMP_PATCH void addGroundObjectToMap(u8 mapIndex, u8 groundObjectIndex, u8 x, u8 z) {

    u8 spriteIndex;

    if (setFieldTile(mapIndex, groundObjectIndex, x, z)) {

        spriteIndex = groundObjectsInfo[groundObjectIndex].spriteIndex;

        if (groundObjectsInfo[groundObjectIndex].mapAdditionIndex != 0xFF) {
            setMapAdditionIndexFromCoordinates(MAIN_MAP_INDEX, groundObjectsInfo[groundObjectIndex].mapAdditionIndex, groundObjectsGridX + x, groundObjectsGridZ + z);
        }

        switch (spriteIndex) {

            default:
                setMapGroundObjectSpriteIndex(MAIN_MAP_INDEX, spriteIndex, x, z);
                break;

            case 0xFF:
                setMapGroundObjectSpriteIndex(MAIN_MAP_INDEX, 0xFFFF, x, z);
                break;

            case 0:
                setMapGroundObjectSpriteIndex(MAIN_MAP_INDEX, 0, x, z);
                break;

        }

        // During a bulk load these run once at the end of the load instead of
        // once per object (see setupLevelMap / updateGroundObjects).
        if (!gBulkLoadingGroundObjects) {
            setGroundObjects(MAIN_MAP_INDEX);
            setGridToTileTextureMappings(MAIN_MAP_INDEX);
        }

    }

}

// Drop the trailing setGroundObjects(MAIN_MAP_INDEX): with the bulk-load fix the
// per-object rebuilds are gone, and updateGroundObjects() does the single
// authoritative rebuild (and gates rendering on it) once the map is active.
RECOMP_PATCH void setupLevelMap(u16 mapIndex) {

    enableMapController(MAIN_MAP_INDEX);

    // set ground object sprites on map struct
    loadLevelGroundObjects(mapIndex);
    initializeMapAdditionsForLevel(mapIndex);

    // load and setup map object sprites
    loadLevelMapObjects(mapIndex);

    setAdditionalMapAdditionsForLevel(mapIndex);

    setMapGroundObjects(gBaseMapIndex);

    if (getLevelFlags(mapIndex) & LEVEL_OUTDOORS) {
        setWeatherSprites();
    }

    setGridToTileTextureMappings(MAIN_MAP_INDEX);

}

// --- Optimization 2: cache ROM reads across transitions ----------------------

RECOMP_PATCH bool dmaMapAssets(u16 mainMapIndex, u16 levelMapIndex) {

    MapDataAddress *mda = &mapDataAddresses[levelMapIndex];
    MapController *mc = &mapControllers[mainMapIndex];

    bool result = FALSE;

    u8 *mapGrid;
    void *mesh;
    u8 *terrainQuads;
    u8 **gridToLevelInteractionIndex;
    void *coreMapObjects;
    void *tileTextures;
    void *tilePalettes;
    void *coreMapObjectsTextures;
    void *coreMapObjectsPalettes;
    u8 *mapAdditionsMetadata;

    if (mainMapIndex == MAIN_MAP_INDEX && mc->flags & MAP_CONTROLLER_INITIALIZED) {

        // Reuse the buffer when the same map is reloaded; only re-DMA on a change.
        if (!(mc->flags & MAP_CONTROLLER_DATA_CACHED) || mc->mapIndex != levelMapIndex) {

            mc->mapIndex = levelMapIndex;

            nuPiReadRom((u32)mda->romStart, mc->mapDataIndex, (u32)((u8 *)mda->romEnd - (u8 *)mda->romStart));

            mc->flags |= MAP_CONTROLLER_DATA_CACHED;

        }

        mapGrid = getAddress(mc->mapDataIndex, 0);
        mesh = getAddress(mc->mapDataIndex, 1);
        terrainQuads = getAddress(mc->mapDataIndex, 2);
        gridToLevelInteractionIndex = (u8 **)getAddress(mc->mapDataIndex, 3);
        coreMapObjects = getAddress(mc->mapDataIndex, 4);
        tileTextures = getAddress(mc->mapDataIndex, 5);
        tilePalettes = getAddress(mc->mapDataIndex, 6);
        coreMapObjectsTextures = getAddress(mc->mapDataIndex, 7);
        coreMapObjectsPalettes = getAddress(mc->mapDataIndex, 8);
        mapAdditionsMetadata = getAddress(mc->mapDataIndex, 9);

        mc->flags |= MAP_CONTROLLER_ASSETS_LOADED;

        setupMap(mc->mainMapIndex,
            mapGrid,
            mesh,
            terrainQuads,
            gridToLevelInteractionIndex,
            coreMapObjects,
            tileTextures,
            tilePalettes,
            coreMapObjectsTextures,
            coreMapObjectsPalettes,
            mapAdditionsMetadata);

        result = TRUE;

    }

    return result;

}

RECOMP_PATCH bool loadGroundObjects(u16 mapIndex, u8 x, u8 z, u32 *textureIndex, u32 *paletteIndex, u8 *spriteToPaletteIndex, u32 romTextureStart, u32 romTextureEnd, u32 romAssetsIndexStart, u32 romAssetsIndexEnd, u8 y) {

    MainMap *mm = &mainMap[mapIndex];

    bool result = FALSE;

    u32 assetIndex[8];

    u32 offset1;
    u32 offset2;
    u32 offset3;
    u32 offset4;
    u32 offset5;

    if (mapIndex == MAIN_MAP_INDEX && (mm->mapState.flags & MAP_ACTIVE)) {

        mm->groundObjects.textureIndex = textureIndex;
        mm->groundObjects.paletteIndex = paletteIndex;
        mm->groundObjects.spriteToPaletteIndex = spriteToPaletteIndex;

        // grid positions
        mm->groundObjects.x = x;
        mm->groundObjects.z = z;
        mm->groundObjects.y = y;

        // The ground-object asset segment is shared by every map, so it only
        // needs to be read from ROM once into its (fixed) buffers.
        if (!(mapControllers[mapIndex].flags & MAP_CONTROLLER_GROUND_OBJECTS_CACHED)) {

            nuPiReadRom(romAssetsIndexStart, assetIndex, romAssetsIndexEnd - romAssetsIndexStart);

            offset1 = assetIndex[0];
            offset2 = assetIndex[1];
            offset3 = assetIndex[2];
            offset4 = assetIndex[3];
            offset5 = assetIndex[4];

            nuPiReadRom(romTextureStart + offset1, mm->groundObjects.textureIndex, offset2 - offset1);
            nuPiReadRom(romTextureStart + offset2, mm->groundObjects.paletteIndex, offset3 - offset2);
            nuPiReadRom(romTextureStart + offset4, mm->groundObjects.spriteToPaletteIndex, offset5 - offset4);

            mapControllers[mapIndex].flags |= MAP_CONTROLLER_GROUND_OBJECTS_CACHED;

        }

        result = TRUE;

    }

    return result;

}

RECOMP_PATCH void deactivateAllMapControllers(void) {

    u16 i;

    for (i = 0; i < MAX_MAPS; i++) {
        mapControllers[i].flags &= ~(MAP_CONTROLLER_ASSETS_LOADED | MAP_CONTROLLER_ACTIVE | MAP_CONTROLLER_DATA_CACHED);
    }

}

// The naming screen's texture buffer overlaps MAP_DATA_BUFFER. When a naming
// screen is opened mid-cutscene (after a map is already loaded and its data
// cached), loading the naming screen clobbers the cached map data, so the
// map-data cache must be invalidated to force dmaMapAssets() to re-read it when
// gameplay resumes. Without the cache (optimization 2 above) dmaMapAssets always
// re-read, so this only matters once that caching is in place.
RECOMP_HOOK("loadNamingScreenCallback")
void hm64_map_loading_naming_screen_invalidate(void) {
    mapControllers[MAIN_MAP_INDEX].flags &= ~MAP_CONTROLLER_DATA_CACHED;
}

RECOMP_PATCH bool activateMapAddition(u16 mapIndex, u16 mapAdditionIndex, bool loopFlag) {

    MainMap *mm = &mainMap[mapIndex];

    bool result = FALSE;

    if (mapIndex == MAIN_MAP_INDEX && mm->mapState.flags & MAP_ACTIVE && mapAdditionIndex < 32) {

        if (!(mm->mapAdditions[mapAdditionIndex].flags & MAP_ADDITION_ACTIVE)) {

            mm->mapAdditions[mapAdditionIndex].flags = MAP_ADDITION_ACTIVE;
            mm->mapAdditions[mapAdditionIndex].processingTimer = 0;
            mm->mapAdditions[mapAdditionIndex].currentStep = 0;

            result = TRUE;

            if (loopFlag) {
                mm->mapAdditions[mapAdditionIndex].flags = MAP_ADDITION_ACTIVE | MAP_ADDITION_LOOPING;
                // reset cache for overlay screens (toolbox, freezer, cabinet)
                mapControllers[MAIN_MAP_INDEX].flags &= ~MAP_CONTROLLER_DATA_CACHED;
            }

        }

    }

    return result;

}

// --- Optimization 3: precompute terrain heights once per load ----------------

RECOMP_PATCH void setGroundObjects(u16 mapIndex) {

    MainMap *mm = &mainMap[mapIndex];

    u16 tempArr[0x40];

    u16 j, l;
    u8 k, m;
    u16 spriteIndex;
    u16 temp;
    s16 tileOffX, tileOffZ;

    for (j = 0; j < GROUND_OBJECT_GRID_SIZE; j++) {
        mm->groundObjects.previousGridToSpriteIndex[j] = 0;
        mm->groundObjects.nextGridToSpriteIndex[j] = 0;
    }

    for (k = 0; k < MAX_GROUND_OBJECTS; k++) {
        mm->groundObjects.spriteIndexToGrid[k] = 0xFFFF;
        tempArr[k] = 0xFFFF;
    }

    for (l = 0; l < GROUND_OBJECT_GRID_SIZE; l++) {

        spriteIndex = mm->groundObjects.gridToSpriteIndex[l];

        if (spriteIndex && spriteIndex != 0xFFFF) {

            if (mm->groundObjects.spriteIndexToGrid[spriteIndex] == 0xFFFF) {

                mm->groundObjects.spriteIndexToGrid[spriteIndex] = l;
                tempArr[spriteIndex] = l;
                mm->groundObjects.previousGridToSpriteIndex[l] = 0xFFFF;

            } else {

                temp = tempArr[spriteIndex];
                tempArr[spriteIndex] = l;
                mm->groundObjects.nextGridToSpriteIndex[temp] = l;
                mm->groundObjects.previousGridToSpriteIndex[l] = temp;

            }

        }

    }

    for (m = 0; m < MAX_GROUND_OBJECTS; m++) {

        if (tempArr[m] != 0xFFFF) {
            mm->groundObjects.nextGridToSpriteIndex[tempArr[m]] = 0xFFFF;
        }

    }

    // Terrain-sampling maps only: cache each occupied tile's terrain height so
    // renderGroundObjects() doesn't sample it every frame.
    if (!mm->groundObjects.y && (mm->mapState.flags & MAP_ACTIVE)) {

        tileOffX = (s16)((-(mm->mapGrid.mapWidth * mm->mapGrid.tileSizeX) / 2)
                 + (mm->mapGrid.tileSizeX * mm->groundObjects.x) - (mm->mapGrid.tileSizeX / 2));

        tileOffZ = (s16)((-(mm->mapGrid.mapHeight * mm->mapGrid.tileSizeZ) / 2)
                 + (mm->mapGrid.tileSizeZ * mm->groundObjects.z) - (mm->mapGrid.tileSizeZ / 2));

        for (l = 0; l < GROUND_OBJECT_GRID_SIZE; l++) {

            spriteIndex = mm->groundObjects.gridToSpriteIndex[l];

            if (spriteIndex && spriteIndex != 0xFFFF) {
                groundObjectTerrainHeightCache[l] = getTerrainHeightAtPosition(mapIndex,
                    tileOffX + (gridIndexToTileIndexX[l] * 32),
                    tileOffZ + (gridIndexToTileIndexZ[l] * 32));
            }

        }

    }

}

RECOMP_PATCH void renderGroundObjects(MainMap *mainMap) {

    Gfx *dl;
    Gfx *startingPositionDl;

    u16 i;
    u16 count;
    u16 gridIndex;
    u32 index;

    s16 tileOffX;
    s16 tileOffZ;

    f32 cameraOffsetX;
    f32 cameraOffsetZ;
    f32 worldX;
    f32 worldZ;

    index = gGraphicsBufferIndex;

    cameraOffsetX = mainMap->mapCameraView.cameraTileX * mainMap->mapGrid.tileSizeX;
    cameraOffsetZ = mainMap->mapCameraView.cameraTileZ * mainMap->mapGrid.tileSizeZ;

    tileOffX = (-(mainMap->mapGrid.mapWidth * mainMap->mapGrid.tileSizeX) / 2)
             + (mainMap->mapGrid.tileSizeX * mainMap->groundObjects.x) - (mainMap->mapGrid.tileSizeX / 2);

    tileOffZ = (-(mainMap->mapGrid.mapHeight * mainMap->mapGrid.tileSizeZ) / 2)
             + (mainMap->mapGrid.tileSizeZ * mainMap->groundObjects.z) - (mainMap->mapGrid.tileSizeZ / 2);

    count = 0;

    dl = groundObjectBitmapsDisplayList[index];

    if (mainMap->groundObjects.y) {

        // Fixed world-space Y baseline: no terrain sampling.
        for (i = 0; i < MAX_GROUND_OBJECTS; i++) {

            if (mainMap->groundObjects.spriteIndexToGrid[i] != 0xFFFF) {

                startingPositionDl = dl;

                dl = prepareGroundObjectBitmap(dl, &mainMap->groundObjectBitmaps[i]);

                gridIndex = mainMap->groundObjects.spriteIndexToGrid[i];

                do {

                    if (mainMap->visibilityGrid[gridIndexToTileIndexZ[gridIndex] + mainMap->groundObjects.z][gridIndexToTileIndexX[gridIndex] + mainMap->groundObjects.x]) {

                        dl = renderGroundObject(dl, mainMap, &mainMap->groundObjectBitmaps[i], count);

                        addGroundObjectToSceneGraph(mainMap,
                            tileOffX + (gridIndexToTileIndexX[gridIndex] * 32) + mainMap->groundObjectBitmaps[i].coordinates.x,
                            mainMap->groundObjects.y + mainMap->groundObjectBitmaps[i].coordinates.y,
                            tileOffZ + (gridIndexToTileIndexZ[gridIndex] * 32) + mainMap->groundObjectBitmaps[i].coordinates.z,
                            cameraOffsetX, cameraOffsetZ, startingPositionDl);

                        count++;

                        startingPositionDl = dl;

                    }

                    gridIndex = mainMap->groundObjects.nextGridToSpriteIndex[gridIndex];

                } while (gridIndex != 0xFFFF);

            }

        }

    } else {

        // Terrain-sampling map: use the height cached by setGroundObjects().
        for (i = 0; i < MAX_GROUND_OBJECTS; i++) {

            if (mainMap->groundObjects.spriteIndexToGrid[i] != 0xFFFF) {

                startingPositionDl = dl;

                dl = prepareGroundObjectBitmap(dl, &mainMap->groundObjectBitmaps[i]);

                gridIndex = mainMap->groundObjects.spriteIndexToGrid[i];

                do {

                    if (mainMap->visibilityGrid[gridIndexToTileIndexZ[gridIndex] + mainMap->groundObjects.z][gridIndexToTileIndexX[gridIndex] + mainMap->groundObjects.x]) {

                        dl = renderGroundObject(dl, mainMap, &mainMap->groundObjectBitmaps[i], count);

                        worldX = tileOffX + (gridIndexToTileIndexX[gridIndex] * 32) + mainMap->groundObjectBitmaps[i].coordinates.x;
                        worldZ = tileOffZ + (gridIndexToTileIndexZ[gridIndex] * 32) + mainMap->groundObjectBitmaps[i].coordinates.z - 4.0f;

                        addGroundObjectToSceneGraph(mainMap,
                            worldX,
                            groundObjectTerrainHeightCache[gridIndex],
                            worldZ,
                            cameraOffsetX, cameraOffsetZ, startingPositionDl);

                        count++;

                        startingPositionDl = dl;

                    }

                    gridIndex = mainMap->groundObjects.nextGridToSpriteIndex[gridIndex];

                } while (gridIndex != 0xFFFF);

            }

        }

    }

}
