// CrystalDust's Whirlpool field move, restored in Phase 4 (D51). The Phase 1
// merge dropped it along with the MB_WHIRLPOOL metatile behavior.
#include "global.h"
#include "event_scripts.h"
#include "fldeff.h"
#include "fieldmap.h"
#include "field_effect.h"
#include "field_player_avatar.h"
#include "metatile_behavior.h"
#include "party_menu.h"
#include "script.h"
#include "sound.h"
#include "constants/field_effects.h"
#include "constants/songs.h"

static void FieldCallback_Whirlpool(void)
{
    gFieldEffectArguments[0] = GetCursorSelectionMonId();
    ScriptContext_SetupScript(EventScript_UseWhirlpoolFromPartyMenu);
}

bool32 SetUpFieldMove_Whirlpool(void)
{
    s16 x, y;

    GetXYCoordsOneStepInFrontOfPlayer(&x, &y);
    if (MetatileBehavior_IsWhirlpool(MapGridGetMetatileBehaviorAt(x, y)) == TRUE)
    {
        gFieldCallback2 = FieldCallback_PrepareFadeInFromMenu;
        gPostMenuFieldCallback = FieldCallback_Whirlpool;
        return TRUE;
    }
    return FALSE;
}

// The dispersal itself is handled by EventScript_UseWhirlpool.
static void FieldMove_Whirlpool(void)
{
    FieldEffectActiveListRemove(FLDEFF_USE_WHIRLPOOL);
    ScriptContext_Enable();
}

bool8 FldEff_UseWhirlpool(void)
{
    u8 taskId = CreateFieldMoveTask();
    gTasks[taskId].data[8] = (u32)FieldMove_Whirlpool >> 16;
    gTasks[taskId].data[9] = (u32)FieldMove_Whirlpool;
    return FALSE;
}
