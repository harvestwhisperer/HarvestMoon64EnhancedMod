#include "common.h"

#include "game/namingScreen.h"

#include "ld_symbols.h"

#include "system/audio.h"
#include "system/controller.h"
#include "system/dialogue.h"
#include "system/message.h"
#include "system/globalSprites.h"

#include "game/animals.h"
#include "game/cutscenes.h"
#include "game/game.h"
#include "game/gameAudio.h"
#include "game/gameStart.h"
#include "game/gameStatus.h"
#include "game/level.h"
#include "game/npc.h"
#include "game/player.h"
#include "game/time.h"
#include "game/transition.h"

#include "mainLoop.h"

#include "buffers/buffers.h"

#include "assetIndices/cutscenes.h"
#include "assetIndices/dialogues.h"
#include "assetIndices/sequences.h"
#include "assetIndices/sfxs.h"
#include "assetIndices/sprites.h"

#include "modding.h"

extern void loadNameSelectionSprites(void);
extern void deactivateNamingScreenSprites(void);
extern void loadSeasonSelectionSprites(void);
extern void handleSeasonSelectionInput(void);
extern void handleNamingGridInput(void);

// TODO: might not be needed after symbols update
extern void clearSpecialDialogueBit(u16 bitIndex);

RECOMP_PATCH void initializeNamingScreen(u8* arg0, u8 arg1) {

    s32 i = 0;

    namingScreenContext.screenType = arg1;
    namingScreenContext.selectedSeason = 0;

    namingScreenContext.name = arg0;

    for (i = 0; i < NAME_LENGTH; i++) {
        namingScreenContext.name[i] = 0xFF;
    }

    loadNameSelectionSprites();

    setMessageBoxViewSpacePosition(0, 0, -64.0f, 352.0f);
    setMessageBoxInterpolationWithFlags(0, -4, 0);
    setMessageBoxSpriteIndices(0, 1, 0, 0);

    namingScreenContext.dialogueIndex = arg1;

    if (gCurrentAudioSequenceIndex != NAMING_SCREEN_THEME) {
        stopCurrentAudioSequence(NAMING_SCREEN_THEME);
        setCurrentAudioSequence(NAMING_SCREEN_THEME);
        setAudioSequenceVolume(NAMING_SCREEN_THEME, SEQUENCE_VOLUME);
        gCurrentAudioSequenceIndex = NAMING_SCREEN_THEME;
    }

    setMainLoopCallbackFunctionIndex(NAMING_SCREEN);

}

RECOMP_PATCH void namingScreenCallback(void) {

    s32 i;
    s32 temp = (namingScreenContext.flags & NAMING_SCREEN_SCREEN_STATE_MASK) >> NAMING_SCREEN_SCREEN_STATE_SHIFT;
    s32 temp2;

    if (globalSprites[0x90].stateFlags & 0x400) {

        if (namingScreenContext.flags & NAMING_SCREEN_GOTO_SEASON_SELECT) {

            deactivateNamingScreenSprites();

            deactivateMessageBox(3);

            loadSeasonSelectionSprites();

            namingScreenContext.flags &= ~NAMING_SCREEN_GOTO_SEASON_SELECT;

            return;

        }

        if (namingScreenContext.flags & NAMING_SCREEN_GOTO_SEASON_CONFIRM) {

            temp2 = namingScreenContext.selectedSeason;

            deactivateNamingScreenSprites();

            dmaSprite(0x80, &_namingScreen1TextureSegmentRomStart, &_namingScreen1TextureSegmentRomEnd, &_namingScreen1AssetsIndexSegmentRomStart, &_namingScreen1AssetsIndexSegmentRomEnd, NULL, NULL, (u8*)NAMING_SCREEN_CONFIRMATION_SCREEN_TEXTURE_BUFFER, NULL, (u16*)NAMING_SCREEN_CONFIRMATION_SCREEN_PALETTE_BUFFER, (AnimationFrameMetadata*)NAMING_SCREEN_CONFIRMATION_SCREEN_ANIMATION_FRAME_METADATA_BUFFER, (u32*)NAMING_SCREEN_CONFIRMATION_SCREEN_TEXTURE_TO_PALETTE_BUFFER, NULL, 0, FALSE);
            dmaSprite(0x84, &_namingScreen1TextureSegmentRomStart, &_namingScreen1TextureSegmentRomEnd, &_namingScreen1AssetsIndexSegmentRomStart, &_namingScreen1AssetsIndexSegmentRomEnd, NULL, NULL, (u8*)NAMING_SCREEN_CONFIRMATION_SCREEN_TEXTURE_BUFFER, NULL, (u16*)NAMING_SCREEN_CONFIRMATION_SCREEN_PALETTE_BUFFER, (AnimationFrameMetadata*)NAMING_SCREEN_CONFIRMATION_SCREEN_ANIMATION_FRAME_METADATA_BUFFER, (u32*)NAMING_SCREEN_CONFIRMATION_SCREEN_TEXTURE_TO_PALETTE_BUFFER, NULL, 0, FALSE);

            dmaSprite(LANDSCAPE_BACKGROUND, &_namingScreenBackgroundTextureSegmentRomStart, &_namingScreenBackgroundTextureSegmentRomEnd, &_namingScreenBackgroundAssetsIndexSegmentRomStart, &_namingScreenBackgroundAssetsIndexSegmentRomEnd, NULL, NULL, (u8*)NAMING_SCREEN_BACKGROUND_TEXTURE_BUFFER, NULL, (u16*)NAMING_SCREEN_BACKGROUND_PALETTE_BUFFER, (AnimationFrameMetadata*)NAMING_SCREEN_BACKGROUND_ANIMATION_FRAME_METADATA_BUFFER, (u32*)NAMING_SCREEN_BACKGROUND_TEXTURE_TO_PALETTE_LOOKUP_BUFFER, NULL, 0, FALSE);
            setBilinearFiltering(LANDSCAPE_BACKGROUND, TRUE);
            setSpriteScale(LANDSCAPE_BACKGROUND, 2.0f, 2.0f, 1.0f);

            startSpriteAnimation(0x80, 1, 0);
            startSpriteAnimation(0x84, 2, 0);
            startSpriteAnimation(LANDSCAPE_BACKGROUND, 0, 0);

            setSpriteColor(0x80, 0, 0, 0, 0);
            setSpriteColor(0x84, 0, 0, 0, 0);
            setSpriteColor(LANDSCAPE_BACKGROUND, 0, 0, 0, 0);

            updateSpriteRGBA(0x80, 255, 255, 255, 255, 8);
            updateSpriteRGBA(0x84, 255, 255, 255, 255, 8);
            updateSpriteRGBA(LANDSCAPE_BACKGROUND, 255, 255, 255, 255, 8);

            namingScreenContext.dialogueIndex = 10;
            namingScreenContext.flags &= ~NAMING_SCREEN_SCREEN_STATE_MASK;
            namingScreenContext.flags |= (NAMING_SCREEN_STATE_SEASON_CONFIRM << NAMING_SCREEN_SCREEN_STATE_SHIFT);

            initializeEmptyMessageBox(3, (u8*)MESSAGE_BOX_3_TEXT_BUFFER);
            setMessageBoxViewSpacePosition(3, 4.0f, 56.0f, 30.0f);
            setMessageBoxLineAndRowSizes(3, 6, 1);
            setMessageBoxSpacing(3, 0, 2);
            setMessageBoxFont(3, 14, 14, (u8*)FONT_TEXTURE_BUFFER, FONT_PALETTE_1_BUFFER);
            setMessageBoxInterpolationWithFlags(3, 1, 1);
            setMessageBoxSpriteIndices(3, 0xFF, 0xFF, 0xFF);
            initializeMessageBox(3, NAMING_SCREEN_TEXT_INDEX, 13, MESSAGE_BOX_MODE_NO_INPUT);
            setMessageBoxRGBA(3, 0, 0, 0, 0);
            setMessageBoxRGBAWithTransition(3, 255, 255, 255, 255, 8);

            initializeEmptyMessageBox(4, (u8*)MESSAGE_BOX_4_TEXT_BUFFER);
            setMessageBoxViewSpacePosition(4, 4.0f, -10.0f, 30.0f);
            setMessageBoxLineAndRowSizes(4, 6, 1);
            setMessageBoxSpacing(4, 0, 2);
            setMessageBoxFont(4, 14, 14, (u8*)FONT_TEXTURE_BUFFER, FONT_PALETTE_1_BUFFER);
            setMessageBoxInterpolationWithFlags(4, 1, 1);
            setMessageBoxSpriteIndices(4, 0xFF, 0xFF, 0xFF);
            initializeMessageBox(4, NAMING_SCREEN_TEXT_INDEX, temp2 + 14, MESSAGE_BOX_MODE_NO_INPUT);
            setMessageBoxRGBA(4, 0, 0, 0, 0);
            setMessageBoxRGBAWithTransition(4, 255, 255, 255, 255, 8);

            namingScreenContext.flags &= ~NAMING_SCREEN_GOTO_SEASON_CONFIRM;

            return;

        }

        if (namingScreenContext.flags & NAMING_SCREEN_RETURN_TO_NAMING) {

            deactivateNamingScreenSprites();
            deactivateMessageBox(3);
            deactivateMessageBox(4);
            loadNameSelectionSprites();

            namingScreenContext.dialogueIndex = 12;
            namingScreenContext.flags &= ~NAMING_SCREEN_RETURN_TO_NAMING;

            return;

        }

         if (namingScreenContext.flags & NAMING_SCREEN_CONFIRM_AND_EXIT) {

            setGameVariableString(0, gPlayer.name, 6);

            deactivateNamingScreenSprites();
            deactivateMessageBox(3);
            deactivateMessageBox(4);
            setMessageBoxSpriteIndices(0, 0, 0, 0);
            setMessageBoxViewSpacePosition(0, 24.0f, -64.0f, 352.0f);

            switch (namingScreenContext.screenType) {

                 case NAMING_SCREEN_TYPE_PLAYER:
                    initializeNamingScreen(gFarmName, NAMING_SCREEN_TYPE_FARM);
                    return;

                 case NAMING_SCREEN_TYPE_FARM:
                    initializeNamingScreen(dogInfo.name, NAMING_SCREEN_TYPE_DOG);
                    return;

                 case NAMING_SCREEN_TYPE_DOG:
                    startGame();
                    return;

                 case NAMING_SCREEN_TYPE_HORSE:
                    setLevelAudio(gBaseMapIndex, gSeason, gHour);
                    gCutsceneIndex = CUTSCENE_RANCH_HORSE_NAMING_FOLLOWUP;
                    loadCutscene(FALSE);
                    exitOverlayScreen();
                    setLevelLighting(8, MAIN_GAME);

                    return;

                 case NAMING_SCREEN_TYPE_BABY:

                    setLevelAudio(gBaseMapIndex, gSeason, gHour);

                    switch (gWife) {
                        case MARIA:
                            gCutsceneIndex = CUTSCENE_HOUSE_MARIA_AFTER_BABY_NAMING;
                            clearSpecialDialogueBit(MARIA_PREGNANT_DIALOGUE);
                            break;
                        case POPURI:
                            gCutsceneIndex = CUTSCENE_HOUSE_POPURI_AFTER_BABY_NAMING;
                            clearSpecialDialogueBit(POPURI_PREGNANT_DIALOGUE);
                            break;
                        case ELLI:
                            gCutsceneIndex = CUTSCENE_HOUSE_ELLI_AFTER_BABY_NAMING;
                            clearSpecialDialogueBit(ELLI_PREGNANT_DIALOGUE);
                            break;
                        case ANN:
                            gCutsceneIndex = CUTSCENE_HOUSE_ANN_AFTER_BABY_NAMING;
                            clearSpecialDialogueBit(ANN_PREGNANT_DIALOGUE);
                            break;
                        case KAREN:
                            gCutsceneIndex = CUTSCENE_HOUSE_KAREN_AFTER_BABY_NAMING;
                            clearSpecialDialogueBit(KAREN_PREGNANT_DIALOGUE);
                            break;
                        }

                        loadCutscene(FALSE);

                        exitOverlayScreen();
                        setLevelLighting(8, MAIN_GAME);

                    return;

                 case NAMING_SCREEN_TYPE_COW:
                 case NAMING_SCREEN_TYPE_SHEEP:
                 case NAMING_SCREEN_TYPE_CHICKEN:
                    setLevelAudio(gBaseMapIndex, gSeason, gHour);
                    exitOverlayScreen();
                    setLevelLighting(8, MAIN_GAME);
                    return;

             }

        } else {

            if (namingScreenContext.flags & NAMING_SCREEN_AUDIO_FADE_TRANSITION) {
                namingScreenContext.flags &= ~NAMING_SCREEN_AUDIO_FADE_TRANSITION;
                namingScreenContext.flags |= NAMING_SCREEN_CONFIRM_AND_EXIT;
                return;
            }

            if (namingScreenContext.dialogueIndex != 0xFFFF) {
                initializeDialogueSession(0, DIALOGUE_NAMING_SCREEN, namingScreenContext.dialogueIndex, 0);
                namingScreenContext.dialogueIndex = 0xFFFF;
                return;
            }

            if (dialogues[0].sessionManager.flags & 4) {

                    switch (temp) {

                        case NAMING_SCREEN_STATE_NAMING_GRID:
                            handleNamingGridInput();
                            return;

                        case NAMING_SCREEN_STATE_SEASON_SELECT:
                            handleSeasonSelectionInput();
                            return;

                        case NAMING_SCREEN_STATE_SEASON_CONFIRM:

                            if (dialogues[0].sessionManager.selectedMenuRow == 0) {

                                namingScreenContext.flags |= NAMING_SCREEN_CONFIRM_AND_EXIT;

                                setMessageBoxRGBAWithTransition(3, 0, 0, 0, 0, 8);
                                setMessageBoxRGBAWithTransition(4, 0, 0, 0, 0, 8);
                                updateSpriteRGBA(0x80, 0, 0, 0, 0, 8);
                                updateSpriteRGBA(0x84, 0, 0, 0, 0, 8);
                                updateSpriteRGBA(LANDSCAPE_BACKGROUND, 0, 0, 0, 0, 8);

                                gPlayerBirthdaySeason = namingScreenContext.selectedSeason + 1;

                                return;

                            }

                            namingScreenContext.flags |= NAMING_SCREEN_RETURN_TO_NAMING;

                            setMessageBoxRGBAWithTransition(3, 0, 0, 0, 0, 8);
                            setMessageBoxRGBAWithTransition(4, 0, 0, 0, 0, 8);
                            updateSpriteRGBA(0x80, 0, 0, 0, 0, 8);
                            updateSpriteRGBA(0x84, 0, 0, 0, 0, 8);
                            updateSpriteRGBA(LANDSCAPE_BACKGROUND, 0, 0, 0, 0, 8);

                            return;

                        case NAMING_SCREEN_STATE_ANIMAL_CONFIRM:

                            if (dialogues[0].sessionManager.selectedMenuRow == 0) {

                                namingScreenContext.flags |= NAMING_SCREEN_CONFIRM_AND_EXIT;

                                setMessageBoxRGBAWithTransition(3, 0, 0, 0, 0, 8);

                                updateSpriteRGBA(0x80, 0, 0, 0, 0, 8);
                                updateSpriteRGBA(0x81, 0, 0, 0, 0, 8);
                                updateSpriteRGBA(0x82, 0, 0, 0, 0, 8);
                                updateSpriteRGBA(0x8F, 0, 0, 0, 0, 8);
                                updateSpriteRGBA(LANDSCAPE_BACKGROUND, 0, 0, 0, 0, 8);
                                updateSpriteRGBA(0x83, 0, 0, 0, 0, 8);
                                updateSpriteRGBA(0x84, 0, 0, 0, 0, 8);
                                updateSpriteRGBA(0x85, 0, 0, 0, 0, 8);
                                updateSpriteRGBA(0x86, 0, 0, 0, 0, 8);
                                updateSpriteRGBA(0x87, 0, 0, 0, 0, 8);
                                updateSpriteRGBA(0x88, 0, 0, 0, 0, 8);
                                updateSpriteRGBA(0x89, 0, 0, 0, 0, 8);
                                updateSpriteRGBA(0x8A, 0, 0, 0, 0, 8);
                                updateSpriteRGBA(0x8B, 0, 0, 0, 0, 8);
                                updateSpriteRGBA(0x8C, 0, 0, 0, 0, 8);
                                updateSpriteRGBA(0x8D, 0, 0, 0, 0, 8);
                                updateSpriteRGBA(0x8E, 0, 0, 0, 0, 8);
                                updateSpriteRGBA(0x91, 0, 0, 0, 0, 8);

                                if (namingScreenContext.screenType >= NAMING_SCREEN_TYPE_DOG) {

                                    stopAudioSequenceWithDefaultFadeOut(NAMING_SCREEN_THEME);

                                    namingScreenContext.flags &= ~NAMING_SCREEN_CONFIRM_AND_EXIT;
                                    namingScreenContext.flags |= NAMING_SCREEN_AUDIO_FADE_TRANSITION;

                                }

                            } else {
                                namingScreenContext.flags &= ~NAMING_SCREEN_SCREEN_STATE_MASK;
                                setSpritePaletteIndex(0x91, 4);
                            }

                        return;

                    default:
                        return;

                }

            }
        }

}

}
