#include "global.h"
#include "script.h"
#include "event_data.h"
#include "field_screen_effect.h"
#include "mystery_gift.h"
#include "random.h"
#include "task.h"
#include "trainer_see.h"
#include "util.h"
#include "constants/event_objects.h"
#include "constants/flags.h"
#include "constants/map_scripts.h"
#include "constants/script_commands.h"
#include "field_message_box.h"

#include "dexnav.h"

// CrystalDust's field specials need these (D48).
#include "field_camera.h"
#include "field_effect_helpers.h"
#include "fieldmap.h"
#include "field_player_avatar.h"
#include "item.h"
#include "party_menu.h"
#include "pokemon_storage_system.h"
#include "script_pokemon_util.h"
#include "string_util.h"
#include "tv.h"
#include "palette.h"
#include "constants/items.h"
#include "constants/layouts.h"
#include "constants/moves.h"
#include "constants/room_decor.h"
#include "day_night.h"
#include "event_object_movement.h"
#include "pokemon_storage_system.h"

extern const u16 gObjectEventPal_Eusine[];
extern const u16 gObjectEventPal_Murkrow[];
extern const u16 gObjectEventPal_ShieldDecorations[];

#include "constants/metatile_labels.h"


#define RAM_SCRIPT_MAGIC 51

enum {
    SCRIPT_MODE_STOPPED,
    SCRIPT_MODE_BYTECODE,
    SCRIPT_MODE_NATIVE,
};

enum {
    CONTEXT_RUNNING,
    CONTEXT_WAITING,
    CONTEXT_SHUTDOWN,
};

extern const u8 *gRamScriptRetAddr;

static u8 sGlobalScriptContextStatus;
static struct ScriptContext sGlobalScriptContext;
static struct ScriptContext sImmediateScriptContext;
static bool8 sLockFieldControls;
EWRAM_DATA u8 gMsgIsSignPost = FALSE;
EWRAM_DATA u8 gMsgBoxIsCancelable = FALSE;

extern ScrCmdFunc gScriptCmdTable[];
extern ScrCmdFunc gScriptCmdTableEnd[];

void InitScriptStack(struct ScriptStack *stk)
{
    stk->stackDepth = 0;
    memset(stk->stack, 0, (int)ARRAY_COUNT(stk->stack) * sizeof(u8*));
}

void InitScriptContext(struct ScriptContext *ctx, void *cmdTable, void *cmdTableEnd)
{
    s32 i;

    ctx->mode = SCRIPT_MODE_STOPPED;
    ctx->scriptPtr = NULL;
    ctx->stackDepth = 0;
    ctx->nativePtr = NULL;
    ctx->cmdTable = cmdTable;
    ctx->cmdTableEnd = cmdTableEnd;

    for (i = 0; i < (int)ARRAY_COUNT(ctx->data); i++)
        ctx->data[i] = 0;

    for (i = 0; i < (int)ARRAY_COUNT(ctx->stack); i++)
        ctx->stack[i] = NULL;

    ctx->breakOnTrainerBattle = FALSE;
}

u8 SetupBytecodeScript(struct ScriptContext *ctx, const u8 *ptr)
{
    ctx->scriptPtr = ptr;
    ctx->mode = SCRIPT_MODE_BYTECODE;
    return 1;
}

void SetupNativeScript(struct ScriptContext *ctx, bool8 (*ptr)(void))
{
    ctx->mode = SCRIPT_MODE_NATIVE;
    ctx->nativePtr = ptr;
}

void StopScript(struct ScriptContext *ctx)
{
    assertf(!FuncIsActiveTask(Task_WarpAndLoadMap), "Leaving script while a warp is in progress: try adding a waitstate");
    ctx->mode = SCRIPT_MODE_STOPPED;
    ctx->scriptPtr = NULL;
}

bool8 RunScriptCommand(struct ScriptContext *ctx)
{
    switch (ctx->mode)
    {
    case SCRIPT_MODE_STOPPED:
        return FALSE;
    case SCRIPT_MODE_NATIVE:
        // Try to call a function in C
        // Continue to bytecode if no function or it returns TRUE
        if (ctx->nativePtr)
        {
            if (ctx->nativePtr() == TRUE)
                ctx->mode = SCRIPT_MODE_BYTECODE;
            return TRUE;
        }
        ctx->mode = SCRIPT_MODE_BYTECODE;
        // fallthrough
    case SCRIPT_MODE_BYTECODE:
        while (1)
        {
            u8 cmdCode;
            ScrCmdFunc *func;

            if (ctx->scriptPtr == NULL)
            {
                ctx->mode = SCRIPT_MODE_STOPPED;
                return FALSE;
            }

            cmdCode = *(ctx->scriptPtr);
            ctx->scriptPtr++;
            func = &ctx->cmdTable[cmdCode];

            if (func >= ctx->cmdTableEnd)
            {
                ctx->mode = SCRIPT_MODE_STOPPED;
                return FALSE;
            }

            if ((*func)(ctx) == TRUE)
                return TRUE;
        }
    }

    return TRUE;
}

bool8 ScriptStackPush(struct ScriptStack *stk, const u8 *ptr)
{
    if (stk->stackDepth + 1 >= (int)ARRAY_COUNT(stk->stack))
    {
        return FALSE;
    }
    else
    {
        stk->stack[stk->stackDepth] = ptr;
        stk->stackDepth++;
        return TRUE;
    }
}

bool8 ScriptPush(struct ScriptContext *ctx, const u8 *ptr)
{
    if (ctx->stackDepth + 1 >= (int)ARRAY_COUNT(ctx->stack))
    {
        return TRUE;
    }
    else
    {
        ctx->stack[ctx->stackDepth] = ptr;
        ctx->stackDepth++;
        return FALSE;
    }
}

const u8 *ScriptStackPop(struct ScriptStack *stk)
{
    if (stk->stackDepth == 0)
        return NULL;

    stk->stackDepth--;
    return stk->stack[stk->stackDepth];
}

const u8 *ScriptPop(struct ScriptContext *ctx)
{
    if (ctx->stackDepth == 0)
        return NULL;

    ctx->stackDepth--;
    return ctx->stack[ctx->stackDepth];
}

void ScriptJump(struct ScriptContext *ctx, const u8 *ptr)
{
    assertf(ptr != NULL, "goto to NULL");
    ctx->scriptPtr = ptr;
}

void ScriptCall(struct ScriptContext *ctx, const u8 *ptr)
{
    assertf(ptr != NULL, "call to NULL")
    {
        // HINT: Returning without having pushed the current location is
        // equivalent to branching to a script that just contains
        // 'return'.
        return;
    }

    bool32 failed = ScriptPush(ctx, ctx->scriptPtr);
    assertf(!failed, "could not push %p to %p", ptr, ctx)
    {
        return;
    }

    ctx->scriptPtr = ptr;
}

void ScriptReturn(struct ScriptContext *ctx)
{
    ctx->scriptPtr = ScriptPop(ctx);
}

u16 ScriptReadHalfword(struct ScriptContext *ctx)
{
    u16 value = *(ctx->scriptPtr++);
    value |= *(ctx->scriptPtr++) << 8;
    return value;
}

u16 ScriptPeekHalfword(struct ScriptContext *ctx)
{
    u16 value = *(ctx->scriptPtr);
    value |= *(ctx->scriptPtr + 1) << 8;
    return value;
}

u32 ScriptReadWord(struct ScriptContext *ctx)
{
    u32 value0 = *(ctx->scriptPtr++);
    u32 value1 = *(ctx->scriptPtr++);
    u32 value2 = *(ctx->scriptPtr++);
    u32 value3 = *(ctx->scriptPtr++);
    return (((((value3 << 8) + value2) << 8) + value1) << 8) + value0;
}

u32 ScriptPeekWord(struct ScriptContext *ctx)
{
    u32 value0 = *(ctx->scriptPtr);
    u32 value1 = *(ctx->scriptPtr + 1);
    u32 value2 = *(ctx->scriptPtr + 2);
    u32 value3 = *(ctx->scriptPtr + 3);
    return (((((value3 << 8) + value2) << 8) + value1) << 8) + value0;
}

void LockPlayerFieldControls(void)
{
    sLockFieldControls = TRUE;
    EndDexNavSearch();
}

void UnlockPlayerFieldControls(void)
{
    sLockFieldControls = FALSE;
}

bool8 ArePlayerFieldControlsLocked(void)
{
    return sLockFieldControls;
}

// The ScriptContext_* functions work with the primary script context,
// which yields control back to native code should the script make a wait call.

// Checks if the global script context is able to be run right now.
bool8 ScriptContext_IsEnabled(void)
{
    if (sGlobalScriptContextStatus == CONTEXT_RUNNING)
        return TRUE;
    else
        return FALSE;
}

// Re-initializes the global script context to zero.
void ScriptContext_Init(void)
{
    InitScriptContext(&sGlobalScriptContext, gScriptCmdTable, gScriptCmdTableEnd);
    sGlobalScriptContextStatus = CONTEXT_SHUTDOWN;
}

// Runs the script until the script makes a wait* call, then returns true if
// there's more script to run, or false if the script has hit the end.
// This function also returns false if the context is finished
// or waiting (after a call to _Stop)
bool8 ScriptContext_RunScript(void)
{
    if (sGlobalScriptContextStatus == CONTEXT_SHUTDOWN)
        return FALSE;

    if (sGlobalScriptContextStatus == CONTEXT_WAITING)
        return FALSE;

    LockPlayerFieldControls();

    if (!RunScriptCommand(&sGlobalScriptContext))
    {
        sGlobalScriptContextStatus = CONTEXT_SHUTDOWN;
        UnlockPlayerFieldControls();
        return FALSE;
    }

    return TRUE;
}

// Sets up a new script in the global context and enables the context
void ScriptContext_SetupScript(const u8 *ptr)
{
    InitScriptContext(&sGlobalScriptContext, gScriptCmdTable, gScriptCmdTableEnd);
    SetupBytecodeScript(&sGlobalScriptContext, ptr);
    LockPlayerFieldControls();
    if (OW_FOLLOWERS_SCRIPT_MOVEMENT)
        FlagSet(FLAG_SAFE_FOLLOWER_MOVEMENT);
    sGlobalScriptContextStatus = CONTEXT_RUNNING;
}

// Moves a script from a local context to the global context and enables it.
void ScriptContext_ContinueScript(struct ScriptContext *ctx)
{
    sGlobalScriptContext = *ctx;
    LockPlayerFieldControls();
    sGlobalScriptContextStatus = CONTEXT_RUNNING;
}

// Puts the script into waiting mode; usually called from a wait* script command.
void ScriptContext_Stop(void)
{
    sGlobalScriptContextStatus = CONTEXT_WAITING;
}

// Puts the script into running mode.
void ScriptContext_Enable(void)
{
    sGlobalScriptContextStatus = CONTEXT_RUNNING;
    LockPlayerFieldControls();
}

void ScriptContext_SetupContextFromStack(struct ScriptStack *stk, struct ScriptContext *ctx)
{
    const u8 *ptr;

    while ((ptr = ScriptStackPop(stk)) != NULL)
    {
        if (ScriptPush(ctx, ptr)) {
            errorf("Failed to push %p to %p.", ptr, ctx);
        }
    }

    ctx->scriptPtr = ScriptPop(ctx);
    ctx->mode = SCRIPT_MODE_BYTECODE;

    if (OW_FOLLOWERS_SCRIPT_MOVEMENT)
        FlagSet(FLAG_SAFE_FOLLOWER_MOVEMENT);
}

void ScriptContext_SetupGlobalContextFromStack(struct ScriptStack *stk)
{
    ScriptContext_SetupContextFromStack(stk, &sGlobalScriptContext);
}

// Sets up and runs a script in its own context immediately. The script will be
// finished when this function returns. Used mainly by all of the map header
// scripts (except the frame table scripts).
void RunScriptImmediately(const u8 *ptr)
{
    InitScriptContext(&sImmediateScriptContext, gScriptCmdTable, gScriptCmdTableEnd);
    SetupBytecodeScript(&sImmediateScriptContext, ptr);
    while (RunScriptCommand(&sImmediateScriptContext) == TRUE);
}

const u8 *MapHeaderGetScriptTable(u8 tag)
{
    const u8 *mapScripts = gMapHeader.mapScripts;

    if (!mapScripts)
        return NULL;

    while (1)
    {
        if (!*mapScripts)
            return NULL;
        if (*mapScripts == tag)
        {
            mapScripts++;
            return T2_READ_PTR(mapScripts);
        }
        mapScripts += 5;
    }
}

void MapHeaderRunScriptType(u8 tag)
{
    const u8 *ptr = MapHeaderGetScriptTable(tag);
    if (ptr)
        RunScriptImmediately(ptr);
}

const u8 *MapHeaderCheckScriptTable(u8 tag)
{
    const u8 *ptr = MapHeaderGetScriptTable(tag);

    if (!ptr)
        return NULL;

    while (1)
    {
        u16 varIndex1;
        u16 varIndex2;

        // Read first var (or .2byte terminal value)
        varIndex1 = T1_READ_16(ptr);
        if (!varIndex1)
            return NULL; // Reached end of table
        ptr += 2;

        // Read second var
        varIndex2 = T1_READ_16(ptr);
        ptr += 2;

        // Run map script if vars are equal
        if (VarGet(varIndex1) == VarGet(varIndex2))
        {
            const u8 *mapScript = T2_READ_PTR(ptr);
            if (!Script_HasNoEffect(mapScript))
                return mapScript;
        }

        ptr += 4;
    }
}

void RunOnLoadMapScript(void)
{
    MapHeaderRunScriptType(MAP_SCRIPT_ON_LOAD);
}

void RunOnTransitionMapScript(void)
{
    MapHeaderRunScriptType(MAP_SCRIPT_ON_TRANSITION);
}

void RunOnResumeMapScript(void)
{
    MapHeaderRunScriptType(MAP_SCRIPT_ON_RESUME);
}

void RunOnReturnToFieldMapScript(void)
{
    MapHeaderRunScriptType(MAP_SCRIPT_ON_RETURN_TO_FIELD);
}

void RunOnDiveWarpMapScript(void)
{
    MapHeaderRunScriptType(MAP_SCRIPT_ON_DIVE_WARP);
}

bool8 TryRunOnFrameMapScript(void)
{
    const u8 *ptr = MapHeaderCheckScriptTable(MAP_SCRIPT_ON_FRAME_TABLE);

    if (!ptr)
        return FALSE;

    ScriptContext_SetupScript(ptr);
    return TRUE;
}

void TryRunOnWarpIntoMapScript(void)
{
    const u8 *ptr = MapHeaderCheckScriptTable(MAP_SCRIPT_ON_WARP_INTO_MAP_TABLE);
    if (ptr)
        RunScriptImmediately(ptr);
}

u32 CalculateRamScriptChecksum(void)
{
#if FREE_MYSTERY_EVENT_BUFFERS == FALSE
    return CalcCRC16WithTable((u8 *)(&gSaveBlock1Ptr->ramScript.data), sizeof(gSaveBlock1Ptr->ramScript.data));
#else
    return 0;
#endif //FREE_MYSTERY_EVENT_BUFFERS
}

void ClearRamScript(void)
{
#if FREE_MYSTERY_EVENT_BUFFERS == FALSE
    CpuFill32(0, &gSaveBlock1Ptr->ramScript, sizeof(struct RamScript));
#endif //FREE_MYSTERY_EVENT_BUFFERS
}

bool8 InitRamScript(const u8 *script, u16 scriptSize, u8 mapGroup, u8 mapNum, u8 localId)
{
#if FREE_MYSTERY_EVENT_BUFFERS == FALSE
    struct RamScriptData *scriptData = &gSaveBlock1Ptr->ramScript.data;

    ClearRamScript();

    if (scriptSize > sizeof(scriptData->script))
        return FALSE;

    scriptData->magic = RAM_SCRIPT_MAGIC;
    scriptData->mapGroup = mapGroup;
    scriptData->mapNum = mapNum;
    scriptData->localId = localId;
    memcpy(scriptData->script, script, scriptSize);
    gSaveBlock1Ptr->ramScript.checksum = CalculateRamScriptChecksum();
    return TRUE;
#else
    return FALSE;
#endif //FREE_MYSTERY_EVENT_BUFFERS
}

const u8 *GetRamScript(u8 localId, const u8 *script)
{
#if FREE_MYSTERY_EVENT_BUFFERS == FALSE
    struct RamScriptData *scriptData = &gSaveBlock1Ptr->ramScript.data;
    gRamScriptRetAddr = NULL;
    if (scriptData->magic != RAM_SCRIPT_MAGIC)
        return script;
    if (scriptData->mapGroup != gSaveBlock1Ptr->location.mapGroup)
        return script;
    if (scriptData->mapNum != gSaveBlock1Ptr->location.mapNum)
        return script;
    if (scriptData->localId != localId)
        return script;
    if (CalculateRamScriptChecksum() != gSaveBlock1Ptr->ramScript.checksum)
    {
        ClearRamScript();
        return script;
    }
    else
    {
        gRamScriptRetAddr = script;
        return scriptData->script;
    }
#else
    return script;
#endif //FREE_MYSTERY_EVENT_BUFFERS
}

#define NO_OBJECT LOCALID_PLAYER

bool32 ValidateSavedRamScript(void)
{
#if FREE_MYSTERY_EVENT_BUFFERS == FALSE
    struct RamScriptData *scriptData = &gSaveBlock1Ptr->ramScript.data;
    if (scriptData->magic != RAM_SCRIPT_MAGIC)
        return FALSE;
    if (scriptData->mapGroup != MAP_GROUP(MAP_UNDEFINED))
        return FALSE;
    if (scriptData->mapNum != MAP_NUM(MAP_UNDEFINED))
        return FALSE;
    if (scriptData->localId != NO_OBJECT)
        return FALSE;
    if (CalculateRamScriptChecksum() != gSaveBlock1Ptr->ramScript.checksum)
        return FALSE;
    return TRUE;
#else
    return FALSE;
#endif //FREE_MYSTERY_EVENT_BUFFERS
}

u8 *GetSavedRamScriptIfValid(void)
{
#if FREE_MYSTERY_EVENT_BUFFERS == FALSE
    struct RamScriptData *scriptData = &gSaveBlock1Ptr->ramScript.data;
    if (!ValidateSavedWonderCard())
        return NULL;
    if (scriptData->magic != RAM_SCRIPT_MAGIC)
        return NULL;
    if (scriptData->mapGroup != MAP_GROUP(MAP_UNDEFINED))
        return NULL;
    if (scriptData->mapNum != MAP_NUM(MAP_UNDEFINED))
        return NULL;
    if (scriptData->localId != NO_OBJECT)
        return NULL;
    if (CalculateRamScriptChecksum() != gSaveBlock1Ptr->ramScript.checksum)
    {
        ClearRamScript();
        return NULL;
    }
    else
    {
        return scriptData->script;
    }
#else
    return NULL;
#endif //FREE_MYSTERY_EVENT_BUFFERS
}

void InitRamScript_NoObjectEvent(u8 *script, u16 scriptSize)
{
#if FREE_MYSTERY_EVENT_BUFFERS == FALSE
    if (scriptSize > sizeof(gSaveBlock1Ptr->ramScript.data.script))
        scriptSize = sizeof(gSaveBlock1Ptr->ramScript.data.script);
    InitRamScript(script, scriptSize, MAP_GROUP(MAP_UNDEFINED), MAP_NUM(MAP_UNDEFINED), NO_OBJECT);
#endif //FREE_MYSTERY_EVENT_BUFFERS
}

bool8 LoadTrainerObjectScript(void)
{
    sGlobalScriptContext.scriptPtr = gApproachingTrainers[gNoOfApproachingTrainers - 1].trainerScriptPtr;
    return TRUE;
}

struct ScriptEffectContext {
    u32 breakOn;
    intptr_t breakTo[5];
    const u8 *nextCmd;
};

struct ScriptEffectContext *gScriptEffectContext = NULL;

static bool32 Script_IsEffectInstrumentedCommand(ScrCmdFunc func)
{
    // In ROM mirror 1.
    return (((uintptr_t)func) & 0xE000000) == 0xA000000;
}

/* 'setjmp' and 'longjmp' cause link errors, so we use
 * '__builtin_setjmp' and '__builtin_longjmp' instead.
 * See https://gcc.gnu.org/onlinedocs/gcc/Nonlocal-Gotos.html */
static bool32 RunScriptImmediatelyUntilEffect_InternalLoop(struct ScriptContext *ctx)
{
    if (__builtin_setjmp(gScriptEffectContext->breakTo) == 0)
    {
        while (TRUE)
        {
            u32 cmdCode;
            ScrCmdFunc *func;

            gScriptEffectContext->nextCmd = ctx->scriptPtr;

            if (!ctx->scriptPtr)
                return FALSE;

            cmdCode = *ctx->scriptPtr;
            ctx->scriptPtr++;
            func = &ctx->cmdTable[cmdCode];

            // Invalid script command.
            if (func >= ctx->cmdTableEnd)
                return TRUE;

            if (!Script_IsEffectInstrumentedCommand(*func))
                return TRUE;

            // Command which waits for a frame.
            if ((*func)(ctx))
            {
                gScriptEffectContext->nextCmd = ctx->scriptPtr;
                return TRUE;
            }
        }
    }
    else
    {
        return TRUE;
    }
}

void Script_GotoBreak_Internal(void)
{
    __builtin_longjmp(gScriptEffectContext->breakTo, 1);
}

bool32 RunScriptImmediatelyUntilEffect_Internal(u32 effects, const u8 *ptr, struct ScriptContext *ctx)
{
    bool32 result;
    struct ScriptEffectContext seCtx;
    seCtx.breakOn = effects & 0x7FFFFFFF;

    if (ctx == NULL)
        ctx = &sImmediateScriptContext;

    InitScriptContext(ctx, gScriptCmdTable, gScriptCmdTableEnd);
    if (effects & SCREFF_TRAINERBATTLE)
        ctx->breakOnTrainerBattle = TRUE;
    SetupBytecodeScript(ctx, ptr);

    rng_value_t rngValue = gRngValue;
    gScriptEffectContext = &seCtx;
    result = RunScriptImmediatelyUntilEffect_InternalLoop(ctx);
    gScriptEffectContext = NULL;
    gRngValue = rngValue;

    if (result)
        ctx->scriptPtr = seCtx.nextCmd;

    return result;
}

bool32 Script_HasNoEffect(const u8 *ptr)
{
    return !RunScriptImmediatelyUntilEffect(SCREFF_V1 | SCREFF_SAVE | SCREFF_HARDWARE, ptr, NULL);
}

void Script_RequestEffects_Internal(u32 effects)
{
    if (gScriptEffectContext->breakOn & effects)
        __builtin_longjmp(gScriptEffectContext->breakTo, 1);
}

void Script_RequestWriteVar_Internal(u32 varId)
{
    if (varId == 0)
        return;

    if ((!gMapHeader.writeSpecialVarIsEffect)
     && (SPECIAL_VARS_START <= varId && varId <= SPECIAL_VARS_END))
        return;
    Script_RequestEffects(SCREFF_V1 | SCREFF_SAVE);
}

bool32 Script_MatchesCallNative(const u8 *script, void *funcPtr, bool32 requestEffects)
{
    if (script[0] != SCR_OP_CALLNATIVE)
        return FALSE;
    u32 callnativeFunc = (((((script[4] << 8) + script[3]) << 8) + script[2]) << 8) + script[1];
    u32 targetFunc = (u32)funcPtr;
    if (requestEffects)
        targetFunc |= 0xA000000;
    if (callnativeFunc == targetFunc)
        return TRUE;
    return FALSE;
}

bool32 Script_MatchesSpecial(const u8 *script, void *funcPtr)
{
    if (script[0] != SCR_OP_SPECIAL)
        return FALSE;
    typedef u16 (*SpecialFunc)(void);
    extern const SpecialFunc gSpecials[];
    SpecialFunc specialFunc = gSpecials[(script[2] << 8) + script[1]];
    if ((u32)specialFunc == ((u32)funcPtr))
        return TRUE;
    return FALSE;
}


void SetWalkingIntoSignVars(void)
{
    // gWalkAwayFromSignInhibitTimer = 6;
    // sMsgBoxIsCancelable = TRUE;
}

// CrystalDust: counts all 16 badges, Johto and Kanto. NUM_BADGES only spans
// the Johto eight. Used by the radio's Places and People show (D30).
void CountBadges(void)
{
    u32 i;
    u32 numBadges = 0;

    for (i = 0; i < NUM_BADGES; i++)
    {
        if (FlagGet(FLAG_BADGE01_GET + i))
            numBadges++;
        if (FlagGet(FLAG_BADGE09_GET + i))
            numBadges++;
    }

    gSpecialVar_Result = numBadges;
}


// CrystalDust's field specials, restored in Phase 2 (D48).
static u8 sMsgBoxWalkawayDisabled;


void PatchEusinePaletteToSlot11(void)
{
    if(!FlagGet(FLAG_AWAKENED_LEGENDARY_BEASTS))
    {
        gSprites[gObjectEvents[GetObjectEventIdByLocalIdAndMap(1, gSaveBlock1Ptr->location.mapNum, gSaveBlock1Ptr->location.mapGroup)].spriteId].oam.paletteNum = 11;
        LoadPalette(gObjectEventPal_Eusine, OBJ_PLTT_ID(11), PLTT_SIZE_4BPP);
    }
}

void OverrideKimonoGirlsPaletteSlots(void)
{
    u8 mapNum = gSaveBlock1Ptr->location.mapNum;
    u8 mapGroup = gSaveBlock1Ptr->location.mapGroup;

    gSprites[gObjectEvents[GetObjectEventIdByLocalIdAndMap(6, mapNum, mapGroup)].spriteId].oam.paletteNum = 2; //Espeon palette
    gSprites[gObjectEvents[GetObjectEventIdByLocalIdAndMap(7, mapNum, mapGroup)].spriteId].oam.paletteNum = 4; //Umbreon palette
    gSprites[gObjectEvents[GetObjectEventIdByLocalIdAndMap(8, mapNum, mapGroup)].spriteId].oam.paletteNum = 11; //Vaporeon palette
    gSprites[gObjectEvents[GetObjectEventIdByLocalIdAndMap(9, mapNum, mapGroup)].spriteId].oam.paletteNum = 10; //Jolteon palette
}

extern const u16 gObjectEventPal_EspeonKimonoGirl[];
extern const u16 gObjectEventPal_UmbreonKimonoGirl[];
extern const u16 gObjectEventPal_VaporeonKimonoGirl[];
extern const u16 gObjectEventPal_JolteonKimonoGirl[];

void PatchKimonoGirlPalettesToSlots(void)
{
    LoadPalette(gObjectEventPal_EspeonKimonoGirl, OBJ_PLTT_ID(2), PLTT_SIZE_4BPP);
    LoadPalette(gObjectEventPal_UmbreonKimonoGirl, OBJ_PLTT_ID(4), PLTT_SIZE_4BPP);
    LoadPalette(gObjectEventPal_VaporeonKimonoGirl, OBJ_PLTT_ID(11), PLTT_SIZE_4BPP);
    LoadPalette(gObjectEventPal_JolteonKimonoGirl, OBJ_PLTT_ID(10), PLTT_SIZE_4BPP);
}

extern const u16 gObjectEventPal_EnteiAsleep[];
extern const u16 gObjectEventPal_RaikouAsleep[];
extern const u16 gObjectEventPal_SuicuneAsleep[];

void PatchAsleepBeastsPalettesToSlots(void)
{
    LoadPalette(gObjectEventPal_EnteiAsleep, OBJ_PLTT_ID(2), PLTT_SIZE_4BPP);
    LoadPalette(gObjectEventPal_RaikouAsleep, OBJ_PLTT_ID(4), PLTT_SIZE_4BPP);
    LoadPalette(gObjectEventPal_SuicuneAsleep, OBJ_PLTT_ID(3), PLTT_SIZE_4BPP);
}

extern const u16 gObjectEventPal_Npc1[];
extern const u16 gObjectEventPal_Npc2[];
extern const u16 gObjectEventPal_Npc3[];

void AwakenEntei(void)
{
    LoadPalette(gObjectEventPal_Npc1, OBJ_PLTT_ID(2), PLTT_SIZE_4BPP);
}

void AwakenRaikou(void)
{
    LoadPalette(gObjectEventPal_Npc3, OBJ_PLTT_ID(4), PLTT_SIZE_4BPP);
}

void AwakenSuicune(void)
{
    LoadPalette(gObjectEventPal_Npc2, OBJ_PLTT_ID(3), PLTT_SIZE_4BPP);
}

void IsHoOhInFirstInParty(void)
{
    if(GetMonData(&gPlayerParty[0], MON_DATA_SPECIES_OR_EGG) == SPECIES_HO_OH)
        gSpecialVar_Result = TRUE;
    else
        gSpecialVar_Result = FALSE;
}

void GiveEnemyMonSelfdestruct(void)
{
    u16 move = MOVE_SELF_DESTRUCT;
    SetMonData(&gEnemyParty[0], MON_DATA_MOVE1, &move);
}

void PatchMurkrowPaletteToSlot10(void)
{
    LoadPalette(gObjectEventPal_Murkrow, OBJ_PLTT_ID(10), PLTT_SIZE_4BPP);
}

static bool32 MonHasPlayersOT(struct Pokemon *mon, u32 playerID)
{
    if (playerID != GetMonData(mon, MON_DATA_OT_ID, NULL))
        return FALSE;

    GetMonData(mon, MON_DATA_OT_NAME, gStringVar1);
    if (StringCompare(gSaveBlock2Ptr->playerName, gStringVar1))
        return FALSE;

    return TRUE;
}

void CheckOwnAllBeasts(void)
{
    u32 i, j, k;
    struct Pokemon tempMon;
    u32 species = SPECIES_RAIKOU;
    bool32 hasBeasts[3] = {FALSE, FALSE, FALSE}; // array to store the values of hasRaikou, hasEntei, hasSuicune
    bool32 hasMon = FALSE;
    u32 playerID = GetPlayerIDAsU32();

    for(k = 0; k < 3; k++, species++)
    {
        hasMon = FALSE;

        // check party for species
        for(i = 0; i < PARTY_SIZE && !hasMon; i++)
        {
            if(species == GetMonData(&gPlayerParty[i], MON_DATA_SPECIES, 0))
            {
                hasMon = MonHasPlayersOT(&gPlayerParty[i], playerID);
            }
        }
        // check boxes for species; doesn't run if mon with playerOT found in party
        for(i = 0; i < TOTAL_BOXES_COUNT && !hasMon; i++)
        {
            for(j = 0; j < IN_BOX_COUNT && !hasMon; j++)
            {
                if(GetBoxMonDataAt(i, j, MON_DATA_SPECIES) == species)
                {
                    BoxMonToMon(GetBoxedMonPtr(i, j), &tempMon);
                    hasMon = MonHasPlayersOT(&tempMon, playerID);
                }
            }
        }
        hasBeasts[k] = hasMon;
    }
    
    gSpecialVar_0x800B = hasBeasts[2]; // Player has Suicune
    gSpecialVar_Result = hasBeasts[0] && hasBeasts[1] && hasBeasts[2]; // Player has all three
    return;
}

void SetLandmarkFlagIfEnteredFromNorth(void)
{
    if(gSaveBlock1Ptr->pos.y <= 2)
    {
        FlagSet(FLAG_LANDMARK_ROUTE_10_POKEMON_CENTER);
    }
}

void CheckHasFossils(void)
{
    u8 multichoiceCase = 0;

    bool8 haveHelixFossil = gSpecialVar_0x8008;
    bool8 haveDomeFossil = gSpecialVar_0x8009;
    bool8 haveOldAmber = gSpecialVar_0x800A;
    bool8 haveRootFossil = gSpecialVar_0x800B;
    bool8 haveClawFossil = gSpecialVar_Result;

    multichoiceCase = (haveClawFossil << 4) | (haveRootFossil << 3) | (haveOldAmber << 2) | (haveDomeFossil << 1) | haveHelixFossil;

    gSpecialVar_Result = multichoiceCase;
}

void CheckShouldForceBike(void)
{
    if ((gMapHeader.mapLayoutId == LAYOUT_ROUTE18) && (gSaveBlock1Ptr->pos.x <= 41) && (gSaveBlock1Ptr->pos.y <= 11))
    {
        FlagSet(FLAG_SYS_CYCLING_ROAD);
        if (gPlayerAvatar.flags & PLAYER_AVATAR_FLAG_ON_FOOT)
        {
            SetPlayerAvatarTransitionFlags(PLAYER_AVATAR_FLAG_BIKE);
            return;
        }
    }
    if ((gMapHeader.mapLayoutId == LAYOUT_ROUTE16) && (gSaveBlock1Ptr->pos.x <= 20) && (gSaveBlock1Ptr->pos.y >= 10))
    {
        FlagSet(FLAG_SYS_CYCLING_ROAD);
        if (gPlayerAvatar.flags & PLAYER_AVATAR_FLAG_ON_FOOT)
        {
            SetPlayerAvatarTransitionFlags(PLAYER_AVATAR_FLAG_BIKE);
            return;
        }
    }
}

void SetMtMoonRocksClearedGoneThroughLowerFloors(void)
{
    if(gSaveBlock1Ptr->pos.x == 26 && gSaveBlock1Ptr->pos.y == 20)
    {
        FlagSet(FLAG_MT_MOON_ROCKS_CLEARED);
    }
}

void HideWarpArrowSprite(void)
{
    struct ObjectEvent *playerObjEvent = &gObjectEvents[gPlayerAvatar.objectEventId];
    SetSpriteInvisible(playerObjEvent->warpArrowSpriteId);
}

static void SetUpRoomDecorBed(void)
{
    if(VarGet(VAR_ROOM_BED) <= BED_PIKACHU && VarGet(VAR_ROOM_BED) != BED_NONE)
    {
        // Feathery Bed values
        u32 topLeft = 0x068;
        u32 topMid = 0x069;
        u32 topRight = 0x06A;
        u32 botLeft = 0x070;
        u32 botMid = 0x071;
        u32 botRight = 0x072;
        // top of bed frame; the same for all beds
        MapGridSetMetatileIdAt(TILE_BED_START_X    , TILE_BED_START_Y    , 0x012);
        MapGridSetMetatileIdAt(TILE_BED_START_X + 1, TILE_BED_START_Y    , 0x013);
        MapGridSetMetatileIdAt(TILE_BED_START_X + 2, TILE_BED_START_Y    , 0x014);
        switch(VarGet(VAR_ROOM_BED))
        {
            case BED_PINK:
                topLeft += 3;
                topMid += 3;
                topRight += 3;
                botLeft += 3;
                botMid += 3;
                botRight += 3;
                break;
            case BED_POLKADOT:
                topLeft += 16;
                topMid += 16;
                topRight += 16;
                botLeft += 16;
                botMid += 16;
                botRight += 16;
                break;
            case BED_PIKACHU:
                topLeft += 19;
                topMid += 19;
                topRight += 19;
                botLeft += 19;
                botMid += 19;
                botRight += 19;
                break;
        }
        MapGridSetMetatileIdAt(TILE_BED_START_X    , TILE_BED_START_Y + 1, topLeft);
        MapGridSetMetatileIdAt(TILE_BED_START_X + 1, TILE_BED_START_Y + 1, topMid);
        MapGridSetMetatileIdAt(TILE_BED_START_X + 2, TILE_BED_START_Y + 1, topRight);
        MapGridSetMetatileIdAt(TILE_BED_START_X    , TILE_BED_START_Y + 2, botLeft);
        MapGridSetMetatileIdAt(TILE_BED_START_X + 1, TILE_BED_START_Y + 2, (botMid) | MAPGRID_COLLISION_MASK);
        MapGridSetMetatileIdAt(TILE_BED_START_X + 2, TILE_BED_START_Y + 2, botRight);
    }
}

static void SetUpRoomDecorPlant(void)
{
    if(VarGet(VAR_ROOM_PLANT) <= PLANT_GORGEOUS && VarGet(VAR_ROOM_PLANT) != PLANT_NONE)
    {   // left only for 2x2 plants, set to Colorful
        // Magna Plant Values
        u32 topLeft = 0x08A;
        u32 botLeft = 0x092;
        u32 topRight = 0x05E;
        u32 botRight = 0x066;
        switch(VarGet(VAR_ROOM_PLANT))
        {
            case PLANT_MAGNA:
                topLeft = 0x001;
                botLeft = 0x001;
                break;
            case PLANT_TROPIC:
                topLeft = 0x001;
                botLeft = 0x001;
                topRight += 16;
                botRight += 16;
                break;
            case PLANT_JUMBO:
                topLeft = 0x001;
                botLeft = 0x001;
                topRight += 32;
                botRight += 32;
                break;
            case PLANT_RED:
                topLeft = 0x001;
                botLeft = 0x001;
                topRight += 17;
                botRight += 17;
                break;
            case PLANT_TROPICAL:
                topLeft = 0x001;
                botLeft = 0x001;
                topRight += 1;
                botRight += 1;
                break;
            case PLANT_PRETTY:
                topLeft = 0x001;
                botLeft = 0x001;
                topRight += 33;
                botRight += 33;
                break;
            case PLANT_COLORFUL:
                topRight += 45;
                botRight += 45;
                break;
            case PLANT_BIG:
                topLeft += 2;
                botLeft += 2;
                topRight += 47;
                botRight += 47;
                break;
            case PLANT_GORGEOUS:
                topLeft += 4;
                botLeft += 4;
                topRight += 49;
                botRight += 49;
                break;
        }
        MapGridSetMetatileIdAt(TILE_PLANT_START_X - 1, TILE_PLANT_START_Y    , topLeft);
        MapGridSetMetatileIdAt(TILE_PLANT_START_X    , TILE_PLANT_START_Y    , topRight);
        if(VarGet(VAR_ROOM_PLANT) < PLANT_COLORFUL)
            MapGridSetMetatileIdAt(TILE_PLANT_START_X - 1, TILE_PLANT_START_Y + 1, botLeft);
        else
            MapGridSetMetatileIdAt(TILE_PLANT_START_X - 1, TILE_PLANT_START_Y + 1, (botLeft) | MAPGRID_COLLISION_MASK);
        MapGridSetMetatileIdAt(TILE_PLANT_START_X    , TILE_PLANT_START_Y + 1, (botRight) | MAPGRID_COLLISION_MASK);
    }
}

static void SetUpRoomDecorPoster(void)
{
    if(VarGet(VAR_ROOM_POSTER) <= POSTER_KISS && VarGet(VAR_ROOM_POSTER) != POSTER_NONE)
    {
        MapGridSetMetatileIdAt(TILE_POSTER_START_X    , TILE_POSTER_START_Y    , (0x02A + ((VarGet(VAR_ROOM_POSTER) - 1) * 2)) | MAPGRID_COLLISION_MASK);
        MapGridSetMetatileIdAt(TILE_POSTER_START_X + 1, TILE_POSTER_START_Y    , (0x02B + ((VarGet(VAR_ROOM_POSTER) - 1) * 2)) | MAPGRID_COLLISION_MASK);
        if(VarGet(VAR_ROOM_POSTER) < POSTER_SKY)
        {
            MapGridSetMetatileIdAt(TILE_POSTER_START_X    , TILE_POSTER_START_Y + 1, (0x046 + ((VarGet(VAR_ROOM_POSTER) - 1) * 2)) | MAPGRID_COLLISION_MASK);
            MapGridSetMetatileIdAt(TILE_POSTER_START_X + 1, TILE_POSTER_START_Y + 1, (0x047 + ((VarGet(VAR_ROOM_POSTER) - 1) * 2)) | MAPGRID_COLLISION_MASK);
        }
        else
        {
            MapGridSetMetatileIdAt(TILE_POSTER_START_X    , TILE_POSTER_START_Y + 1, (0x060 + ((VarGet(VAR_ROOM_POSTER) - 13) * 2)) | MAPGRID_COLLISION_MASK);
            MapGridSetMetatileIdAt(TILE_POSTER_START_X + 1, TILE_POSTER_START_Y + 1, (0x061 + ((VarGet(VAR_ROOM_POSTER) - 13) * 2)) | MAPGRID_COLLISION_MASK);
        }
    }
}

static void SetUpRoomDecorConsole(void)
{
    if(VarGet(VAR_ROOM_CONSOLE) <= CONSOLE_GAMECUBE && VarGet(VAR_ROOM_CONSOLE) != CONSOLE_NONE)
    {
        MapGridSetMetatileIdAt(TILE_CONSOLE_START_X, TILE_CONSOLE_START_Y    , (0x019 + (VarGet(VAR_ROOM_CONSOLE) - 1)) | MAPGRID_COLLISION_MASK);
        MapGridSetMetatileIdAt(TILE_CONSOLE_START_X, TILE_CONSOLE_START_Y + 1, 0x021 + (VarGet(VAR_ROOM_CONSOLE) - 1));
    }
}

static void SetUpRoomDecorCarpet(void)
{
    if(VarGet(VAR_ROOM_CARPET) <= CARPET_SPIKES && VarGet(VAR_ROOM_CARPET) != CARPET_NONE)
    {
        u32 shift = 0;
        switch(VarGet(VAR_ROOM_CARPET))
        {
            case CARPET_YELLOW:
            case CARPET_GREEN:
                shift = 1;
                break;
            case CARPET_SURF:
            case CARPET_THUNDER:
                shift = 2;
                break;
            case CARPET_FIRE_BLAST:
            case CARPET_POWDER_SNOW:
                shift = 3;
                break;
            case CARPET_ATTRACT:
            case CARPET_FISSURE:
                shift = 4;
                break;
            case CARPET_SPIKES:
                shift = 5;
                break;
        }
        MapGridSetMetatileIdAt(TILE_CARPET_START_X    , TILE_CARPET_START_Y    , 0x218 + ((VarGet(VAR_ROOM_CARPET) - 1) * 4) + (shift * 24));
        MapGridSetMetatileIdAt(TILE_CARPET_START_X + 1, TILE_CARPET_START_Y    , 0x219 + ((VarGet(VAR_ROOM_CARPET) - 1) * 4) + (shift * 24));
        MapGridSetMetatileIdAt(TILE_CARPET_START_X + 2, TILE_CARPET_START_Y    , 0x21A + ((VarGet(VAR_ROOM_CARPET) - 1) * 4) + (shift * 24));
        MapGridSetMetatileIdAt(TILE_CARPET_START_X + 3, TILE_CARPET_START_Y    , 0x21B + ((VarGet(VAR_ROOM_CARPET) - 1) * 4) + (shift * 24));

        MapGridSetMetatileIdAt(TILE_CARPET_START_X    , TILE_CARPET_START_Y + 1, 0x220 + ((VarGet(VAR_ROOM_CARPET) - 1) * 4) + (shift * 24));
        MapGridSetMetatileIdAt(TILE_CARPET_START_X + 1, TILE_CARPET_START_Y + 1, 0x221 + ((VarGet(VAR_ROOM_CARPET) - 1) * 4) + (shift * 24));
        MapGridSetMetatileIdAt(TILE_CARPET_START_X + 2, TILE_CARPET_START_Y + 1, 0x222 + ((VarGet(VAR_ROOM_CARPET) - 1) * 4) + (shift * 24));
        MapGridSetMetatileIdAt(TILE_CARPET_START_X + 3, TILE_CARPET_START_Y + 1, 0x223 + ((VarGet(VAR_ROOM_CARPET) - 1) * 4) + (shift * 24));

        MapGridSetMetatileIdAt(TILE_CARPET_START_X    , TILE_CARPET_START_Y + 2, 0x228 + ((VarGet(VAR_ROOM_CARPET) - 1) * 4) + (shift * 24));
        MapGridSetMetatileIdAt(TILE_CARPET_START_X + 1, TILE_CARPET_START_Y + 2, 0x229 + ((VarGet(VAR_ROOM_CARPET) - 1) * 4) + (shift * 24));
        MapGridSetMetatileIdAt(TILE_CARPET_START_X + 2, TILE_CARPET_START_Y + 2, 0x22A + ((VarGet(VAR_ROOM_CARPET) - 1) * 4) + (shift * 24));
        MapGridSetMetatileIdAt(TILE_CARPET_START_X + 3, TILE_CARPET_START_Y + 2, 0x22B + ((VarGet(VAR_ROOM_CARPET) - 1) * 4) + (shift * 24));

        MapGridSetMetatileIdAt(TILE_CARPET_START_X    , TILE_CARPET_START_Y + 3, 0x230 + ((VarGet(VAR_ROOM_CARPET) - 1) * 4) + (shift * 24));
        MapGridSetMetatileIdAt(TILE_CARPET_START_X + 1, TILE_CARPET_START_Y + 3, 0x231 + ((VarGet(VAR_ROOM_CARPET) - 1) * 4) + (shift * 24));
        MapGridSetMetatileIdAt(TILE_CARPET_START_X + 2, TILE_CARPET_START_Y + 3, 0x232 + ((VarGet(VAR_ROOM_CARPET) - 1) * 4) + (shift * 24));
        MapGridSetMetatileIdAt(TILE_CARPET_START_X + 3, TILE_CARPET_START_Y + 3, 0x233 + ((VarGet(VAR_ROOM_CARPET) - 1) * 4) + (shift * 24));
    }
}

static void SetUpRoomDecorDesk(void)
{
    if(VarGet(VAR_ROOM_TABLE) <= DESK_HARD && VarGet(VAR_ROOM_TABLE) != DESK_NONE)
    {
        u32 shift = 0;
        if(VarGet(VAR_ROOM_TABLE) > DESK_COMFORT)
        {
            shift = 1;
        }
        MapGridSetMetatileIdAt(TILE_DESK_START_X    , TILE_DESK_START_Y    , (0x098 + ((VarGet(VAR_ROOM_TABLE) - 1) * 2) + ((VarGet(VAR_ROOM_CARPET)) * 32) + (shift * 8)) | MAPGRID_COLLISION_MASK);
        MapGridSetMetatileIdAt(TILE_DESK_START_X + 1, TILE_DESK_START_Y    , (0x099 + ((VarGet(VAR_ROOM_TABLE) - 1) * 2) + ((VarGet(VAR_ROOM_CARPET)) * 32) + (shift * 8)) | MAPGRID_COLLISION_MASK);
        MapGridSetMetatileIdAt(TILE_DESK_START_X    , TILE_DESK_START_Y + 1, (0x0A0 + ((VarGet(VAR_ROOM_TABLE) - 1) * 2) + ((VarGet(VAR_ROOM_CARPET)) * 32) + (shift * 8)) | MAPGRID_COLLISION_MASK);
        MapGridSetMetatileIdAt(TILE_DESK_START_X + 1, TILE_DESK_START_Y + 1, (0x0A1 + ((VarGet(VAR_ROOM_TABLE) - 1) * 2) + ((VarGet(VAR_ROOM_CARPET)) * 32) + (shift * 8)) | MAPGRID_COLLISION_MASK);
    }
}

static void SetUpRoomDecorCushion(void)
{
    FlagSet(FLAG_TEMP_4);
    if(VarGet(VAR_ROOM_BED) <= BED_PIKACHU && VarGet(VAR_ROOM_BED) != BED_NONE) // no cushion if no bed
    {
        if(VarGet(VAR_ROOM_CUSHION) <= CUSHION_WATER && VarGet(VAR_ROOM_CUSHION) != CUSHION_NONE)
        {
            FlagClear(FLAG_TEMP_4);
            switch(VarGet(VAR_ROOM_CUSHION))
            {
                case CUSHION_PIKA:
                    VarSet(VAR_OBJ_GFX_ID_2, OBJ_EVENT_GFX_PIKA_CUSHION);
                    break;
                case CUSHION_ROUND:
                    VarSet(VAR_OBJ_GFX_ID_2, OBJ_EVENT_GFX_ROUND_CUSHION);
                    break;
                case CUSHION_KISS:
                    VarSet(VAR_OBJ_GFX_ID_2, OBJ_EVENT_GFX_KISS_CUSHION);
                    break;
                case CUSHION_ZIGZAG:
                    VarSet(VAR_OBJ_GFX_ID_2, OBJ_EVENT_GFX_ZIGZAG_CUSHION);
                    break;
                case CUSHION_SPIN:
                    VarSet(VAR_OBJ_GFX_ID_2, OBJ_EVENT_GFX_SPIN_CUSHION);
                    break;
                case CUSHION_DIAMOND:
                    VarSet(VAR_OBJ_GFX_ID_2, OBJ_EVENT_GFX_DIAMOND_CUSHION);
                    break;
                case CUSHION_BALL:
                    VarSet(VAR_OBJ_GFX_ID_2, OBJ_EVENT_GFX_BALL_CUSHION);
                    break;
                case CUSHION_GRASS:
                    VarSet(VAR_OBJ_GFX_ID_2, OBJ_EVENT_GFX_GRASS_CUSHION);
                    break;
                case CUSHION_FIRE:
                    VarSet(VAR_OBJ_GFX_ID_2, OBJ_EVENT_GFX_FIRE_CUSHION);
                    break;
                case CUSHION_WATER:
                    VarSet(VAR_OBJ_GFX_ID_2, OBJ_EVENT_GFX_WATER_CUSHION);
                    break;
            }
        }
    }
}

static void SetUpRoomDecorBigDoll(void)
{
    FlagSet(FLAG_TEMP_3);
    u32 bigDoll = VarGet(VAR_ROOM_BIG_DOLL);
    if(bigDoll <= BIG_DOLL_REGISTEEL && bigDoll != BIG_DOLL_NONE)
    {
        FlagClear(FLAG_TEMP_3);
        static const u32 lookupTable[] = {
            BIG_DOLL_SNORLAX, OBJ_EVENT_GFX_BIG_SNORLAX_DOLL,
            BIG_DOLL_ONIX, OBJ_EVENT_GFX_ZBIG_ONIX_DOLL,
            BIG_DOLL_LAPRAS, OBJ_EVENT_GFX_BIG_LAPRAS_DOLL,
            BIG_DOLL_RHYDON, OBJ_EVENT_GFX_BIG_RHYDON_DOLL,
            BIG_DOLL_VENUSAUR, OBJ_EVENT_GFX_BIG_VENUSAUR_DOLL,
            BIG_DOLL_CHARIZARD, OBJ_EVENT_GFX_BIG_CHARIZARD_DOLL,
            BIG_DOLL_BLASTOISE, OBJ_EVENT_GFX_BIG_BLASTOISE_DOLL,
            BIG_DOLL_WAILMER, OBJ_EVENT_GFX_BIG_WAILMER_DOLL,
            BIG_DOLL_REGIROCK, OBJ_EVENT_GFX_BIG_REGIROCK_DOLL,
            BIG_DOLL_REGICE, OBJ_EVENT_GFX_BIG_REGICE_DOLL,
            BIG_DOLL_REGISTEEL, OBJ_EVENT_GFX_BIG_REGISTEEL_DOLL
        };
        u32 i;
        for (i = 0; i < sizeof(lookupTable) / sizeof(lookupTable[0]); i += 2)
        {
            if (bigDoll == lookupTable[i])
            {
                VarSet(VAR_OBJ_GFX_ID_3, lookupTable[i + 1]);
                break;
            }
        }
    }
}

static u16 MapOrnamentConstantsToObjectEventGfx(u16 ornament)
{
    static const u16 ornamentToGfx[] = {
        [DOLL_PIKACHU] = OBJ_EVENT_GFX_PIKACHU_DOLL,
        [DOLL_SURF_PIKACHU] = OBJ_EVENT_GFX_ZSURFING_PIKACHU_DOLL,
        [DOLL_CLEFAIRY] = OBJ_EVENT_GFX_CLEFAIRY_DOLL,
        [DOLL_JIGGLYPUFF] = OBJ_EVENT_GFX_JIGGLYPUFF_DOLL,
        [DOLL_BULBASAUR] = OBJ_EVENT_GFX_ZBULBASAUR_DOLL,
        [DOLL_ODDISH] = OBJ_EVENT_GFX_ZODDISH_DOLL,
        [DOLL_GENGAR] = OBJ_EVENT_GFX_ZGENGAR_DOLL,
        [DOLL_SHELLDER] = OBJ_EVENT_GFX_ZSHELLDER_DOLL,
        [DOLL_GRIMER] = OBJ_EVENT_GFX_ZGRIMER_DOLL,
        [DOLL_VOLTORB] = OBJ_EVENT_GFX_ZVOLTORB_DOLL,
        [DOLL_WEEDLE] = OBJ_EVENT_GFX_ZWEEDLE_DOLL,
        [DOLL_MAGIKARP] = OBJ_EVENT_GFX_ZMAGIKARP_DOLL,
        [DOLL_CHARMANDER] = OBJ_EVENT_GFX_ZCHARMANDER_DOLL,
        [DOLL_SQUIRTLE] = OBJ_EVENT_GFX_ZSQUIRTLE_DOLL,
        [DOLL_POLIWAG] = OBJ_EVENT_GFX_ZPOLIWAG_DOLL,
        [DOLL_DIGLETT] = OBJ_EVENT_GFX_ZDIGLETT_DOLL,
        [DOLL_STARYU] = OBJ_EVENT_GFX_ZSTARYU_DOLL,
        [DOLL_TENTACOOL] = OBJ_EVENT_GFX_ZTENTACOOL_DOLL,
        [DOLL_UNOWN] = OBJ_EVENT_GFX_ZUNOWN_DOLL,
        [DOLL_GEODUDE] = OBJ_EVENT_GFX_ZGEODUDE_DOLL,
        [DOLL_MACHOP] = OBJ_EVENT_GFX_ZMACHOP_DOLL,
        [DOLL_SILVER_TROPHY] = OBJ_EVENT_GFX_ZSILVER_TROPHY,
        [DOLL_GOLD_TROPHY] = OBJ_EVENT_GFX_ZGOLD_TROPHY,
        [DOLL_MAGNEMITE] = OBJ_EVENT_GFX_ZMAGNEMITE_DOLL,
        [DOLL_NATU] = OBJ_EVENT_GFX_ZNATU_DOLL,
        [DOLL_PORYGON2] = OBJ_EVENT_GFX_ZPORYGON2_DOLL,
        [DOLL_WOOPER] = OBJ_EVENT_GFX_ZWOOPER_DOLL,
        [DOLL_PICHU] = OBJ_EVENT_GFX_PICHU_DOLL,
        [DOLL_MARILL] = OBJ_EVENT_GFX_MARILL_DOLL,
        [DOLL_TOGEPI] = OBJ_EVENT_GFX_TOGEPI_DOLL,
        [DOLL_CYNDAQUIL] = OBJ_EVENT_GFX_CYNDAQUIL_DOLL,
        [DOLL_CHIKORITA] = OBJ_EVENT_GFX_CHIKORITA_DOLL,
        [DOLL_TOTODILE] = OBJ_EVENT_GFX_TOTODILE_DOLL,
        [DOLL_MEOWTH] = OBJ_EVENT_GFX_MEOWTH_DOLL,
        [DOLL_DITTO] = OBJ_EVENT_GFX_DITTO_DOLL,
        [DOLL_SMOOCHUM] = OBJ_EVENT_GFX_SMOOCHUM_DOLL,
        [DOLL_TREECKO] = OBJ_EVENT_GFX_TREECKO_DOLL,
        [DOLL_TORCHIC] = OBJ_EVENT_GFX_TORCHIC_DOLL,
        [DOLL_MUDKIP] = OBJ_EVENT_GFX_MUDKIP_DOLL,
        [DOLL_DUSKULL] = OBJ_EVENT_GFX_DUSKULL_DOLL,
        [DOLL_WYNAUT] = OBJ_EVENT_GFX_WYNAUT_DOLL,
        [DOLL_BALTOY] = OBJ_EVENT_GFX_BALTOY_DOLL,
        [DOLL_KECLEON] = OBJ_EVENT_GFX_KECLEON_DOLL,
        [DOLL_AZURILL] = OBJ_EVENT_GFX_AZURILL_DOLL,
        [DOLL_SKITTY] = OBJ_EVENT_GFX_SKITTY_DOLL,
        [DOLL_SWABLU] = OBJ_EVENT_GFX_SWABLU_DOLL,
        [DOLL_GULPIN] = OBJ_EVENT_GFX_GULPIN_DOLL,
        [DOLL_LOTAD] = OBJ_EVENT_GFX_LOTAD_DOLL,
        [DOLL_SEEDOT] = OBJ_EVENT_GFX_SEEDOT_DOLL,
        [DOLL_SILVER_SHIELD] = OBJ_EVENT_GFX_ZSILVER_SHIELD,
        [DOLL_GOLD_SHIELD] = OBJ_EVENT_GFX_ZGOLD_SHIELD,
    };
    return ornamentToGfx[ornament];
}

// The Silver and Gold Shields were tiles in RSE Secret Bases, but object events here.
// No existing event object palette matches and the "free" slot 10 is taken up by decor that uses Red's palette.
// So, if the shields are present, their palette has to be force-loaded into their paletteNum slot (11).
void PatchShieldPaletteToSlot11(void)
{
    LoadPalette(gObjectEventPal_ShieldDecorations, OBJ_PLTT_ID(11), PLTT_SIZE_4BPP);
}

static void SetUpRoomDecorOrnaments(void)
{
    u32 roomTable = VarGet(VAR_ROOM_TABLE);
    u32 leftOrnament = VarGet(VAR_ROOM_LEFT_ORNAMENT);
    u32 rightOrnament = VarGet(VAR_ROOM_RIGHT_ORNAMENT);

    FlagSet(FLAG_TEMP_1);
    FlagSet(FLAG_TEMP_2);

    if(roomTable <= DESK_HARD && roomTable != DESK_NONE) // no ornament if no table
    {
        if(leftOrnament <= DOLL_GOLD_SHIELD && leftOrnament != DOLL_NONE)
        {
            FlagClear(FLAG_TEMP_1);
            VarSet(VAR_OBJ_GFX_ID_0, MapOrnamentConstantsToObjectEventGfx(leftOrnament));
        }
        if(rightOrnament <= DOLL_GOLD_SHIELD && rightOrnament != DOLL_NONE)
        {
            FlagClear(FLAG_TEMP_2);
            VarSet(VAR_OBJ_GFX_ID_1, MapOrnamentConstantsToObjectEventGfx(rightOrnament));
        }
        PatchShieldPaletteToSlot11();
    }
}

void SetUpRoomDecor(void)
{
    SetUpRoomDecorBed();
    SetUpRoomDecorPlant();
    SetUpRoomDecorPoster();
    SetUpRoomDecorConsole();
    SetUpRoomDecorCarpet();
    SetUpRoomDecorDesk();
    SetUpRoomDecorCushion();
    SetUpRoomDecorBigDoll();
    SetUpRoomDecorOrnaments();
    DrawWholeMapView();
}

void DisableMsgBoxWalkaway(void)
{
    sMsgBoxWalkawayDisabled = TRUE;
}

void EnableMsgBoxWalkaway(void)
{
    sMsgBoxWalkawayDisabled = FALSE;
}

bool8 IsMsgBoxWalkawayDisabled(void)
{
    return sMsgBoxWalkawayDisabled;
}

void TeachTrappedTentacoolSurf(void)
{
    u32 i;
    u32 move = MOVE_SURF;
    u32 pp = 15;
    if(gSpecialVar_0x8007 == 0) //party
    {
        i = CalculatePlayerPartyCount() - 1;
        SetMonData(&gPlayerParty[i], MON_DATA_MOVE4, &move);
        SetMonData(&gPlayerParty[i], MON_DATA_PP1 + 3, &pp);
        return;
    }
    else //box
    {
        SetBoxMonDataAt(gSpecialVar_MonBoxId, gSpecialVar_MonBoxPos, MON_DATA_MOVE4, &move);
        SetBoxMonDataAt(gSpecialVar_MonBoxId, gSpecialVar_MonBoxPos, MON_DATA_PP1 + 3, &pp);
        return;
    }
}

// ChatGPT optimized
void CheckPlayerTrappedOnCianwoodOrCinnabar(void)
{
    bool32 hasHM03 = CheckBagHasItem(ITEM_HM03, 1);
    u32 i, j;

    for (i = 0; i < PARTY_SIZE; i++) {
        if (GetMonData(&gPlayerParty[i], MON_DATA_SPECIES, NULL) == SPECIES_NONE) {
            break;
        } else {
            struct Pokemon* partyMon = &gPlayerParty[i];
            if (!GetMonData(partyMon, MON_DATA_IS_EGG) && MonKnowsMove(partyMon, MOVE_SURF)) {
                gSpecialVar_Result = 0;
                return;
            }
            if (hasHM03 && CanLearnTeachableMove(GetMonData(partyMon, MON_DATA_SPECIES, NULL), MOVE_SURF)) {
                gSpecialVar_Result = 0;
                return;
            }
        }
    }

    for (i = 0; i < TOTAL_BOXES_COUNT; i++) {
        for (j = 0; j < IN_BOX_COUNT; j++) {
            if (GetBoxMonDataAt(i, j, MON_DATA_SPECIES) == SPECIES_NONE) {
                continue;
            } else {
                struct Pokemon tempMon;
                BoxMonToMon(GetBoxedMonPtr(i, j), &tempMon);
                if (!GetMonData(&tempMon, MON_DATA_IS_EGG) && MonKnowsMove(&tempMon, MOVE_SURF)) {
                    gSpecialVar_Result = 0;
                    return;
                }
                if (hasHM03 && CanLearnTeachableMove(GetMonData(&tempMon, MON_DATA_SPECIES, NULL), MOVE_SURF)) {
                    gSpecialVar_Result = 0;
                    return;
                }
            }
        }
    }

    if(hasHM03)
        gSpecialVar_Result = 1;
    else
        gSpecialVar_Result = 2; // need to teach Tentacool Surf
}

// ChatGPT optimized
void CheckPlayerTrappedAtIndigoPlateau(void)
{
    bool32 hasHM03 = CheckBagHasItem(ITEM_HM03, 1);
    bool32 hasHM02 = CheckBagHasItem(ITEM_HM02, 1);
    bool32 hasKantoFlyPoint = FlagGet(FLAG_VISITED_VERMILION_CITY);
    u32 i, j;

    for (i = 0; i < PARTY_SIZE; i++) {
        if (GetMonData(&gPlayerParty[i], MON_DATA_SPECIES, NULL) == SPECIES_NONE) {
            break;
        } else {
            struct Pokemon* partyMon = &gPlayerParty[i];
            if (!GetMonData(partyMon, MON_DATA_IS_EGG) && 
                (MonKnowsMove(partyMon, MOVE_SURF) || 
                 (MonKnowsMove(partyMon, MOVE_FLY) && hasKantoFlyPoint))) {
                gSpecialVar_Result = 0;
                return;
            }
            if ((hasHM03 && CanLearnTeachableMove(GetMonData(partyMon, MON_DATA_SPECIES, NULL), MOVE_SURF)) ||
                (hasHM02 && CanLearnTeachableMove(GetMonData(partyMon, MON_DATA_SPECIES, NULL), MOVE_FLY) && hasKantoFlyPoint)) {
                gSpecialVar_Result = 0;
                return;
            }
        }
    }

    for (i = 0; i < TOTAL_BOXES_COUNT; i++) {
        for (j = 0; j < IN_BOX_COUNT; j++) {
            if (GetBoxMonDataAt(i, j, MON_DATA_SPECIES) == SPECIES_NONE) {
                continue;
            } else {
                struct Pokemon tempMon;
                BoxMonToMon(GetBoxedMonPtr(i, j), &tempMon);
                if (!GetMonData(&tempMon, MON_DATA_IS_EGG) &&
                    (MonKnowsMove(&tempMon, MOVE_SURF) || 
                     (MonKnowsMove(&tempMon, MOVE_FLY) && hasKantoFlyPoint))) {
                    gSpecialVar_Result = 0;
                    return;
                }
                if ((hasHM03 && CanLearnTeachableMove(GetMonData(&tempMon, MON_DATA_SPECIES, NULL), MOVE_SURF)) ||
                    (hasHM02 && CanLearnTeachableMove(GetMonData(&tempMon, MON_DATA_SPECIES, NULL), MOVE_FLY) && hasKantoFlyPoint)) {
                    gSpecialVar_Result = 0;
                    return;
                }
            }
        }
    }

    gSpecialVar_Result = 1; // trapped
}

