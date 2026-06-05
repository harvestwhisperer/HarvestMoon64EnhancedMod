#include "common.h"

#include "game/player.h"
#include "modding.h"
#include "system/audio.h"
#include "system/controller.h"

#include "mod_config.h"

extern u8 getToolLevel(u8 tool);
extern void playSfx(u16 index);

static u8 s_tool_cycle_slot = 0xFF;
static bool s_should_start_tool_change_animation = FALSE;
static bool s_suppress_tool_change_sfx = FALSE;

extern u8 upgradedToolIndex;
extern u8 upgradedToolLevelIndex;
extern u8 audioTypeTable[];
extern s32 volumesTable[];
extern Addresses sfxRomAddresses[];

#define TOOL_CYCLE_BUTTON_MASK (BUTTON_L | BUTTON_Z)
#define MENU_SELECT_SFX 0
#define TOOL_ACQUISITION_SFX 0x5A

static bool tool_has_acquisition_animation(u8 tool) {
    return tool >= 1 && tool <= 5;
}

static bool cycle_equipped_tool(void) {
    u8 i;
    u8 start = 0;

    if (gPlayer.currentTool != 0) {
        for (i = 0; i < 8; i++) {
            if (gPlayer.toolSlots[i] == gPlayer.currentTool) {
                start = (i + 1) % 8;
                s_tool_cycle_slot = i;
                break;
            }
        }
    }

    if (s_tool_cycle_slot < 8) {
        start = (s_tool_cycle_slot + 1) % 8;
    }

    for (i = 0; i < 8; i++) {
        u8 slot = (start + i) % 8;
        u8 tool = gPlayer.toolSlots[slot];

        if (tool != 0 && tool != gPlayer.currentTool) {
            gPlayer.toolSlots[slot] = gPlayer.currentTool;
            gPlayer.currentTool = tool;
            gPlayer.currentToolLevel = 0;
            gPlayer.toolHeldCounter = 0;
            s_tool_cycle_slot = slot;
            return TRUE;
        }
    }

    if (gPlayer.currentTool == 0) {
        for (i = 0; i < 8; i++) {
            if (gPlayer.toolSlots[i] != 0) {
                gPlayer.currentTool = gPlayer.toolSlots[i];
                gPlayer.toolSlots[i] = 0;
                gPlayer.currentToolLevel = 0;
                gPlayer.toolHeldCounter = 0;
                s_tool_cycle_slot = i;
                return TRUE;
            }
        }
    }

    return FALSE;
}

static void start_tool_change_animation(void) {
    if (!tool_has_acquisition_animation(gPlayer.currentTool)) {
        return;
    }

    upgradedToolIndex = gPlayer.currentTool;
    upgradedToolLevelIndex = getToolLevel(gPlayer.currentTool);
    s_suppress_tool_change_sfx = TRUE;
    setPlayerAction(ACQUIRING_TOOL, ANIM_TOOL_ACQUISITION);
}

static u32 get_tool_cycle_button_pressed(void) {
    return (controllers[CONTROLLER_1].buttonPressed | gControllers[CONTROLLER_1].buttonPressed) & TOOL_CYCLE_BUTTON_MASK;
}

static void clear_tool_cycle_button_pressed(u32 button_mask) {
    controllers[CONTROLLER_1].buttonPressed &= ~button_mask;
    gControllers[CONTROLLER_1].buttonPressed &= ~button_mask;
}

RECOMP_HOOK("handlePlayerInput")
void hm64_qol_handle_player_input_hook(void) {
    u32 button_pressed;

    if (!config_enabled("enable_tool_cycling")) {
        return;
    }

    button_pressed = get_tool_cycle_button_pressed();

    if (!button_pressed) {
        return;
    }

    if (cycle_equipped_tool()) {
        s_should_start_tool_change_animation = TRUE;
        clear_tool_cycle_button_pressed(button_pressed);
        playSfx(MENU_SELECT_SFX);
    }
}

RECOMP_HOOK_RETURN("handlePlayerInput")
void hm64_qol_handle_player_input_return(void) {
    if (!s_should_start_tool_change_animation) {
        return;
    }

    s_should_start_tool_change_animation = FALSE;
    start_tool_change_animation();
}

RECOMP_HOOK_RETURN("updatePlayerAction")
void hm64_qol_update_player_action_return(void) {
    if (gPlayer.actionHandler != ACQUIRING_TOOL) {
        s_suppress_tool_change_sfx = FALSE;
    }
}

RECOMP_PATCH void playSfx(u16 index) {
    if (s_suppress_tool_change_sfx && index == TOOL_ACQUISITION_SFX) {
        s_suppress_tool_change_sfx = FALSE;
        return;
    }

    if (index < TOTAL_SFX) {
        if (audioTypeTable[index] == SFX_TYPE) {
            setSfx(index + 1);
            setSfxVolume(index + 1, volumesTable[index]);
        } else {
            setAudioSequence(audioTypeTable[index], (u8*)sfxRomAddresses[index].romAddrStart, (u8*)sfxRomAddresses[index].romAddrEnd);
            setAudioSequenceVolumes(audioTypeTable[index], volumesTable[index], volumesTable[index]);
        }
    }
}
