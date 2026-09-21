// CrystalDust's new-game intro: the clock-set prompt, then Prof. Oak's speech
// with a Wooper, and the Gold/Kris choice (D76). Ported out of CrystalDust's
// src/main_menu.c into its own file, the way expansion factored FireRed's intro
// into src/oak_speech.c, so main_menu.c keeps expansion's menu and only hands
// off to StartNewGameSceneCrystal().

#include "global.h"
#include "bg.h"
#include "clock.h"
#include "decompress.h"
#include "event_data.h"
#include "field_effect.h"
#include "gpu_regs.h"
#include "graphics.h"
#include "international_string_util.h"
#include "main.h"
#include "malloc.h"
#include "math_util.h"
#include "menu.h"
#include "menu_helpers.h"
#include "naming_screen.h"
#include "new_game.h"
#include "overworld.h"
#include "palette.h"
#include "pokeball.h"
#include "pokemon.h"
#include "random.h"
#include "rtc.h"
#include "scanline_effect.h"
#include "sound.h"
#include "save.h"
#include "sprite.h"
#include "string_util.h"
#include "strings.h"
#include "task.h"
#include "text.h"
#include "text_window.h"
#include "trainer_pokemon_sprites.h"
#include "util.h"
#include "wallclock.h"
#include "window.h"
#include "constants/rgb.h"
#include "constants/songs.h"
#include "oak_speech_crystal.h"


// ---- forward declarations (CrystalDust) ----

static void Task_NewGameClockSetIntro1(u8);
static void Task_NewGameClockSetIntro2(u8);
static void Task_NewGameClockSetIntro3(u8);
static void Task_NewGameClockSetIntro4(u8);
static void Task_NewGameClockSetIntro5(u8);
static void Task_NewGameClockSetIntro6(u8);
static void Task_NewGameOakSpeech_Init(u8);
static void NewGameOakSpeech_CreateWooperSprite(u8);
static void NewGameOakSpeech_CreatePlatformSprites(u8);
static void NewGameOakSpeech_StartFadeInTarget1OutTarget2(u8, u8);
static void Task_NewGameOakSpeech_WaitForTextToStart(u8);
static void NewGameOakSpeech_ShowDialogueWindow(u32, u32);
static void Task_NewGameOakSpeech_PrintThisEllipsis(u8);
static void Task_NewGameOakSpeech_CreatePokeBallToReleaseWooper(u8);
static void Task_NewGameOakSpeech_PrintIsPokemonWaitForAnimation(u8);
static void Task_NewGameOakSpeech_MainSpeech1(u8);
static void Task_NewGameOakSpeech_PutAwayWooper(u8);
static void Task_NewGameOakSpeech_MainSpeech2(u8);
static void Task_NewGameOakSpeech_StartOakPlatformFade(u8);
static void NewGameOakSpeech_StartFadeOutTarget1InTarget2(u8, u8);
static void Task_NewGameOakSpeech_WaitOakPlatformFade(u8);
static void Task_NewGameOakSpeech_StartPlayerFadeIn(u8);
static void Task_NewGameOakSpeech_WaitForPlayerFadeIn(u8);
static void Task_NewGameOakSpeech_BoyOrGirl(u8);
static void LoadMainMenuWindowFrameTiles(u8, u16);
static void DrawMainMenuWindowBorder(const struct WindowTemplate*, u16);
static void Task_NewGameOakSpeech_WaitToShowGenderMenu(u8);
static void Task_NewGameOakSpeech_ChooseGender(u8);
static void NewGameOakSpeech_ShowGenderMenu(void);
static s8 NewGameOakSpeech_ProcessGenderMenuInput(void);
static void NewGameOakSpeech_ClearGenderWindow(u32, u32);
static void Task_NewGameOakSpeech_WhatsYourName(u8);
static void Task_NewGameOakSpeech_WaitForWhatsYourNameToPrint(u8);
static void Task_NewGameOakSpeech_StartNamingScreen(u8);
static void CB2_NewGameOakSpeech_ReturnFromNamingScreen(void);
static void NewGameOakSpeech_SetDefaultPlayerName(u8);
static void Task_NewGameOakSpeech_CreateNameYesNo(u8);
static void Task_NewGameOakSpeech_ProcessNameYesNoMenu(u8);
void CreateYesNoMenuParameterized(u8, u8, u16, u16, u8, u8);
static void Task_NewGameOakSpeech_SlidePlatformAway2(u8);
static void Task_NewGameOakSpeech_AreYouReady(u8);
static void Task_NewGameOakSpeech_PrepareToShrinkPlayer(u8);
static void Task_NewGameOakSpeech_StartFadePlayerToWhite(u8);
static void Task_NewGameOakSpeech_ShrinkPlayer(u8);
static void Task_NewGameOakSpeech_ShrinkBG2(u8);
static void Task_NewGameOakSpeech_FadePlayerToBlack(u8);
static void Task_NewGameOakSpeech_FadePlayerToWhite(u8);
static void Task_NewGameOakSpeech_Cleanup(u8);
static void SpriteCB_Null();
static void Task_NewGameOakSpeech_ReturnFromNamingScreenShowTextbox(u8);
static void LoadOakIntroBigSprite(u16 which, u16 offset);
static void Task_NewGameOakSpeech_FadeEverythingButPlayerAndTextbox(u8 taskId);
static void Task_NewGameOakSpeech_StartShrinkPlayer(u8 taskId);
static void Task_NewGameOakSpeech_WaitToFadeTextbox(u8 taskId);

// ---- graphics ----

static const u8 sMainMenu_UpdateQrCode[] = INCBIN_U8("graphics/misc/qr/domoreawesomecdupdate.4bpp");

// CrystalDust loaded the intro's dialogue tiles at 0xFC, the same base tile the
// Birch speech it replaces used.
#define OAK_INTRO_DLG_BASE_TILE_NUM 0xFC

static const u16 sOakSpeechBgPal[] = INCBIN_U16("graphics/oak_speech/bg0.gbapal");

static const u32 sOakSpeechBgGfx[] = INCBIN_U32("graphics/oak_speech/bg0.4bpp.lz");
static const u32 sOakSpeechBgMap[] = INCBIN_U32("graphics/oak_speech/map.bin.lz");

static const u32 gOakIntroPlatformGfx[] = INCBIN_U32("graphics/oak_speech/platform.4bpp.lz");
static const u16 gOakIntroPlatformPal[] = INCBIN_U16("graphics/oak_speech/platform.gbapal");

static const u16 sOakIntro_GoldPal[] = INCBIN_U16("graphics/oak_speech/gold.gbapal");
static const u16 sOakIntro_KrisPal[] = INCBIN_U16("graphics/oak_speech/kris.gbapal");
static const u16 sOakIntro_OakPal[] = INCBIN_U16("graphics/oak_speech/oak.gbapal");
static const u32 sOakIntro_GoldTiles[] = INCBIN_U32("graphics/oak_speech/gold.8bpp.lz");
static const u32 sOakIntro_KrisTiles[] = INCBIN_U32("graphics/oak_speech/kris.8bpp.lz");
static const u32 sOakIntro_OakTiles[] = INCBIN_U32("graphics/oak_speech/oak.8bpp.lz");


// ---- windows ----

const struct WindowTemplate sClockSetWindowTemplates[] = 
{
    {
        .bg = 0,
        .tilemapLeft = 2,
        .tilemapTop = 15,
        .width = 26,
        .height = 4,
        .paletteNum = 15,
        .baseBlock = 1
    },
    DUMMY_WIN_TEMPLATE
};

static const struct WindowTemplate sOakIntroTextWindows[] =
{
    {
        .bg = 0,
        .tilemapLeft = 2,
        .tilemapTop = 15,
        .width = 27,   // D110: was 26 for CrystalDust's two-tile border; Emerald's frame reaches one tile further
        .height = 4,
        .paletteNum = 15,
        .baseBlock = 1
    },
    {
        .bg = 0,
        .tilemapLeft = 18,
        .tilemapTop = 9,
        .width = 9,
        .height = 4,
        .paletteNum = 15,
        .baseBlock = 0x6D
    },
    {
        .bg = 0,
        .tilemapLeft = 3,
        .tilemapTop = 2,
        .width = 9,
        .height = 10,
        .paletteNum = 15,
        .baseBlock = 0x85
    },
    DUMMY_WIN_TEMPLATE
};

// ---- data ----

static const struct BgTemplate sMainMenuBgTemplates[] = {
    {
        .bg = 0,
        .charBaseIndex = 2,
        .mapBaseIndex = 30,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 0,
        .baseTile = 0
    },
    {
        .bg = 1,
        .charBaseIndex = 0,
        .mapBaseIndex = 7,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 3,
        .baseTile = 0
    }
};

static const struct BgTemplate sOakBgTemplates[] = {
    {
        .bg = 0,
        .charBaseIndex = 2,
        .mapBaseIndex = 31,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 0,
        .baseTile = 0
    },
    {
        .bg = 1,
        .charBaseIndex = 0,
        .mapBaseIndex = 30,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 2,
        .baseTile = 0
    },
    {
        .bg = 2,
        .charBaseIndex = 0,
        .mapBaseIndex = 28,
        .screenSize = 1,
        .paletteMode = 1,
        .priority = 1,
        .baseTile = 0
    }
};

static const struct ScrollArrowsTemplate sScrollArrowsTemplate_MainMenu = {2, 0x78, 8, 3, 0x78, 0x98, 3, 4, 1, 1, 0};

static const union AffineAnimCmd sSpriteAffineAnim_PlayerShrink[] = {
    AFFINEANIMCMD_FRAME(-2, -2, 0, 0x30),
    AFFINEANIMCMD_END
};

static const union AffineAnimCmd *const sSpriteAffineAnimTable_PlayerShrink[] =
{
    sSpriteAffineAnim_PlayerShrink
};

static const struct MenuAction sMenuActions_Gender[] = {
    {gText_OakBoy, {NULL}},
    {gText_OakGirl, {NULL}}
};

static const u8 *const gMalePresetNames[] = {
    gText_DefaultNameChris,
	gText_DefaultNameMat,
	gText_DefaultNameAllan,
	gText_DefaultNameJon,
	gText_DefaultNameGold,
	gText_DefaultNameHiro,
	gText_DefaultNameTaylor,
	gText_DefaultNameKarl,
	gText_DefaultNameSilver,
	gText_DefaultNameKamon,
	gText_DefaultNameOscar,
	gText_DefaultNameMax,
	gText_DefaultNameJimmy,
	gText_DefaultNameDiego,
	gText_DefaultNameAdam,
	gText_DefaultNameRaymond,
	gText_DefaultNameIan,
	gText_DefaultNameRutvik,
	gText_DefaultNameKamron,
	gText_DefaultNameTanek
};

static const u8 *const gFemalePresetNames[] = {
    gText_DefaultNameKris,
    gText_DefaultNameAmanda,
    gText_DefaultNameJuana,
    gText_DefaultNameJodi,
    gText_DefaultNameCrystal,
    gText_DefaultNameMarina,
    gText_DefaultNameSierra,
    gText_DefaultNameJenny,
    gText_DefaultNameLorrie,
    gText_DefaultNameHannah,
    gText_DefaultNameGina,
    gText_DefaultNameColette,
    gText_DefaultNameKatie,
    gText_DefaultNameSarah,
    gText_DefaultNameAlyx,
    gText_DefaultNameEllie,
    gText_DefaultNameJoyce,
    gText_DefaultNameNancy,
    gText_DefaultNameBarbara,
    gText_DefaultNameJill
};

static const struct CompressedSpriteSheet sCompressedSpriteSheet_OakPlatform = 
{
    .data = gOakIntroPlatformGfx,
    .size = 0x600,
    .tag = 0x1000
};

static const struct SpritePalette sSpritePalette_OakPlatform = 
{
    .data = gOakIntroPlatformPal,
    .tag = 0x1000
};

static const union AnimCmd sSpriteAnim_OakPlatform1[] = 
{
    ANIMCMD_FRAME(0, 0),
    ANIMCMD_END
};

static const union AnimCmd sSpriteAnim_OakPlatform2[] = 
{
    ANIMCMD_FRAME(16, 0),
    ANIMCMD_END
};

static const union AnimCmd sSpriteAnim_OakPlatform3[] = 
{
    ANIMCMD_FRAME(32, 0),
    ANIMCMD_END
};

static const union AnimCmd *const sSpriteAnimTable_OakPlatform[] = 
{
    sSpriteAnim_OakPlatform1,
    sSpriteAnim_OakPlatform2,
    sSpriteAnim_OakPlatform3,
};

static const struct OamData sOamData_OakPlatform = 
{
    .objMode = ST_OAM_OBJ_BLEND,
    .size = 2,
    .priority = 2
};

static const struct SpriteTemplate sSpriteTemplate_OakPlatform = 
{
    .tileTag = 0x1000,
    .paletteTag = 0x1000,
    .oam = &sOamData_OakPlatform,
    .anims = sSpriteAnimTable_OakPlatform,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCallbackDummy
};

// Local copies of main_menu.c's statics: the intro runs on the main menu's
// callbacks and window frame, but those are file-static over there.
static void CB2_MainMenu(void)
{
    RunTasks();
    AnimateSprites();
    BuildOamBuffer();
    UpdatePaletteFade();
}

static void VBlankCB_MainMenu(void)
{
    LoadOam();
    ProcessSpriteCopyRequests();
    TransferPlttBuffer();
}

static void LoadMainMenuWindowFrameTiles(u8 bgId, u16 tileOffset)
{
    LoadBgTiles(bgId, GetWindowFrameTilesPal(gSaveBlock2Ptr->optionsWindowFrameType)->tiles, 0x120, tileOffset);
    LoadPalette(GetWindowFrameTilesPal(gSaveBlock2Ptr->optionsWindowFrameType)->pal, BG_PLTT_ID(2), PLTT_SIZE_4BPP);
}

static void DrawMainMenuWindowBorder(const struct WindowTemplate *template, u16 baseTileNum)
{
    u16 r9 = 1 + baseTileNum;
    u16 r10 = 2 + baseTileNum;
    u16 sp18 = 3 + baseTileNum;
    u16 spC = 5 + baseTileNum;
    u16 sp10 = 6 + baseTileNum;
    u16 sp14 = 7 + baseTileNum;
    u16 r6 = 8 + baseTileNum;

    FillBgTilemapBufferRect(template->bg, baseTileNum, template->tilemapLeft - 1, template->tilemapTop - 1, 1, 1, 2);
    FillBgTilemapBufferRect(template->bg, r9, template->tilemapLeft, template->tilemapTop - 1, template->width, 1, 2);
    FillBgTilemapBufferRect(template->bg, r10, template->tilemapLeft + template->width, template->tilemapTop - 1, 1, 1, 2);
    FillBgTilemapBufferRect(template->bg, sp18, template->tilemapLeft - 1, template->tilemapTop, 1, template->height, 2);
    FillBgTilemapBufferRect(template->bg, spC, template->tilemapLeft + template->width, template->tilemapTop, 1, template->height, 2);
    FillBgTilemapBufferRect(template->bg, sp10, template->tilemapLeft - 1, template->tilemapTop + template->height, 1, 1, 2);
    FillBgTilemapBufferRect(template->bg, sp14, template->tilemapLeft, template->tilemapTop + template->height, template->width, 1, 2);
    FillBgTilemapBufferRect(template->bg, r6, template->tilemapLeft + template->width, template->tilemapTop + template->height, 1, 1, 2);
    CopyBgTilemapBufferToVram(template->bg);
}

static EWRAM_DATA u8 *sOakIntro_BgBuffer = NULL;

// ---- intro ----

// Entry point from main_menu.c: hand the main menu's task over to the intro.
void StartNewGameSceneCrystal(u8 taskId)
{
    gTasks[taskId].func = Task_NewGameClockSetIntro1;
}

void Task_NewGameClockSetIntro1(u8 taskId)
{
    if (IsBGMStopped())
    {
        // moved from new_game.c so it doesn't change up the time on us unexpectedly after setting
        if (gSaveFileStatus == SAVE_STATUS_EMPTY || gSaveFileStatus == SAVE_STATUS_CORRUPT)
            RtcReset();
        
        FlagClear(FLAG_SYS_GBS_ENABLED);

        gTasks[taskId].data[0] = 15;
        gTasks[taskId].func = Task_NewGameClockSetIntro2;
    }
}

void Task_NewGameClockSetIntro2(u8 taskId)
{
    if (--gTasks[taskId].data[0] == 0)
    {
        SetGpuReg(REG_OFFSET_DISPCNT, 0);
        SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_OBJ_ON | DISPCNT_OBJ_1D_MAP);
        InitBgFromTemplate(&sOakBgTemplates[0]);
        SetGpuReg(REG_OFFSET_WIN0H, 0);
        SetGpuReg(REG_OFFSET_WIN0V, 0);
        SetGpuReg(REG_OFFSET_WININ, 0);
        SetGpuReg(REG_OFFSET_WINOUT, 0);
        SetGpuReg(REG_OFFSET_BLDCNT, 0);
        SetGpuReg(REG_OFFSET_BLDALPHA, 0);
        SetGpuReg(REG_OFFSET_BLDY, 0);

        ScanlineEffect_Stop();
        ResetSpriteData();
        FreeAllSpritePalettes();
        ResetPaletteFade();
        ResetAllPicSprites();

        gTasks[taskId].func = Task_NewGameClockSetIntro3;
        
        ShowBg(0);
    }
}

void Task_NewGameClockSetIntro3(u8 taskId)
{
    InitWindows(sClockSetWindowTemplates);
    LoadMessageBoxGfx(0, OAK_INTRO_DLG_BASE_TILE_NUM, BG_PLTT_ID(15));
    PutWindowTilemap(0);
    CopyWindowToVram(0, COPYWIN_GFX);
    StringExpandPlaceholders(gStringVar4, gText_SetClock_WokeMeUp);
    AddTextPrinterForMessage(TRUE);
    BeginNormalPaletteFade(0xFFFFFFFF, 0, 16, 0, 0);
    NewGameOakSpeech_ShowDialogueWindow(0, 1);
    gTasks[taskId].func = Task_NewGameClockSetIntro4;
}

void Task_NewGameClockSetIntro4(u8 taskId)
{
    if (!RunTextPrintersAndIsPrinter0Active() && JOY_NEW(A_BUTTON | B_BUTTON))
    {
        BeginNormalPaletteFade(0xFFFFFFFF, 0, 0, 0x10, 0);
        gTasks[taskId].func = Task_NewGameClockSetIntro5;
    }
}

static void ReturnFromSetClock(void)
{
    u8 taskId;

    //InitTimeBasedEvents();
    ResetBgsAndClearDma3BusyFlags(0);
    SetGpuReg(REG_OFFSET_DISPCNT, 0);
    SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_OBJ_ON | DISPCNT_OBJ_1D_MAP);
    sOakIntro_BgBuffer = AllocZeroed(0x400);
    //InitBgsFromTemplates(0, sMainMenuBgTemplates, 2);
    InitBgsFromTemplates(1, sOakBgTemplates, ARRAY_COUNT(sOakBgTemplates));
    SetBgTilemapBuffer(2, sOakIntro_BgBuffer);
    ResetAllBgsCoordinates();
    SetVBlankCallback(NULL);
    SetGpuReg(REG_OFFSET_BG2CNT, 0);
    SetGpuReg(REG_OFFSET_BG1CNT, 0);
    SetGpuReg(REG_OFFSET_BG0CNT, 0);
    SetGpuReg(REG_OFFSET_BG2HOFS, 0);
    SetGpuReg(REG_OFFSET_BG2VOFS, 0);
    SetGpuReg(REG_OFFSET_BG1HOFS, 0);
    SetGpuReg(REG_OFFSET_BG1VOFS, 0);
    SetGpuReg(REG_OFFSET_BG0HOFS, 0);
    SetGpuReg(REG_OFFSET_BG0VOFS, 0);
    DmaFill16(3, 0, VRAM, VRAM_SIZE);
    DmaFill32(3, 0, OAM, OAM_SIZE);
    DmaFill16(3, 0, PLTT, PLTT_SIZE);
    ResetPaletteFade();
    ResetTasks();
    taskId = CreateTask(Task_NewGameClockSetIntro6, 0);
    gTasks[taskId].data[0] = 0;
    ScanlineEffect_Stop();
    ResetSpriteData();
    FreeAllSpritePalettes();
    ResetAllPicSprites();
    SetGpuReg(REG_OFFSET_WIN0H, 0);
    SetGpuReg(REG_OFFSET_WIN0V, 0);
    SetGpuReg(REG_OFFSET_WININ, 0);
    SetGpuReg(REG_OFFSET_WINOUT, 0);
    SetGpuReg(REG_OFFSET_BLDCNT, 0);
    SetGpuReg(REG_OFFSET_BLDALPHA, 0);
    SetGpuReg(REG_OFFSET_BLDY, 0);
    gPlttBufferUnfaded[0] = RGB_BLACK;
    gPlttBufferFaded[0] = RGB_BLACK;
    SetVBlankCallback(VBlankCB_MainMenu);
    SetMainCallback2(CB2_MainMenu);
}

void Task_NewGameClockSetIntro5(u8 taskId)
{
    if (!gPaletteFade.active)
    {
        SetMainCallback2(CB2_StartWallClock);
        gMain.savedCallback = ReturnFromSetClock;
    }
}

void Task_NewGameClockSetIntro6(u8 taskId)
{
    if (gTasks[taskId].data[0]++ > 30)
    {
        gTasks[taskId].func = Task_NewGameOakSpeech_Init;
    }
}

#define tPlayerSpriteId data[2]
#define tPokeBallSpriteId data[3]
#define tSlideOffset data[4]
#define tIsDoneFadingSprites data[5]
#define tPlayerGender data[6]
#define tTimer data[7]
#define tWooperSpriteId data[8]
#define tLotadSpriteId data[9]
#define tBrendanSpriteId data[10]
#define tMaySpriteId data[11]
#define tTimer2 data[12]

enum {
    INTRO_GOLD,
    INTRO_KRIS,
    INTRO_OAK,
};

static void Task_NewGameOakSpeech_Init(u8 taskId)
{
    LZ77UnCompVram(sOakSpeechBgGfx, (void*)VRAM);
    LZ77UnCompVram(sOakSpeechBgMap, (void*)(VRAM + 0xF000));
    LoadPalette(sOakSpeechBgPal, BG_PLTT_ID(0), sizeof(sOakSpeechBgPal));
    gPlttBufferUnfaded[0] = RGB_BLACK;
    gPlttBufferFaded[0] = RGB_BLACK;
    NewGameOakSpeech_CreateWooperSprite(taskId);
    NewGameOakSpeech_CreatePlatformSprites(taskId);
    LoadOakIntroBigSprite(INTRO_OAK, 0);
    BeginNormalPaletteFade(0xFFFFFFFF, 4, 16, 0, 0);
    gTasks[taskId].tSlideOffset = 0;
    gTasks[taskId].func = Task_NewGameOakSpeech_WaitForTextToStart;
    gTasks[taskId].tPlayerSpriteId = 0xFF;
    gTasks[taskId].tPokeBallSpriteId = 0xFF;
    gTasks[taskId].tTimer = 80;
    PlayBGM(MUS_ROUTE30);
    ShowBg(0);
    ShowBg(1);
    ShowBg(2);
}

static void LoadOakIntroBigSprite(u16 which, u16 offset)
{
    u8 *buffer;
    u32 i;

    switch (which)
    {
        case INTRO_GOLD:
            LoadPalette(sOakIntro_GoldPal, BG_PLTT_ID(4), sizeof(sOakIntro_GoldPal));
            LZ77UnCompVram(sOakIntro_GoldTiles, (void *)(VRAM + 0x600 + offset));
            break;
        case INTRO_KRIS:
            LoadPalette(sOakIntro_KrisPal, BG_PLTT_ID(4), sizeof(sOakIntro_KrisPal));
            LZ77UnCompVram(sOakIntro_KrisTiles, (void *)(VRAM + 0x600 + offset));
            break;
        case INTRO_OAK:
            LoadPalette(sOakIntro_OakPal, BG_PLTT_ID(6), sizeof(sOakIntro_OakPal));
            LZ77UnCompVram(sOakIntro_OakTiles, (void *)(VRAM + 0x600 + offset));
            break;
    }

    buffer = AllocZeroed(0x60);

    for (i = 0; i < 0x60; i++)
    {
        buffer[i] = i;
    }

    FillBgTilemapBufferRect(2, 0, 0, 0, 32, 32, 16);
    CopyRectToBgTilemapBufferRect(2, buffer, 0, 0, 8, 12, 11, 2, 8, 12, 16, (offset * 64) + 24, 0);
    CopyBgTilemapBufferToVram(2);
    FREE_AND_SET_NULL(buffer);
}

static void Task_NewGameOakSpeech_WaitForTextToStart(u8 taskId)
{
    if (!gPaletteFade.active)
    {
        if (gTasks[taskId].tTimer)
        {
            gTasks[taskId].tTimer--;
        }
        else
        {
            InitWindows(sOakIntroTextWindows);
            LoadMainMenuWindowFrameTiles(0, 0xF3);
            LoadMessageBoxGfx(0, OAK_INTRO_DLG_BASE_TILE_NUM, BG_PLTT_ID(15));
            NewGameOakSpeech_ShowDialogueWindow(0, 1);
            PutWindowTilemap(0);
            CopyWindowToVram(0, COPYWIN_GFX);
            FillWindowPixelBuffer(0, 0x11);
            StringExpandPlaceholders(gStringVar4, gText_Oak_Welcome);
            AddTextPrinterForMessage(TRUE);
            gTasks[taskId].func = Task_NewGameOakSpeech_PrintThisEllipsis;
        }
    }
}

static void Task_NewGameOakSpeech_PrintThisEllipsis(u8 taskId)
{
    if (!RunTextPrintersAndIsPrinter0Active())
    {
        StringExpandPlaceholders(gStringVar4, gText_Oak_Pokemon);
        AddTextPrinterForMessage(TRUE);
        gTasks[taskId].func = Task_NewGameOakSpeech_CreatePokeBallToReleaseWooper;
    }
}

static void Task_NewGameOakSpeech_CreatePokeBallToReleaseWooper(u8 taskId)
{
    if (!RunTextPrintersAndIsPrinter0Active())
    {
        u8 spriteId = gTasks[taskId].tWooperSpriteId;
        gSprites[spriteId].data[0] = 0;

        CreatePokeballSpriteToReleaseMon(spriteId, gSprites[spriteId].oam.paletteNum, 100, 66, 0, 0, 0x20, 0xFFFF1FFF, SPECIES_WOOPER);
        gTasks[taskId].func = Task_NewGameOakSpeech_PrintIsPokemonWaitForAnimation;
        gTasks[taskId].tTimer = 0;
    }
}

static void Task_NewGameOakSpeech_PrintIsPokemonWaitForAnimation(u8 taskId)
{
    if (gSprites[gTasks[taskId].tWooperSpriteId].animEnded)
    {
        if (gTasks[taskId].tTimer >= 96)
        {
            gTasks[taskId].func = Task_NewGameOakSpeech_MainSpeech1;
        }
    }
        
    if (gTasks[taskId].tTimer < 0x4000)
    {
        gTasks[taskId].tTimer++;
        
        if (gTasks[taskId].tTimer == 32)
        {
            StringExpandPlaceholders(gStringVar4, gText_Oak_Pokemon2);
            AddTextPrinterForMessage(TRUE);
            FillWindowPixelBuffer(0, 0x11);
        }
    }
    RunTextPrinters();
}

static void Task_NewGameOakSpeech_MainSpeech1(u8 taskId)
{
    if (!RunTextPrintersAndIsPrinter0Active())
    {
        StringExpandPlaceholders(gStringVar4, gText_Oak_MainSpeech);
        AddTextPrinterForMessage(TRUE);
        gTasks[taskId].func = Task_NewGameOakSpeech_PutAwayWooper;
    }
}

static void Task_NewGameOakSpeech_PutAwayWooper(u8 taskId)
{
    u8 spriteId;
    if (!RunTextPrintersAndIsPrinter0Active())
    {
        ClearDialogWindowAndFrame(0, TRUE);
        spriteId = gTasks[taskId].tWooperSpriteId;
        gTasks[taskId].tPokeBallSpriteId = CreateTradePokeballSprite(spriteId, gSprites[spriteId].oam.paletteNum, 100, 66, 0, 0, 0x20, 0xFFFF1F3F);
        gTasks[taskId].tTimer2 = 48;
        gTasks[taskId].tTimer = 64;
        gTasks[taskId].func = Task_NewGameOakSpeech_MainSpeech2;
    }
}

static void Task_NewGameOakSpeech_MainSpeech2(u8 taskId)
{
    s16 *data = gTasks[taskId].data;

    if (tTimer != 0)
    {
        if (tTimer < 24)
        {
            gSprites[tWooperSpriteId].y--;
        }
        tTimer--;
    }
    else
    {
        if (tTimer2 == 48)
        {
            FreeAndDestroyMonPicSprite(gTasks[taskId].tWooperSpriteId);
            DestroySprite(&gSprites[tPokeBallSpriteId]);
        }
        
        if (tTimer2 != 0)
        {
            tTimer2--;
        }
        else
        {
            NewGameOakSpeech_ShowDialogueWindow(0, FALSE);
            StringExpandPlaceholders(gStringVar4, gText_Oak_MainSpeech2);
            AddTextPrinterForMessage(TRUE);
            CopyWindowToVram(0, COPYWIN_FULL);
            gTasks[taskId].func = Task_NewGameOakSpeech_StartOakPlatformFade;
        }
    }
}

static void Task_NewGameOakSpeech_StartOakPlatformFade(u8 taskId)
{
    if (!RunTextPrintersAndIsPrinter0Active())
    {
        ClearDialogWindowAndFrame(0, TRUE);
        NewGameOakSpeech_StartFadeOutTarget1InTarget2(taskId, 1);
        gTasks[taskId].tTimer = 48;
        gTasks[taskId].func = Task_NewGameOakSpeech_WaitOakPlatformFade;
    }
}

static void Task_NewGameOakSpeech_WaitOakPlatformFade(u8 taskId)
{
    if (gTasks[taskId].tIsDoneFadingSprites)
    {
        if (gTasks[taskId].tTimer)
        {
            gTasks[taskId].tTimer--;
        }
        else
        {
            gTasks[taskId].func = Task_NewGameOakSpeech_BoyOrGirl;
        }
    }
}

static void Task_NewGameOakSpeech_BoyOrGirl(u8 taskId)
{
    NewGameOakSpeech_ShowDialogueWindow(0, FALSE);
    FillWindowPixelBuffer(0, 0x11);
    StringExpandPlaceholders(gStringVar4, gText_Oak_BoyOrGirl);
    AddTextPrinterForMessage(TRUE);
    CopyWindowToVram(0, COPYWIN_FULL);
    gTasks[taskId].func = Task_NewGameOakSpeech_WaitToShowGenderMenu;
}

static void Task_NewGameOakSpeech_WaitToShowGenderMenu(u8 taskId)
{
    if (!RunTextPrintersAndIsPrinter0Active())
    {
        NewGameOakSpeech_ShowGenderMenu();
        gTasks[taskId].func = Task_NewGameOakSpeech_ChooseGender;
    }
}

static void Task_NewGameOakSpeech_ChooseGender(u8 taskId)
{
    int gender = NewGameOakSpeech_ProcessGenderMenuInput();

    switch (gender)
    {
        case MALE:
            PlaySE(SE_SELECT);
            gSaveBlock2Ptr->playerGender = gender;
            gTasks[taskId].func = Task_NewGameOakSpeech_StartPlayerFadeIn;
            break;
        case FEMALE:
            PlaySE(SE_SELECT);
            gSaveBlock2Ptr->playerGender = gender;
            gTasks[taskId].func = Task_NewGameOakSpeech_StartPlayerFadeIn;
            break;
    }
}

static void Task_NewGameOakSpeech_StartPlayerFadeIn(u8 taskId)
{
    ClearDialogWindowAndFrame(0, TRUE);
    NewGameOakSpeech_ClearGenderWindow(1, 1);
    LoadOakIntroBigSprite(gSaveBlock2Ptr->playerGender, 0);
    NewGameOakSpeech_StartFadeInTarget1OutTarget2(taskId, 1);
    gTasks[taskId].tTimer = 30;
    gTasks[taskId].func = Task_NewGameOakSpeech_WaitForPlayerFadeIn;
}

static void Task_NewGameOakSpeech_WaitForPlayerFadeIn(u8 taskId)
{
    if (gTasks[taskId].tIsDoneFadingSprites)
    {
        if (gTasks[taskId].tTimer)
        {
            gTasks[taskId].tTimer--;
        }
        else
        {
            gTasks[taskId].func = Task_NewGameOakSpeech_WhatsYourName;
        }
    }
}

static void Task_NewGameOakSpeech_WhatsYourName(u8 taskId)
{
    NewGameOakSpeech_ShowDialogueWindow(0, FALSE);
    FillWindowPixelBuffer(0, 0x11);
    StringExpandPlaceholders(gStringVar4, gText_Oak_WhatsYourName);
    AddTextPrinterForMessage(TRUE);
    CopyWindowToVram(0, COPYWIN_FULL);
    gTasks[taskId].func = Task_NewGameOakSpeech_WaitForWhatsYourNameToPrint;
}

static void Task_NewGameOakSpeech_WaitForWhatsYourNameToPrint(u8 taskId)
{
    if (!RunTextPrintersAndIsPrinter0Active())
    {
        BeginNormalPaletteFade(0xFFFFFFFF, 0, 0, 16, RGB_BLACK);
        gTasks[taskId].func = Task_NewGameOakSpeech_StartNamingScreen;
    }
}

static void Task_NewGameOakSpeech_StartNamingScreen(u8 taskId)
{
    if (!gPaletteFade.active)
    {
        FreeAllWindowBuffers();
        NewGameOakSpeech_SetDefaultPlayerName(Random() % 20);
        DestroyTask(taskId);
        DoNamingScreen(NAMING_SCREEN_PLAYER, gSaveBlock2Ptr->playerName, gSaveBlock2Ptr->playerGender, 0, 0, CB2_NewGameOakSpeech_ReturnFromNamingScreen);
    }
}

static void Task_NewGameOakSpeech_SoItsPlayerName(u8 taskId)
{
    if (!gPaletteFade.active)
    {
        NewGameOakSpeech_ShowDialogueWindow(0, 1);
        FillWindowPixelBuffer(0, 0x11);
        StringExpandPlaceholders(gStringVar4, gText_Oak_SoItsPlayer);
        AddTextPrinterForMessage(TRUE);
        gTasks[taskId].tTimer = 30;
        gTasks[taskId].func = Task_NewGameOakSpeech_CreateNameYesNo;
    }
}

static void Task_NewGameOakSpeech_CreateNameYesNo(u8 taskId)
{
    if (!RunTextPrintersAndIsPrinter0Active())
    {
        if (gTasks[taskId].tTimer)
        {
            gTasks[taskId].tTimer--;
        }
        else
        {
            CreateYesNoMenuParameterized(1, 2, 0xF3, 0xD0, 2, 15);
            gTasks[taskId].func = Task_NewGameOakSpeech_ProcessNameYesNoMenu;
        }
    }
}

static void Task_NewGameOakSpeech_ProcessNameYesNoMenu(u8 taskId)
{
    switch (Menu_ProcessInputNoWrapClearOnChoose())
    {
        case 0:
            PlaySE(SE_SELECT);
            ClearDialogWindowAndFrame(0, TRUE);
            gTasks[taskId].func = Task_NewGameOakSpeech_SlidePlatformAway2;
            break;
        case -1:
        case 1:
            PlaySE(SE_SELECT);
            BeginNormalPaletteFade(0xFFFFFFFF, 0, 0, 16, RGB_BLACK);
            gTasks[taskId].func = Task_NewGameOakSpeech_StartNamingScreen;
    }
}

static void Task_NewGameOakSpeech_SlidePlatformAway2(u8 taskId)
{
    u32 i, spriteId;

    if (gTasks[taskId].tSlideOffset)
    {
        gTasks[taskId].tSlideOffset -= 2;
        
        for (i = 0; i < 3; i++)
        {
            spriteId = gTasks[taskId].data[i + 9];
            gSprites[spriteId].x = gTasks[taskId].tSlideOffset + ((i - 1) * 32) + 120;
        }
        
        ChangeBgX(2, 0x200, 1);
    }
    else
    {
        gTasks[taskId].tTimer = 30;
        gTasks[taskId].func = Task_NewGameOakSpeech_AreYouReady;
    }
}

static void Task_NewGameOakSpeech_AreYouReady(u8 taskId)
{
    if (gTasks[taskId].tTimer)
    {
        gTasks[taskId].tTimer--;
    }
    else
    {
        NewGameOakSpeech_ShowDialogueWindow(0, FALSE);
        StringExpandPlaceholders(gStringVar4, gText_Oak_AreYouReady);
        AddTextPrinterForMessage(TRUE);
        CopyWindowToVram(0, COPYWIN_FULL);
        gTasks[taskId].tTimer = 30;
        gTasks[taskId].func = Task_NewGameOakSpeech_PrepareToShrinkPlayer;
    }
}

static void Task_NewGameOakSpeech_PrepareToShrinkPlayer(u8 taskId)
{
    if (!RunTextPrintersAndIsPrinter0Active())
    {
        if (gTasks[taskId].tTimer)
        {
            gTasks[taskId].tTimer--;
        }
        else
        {
            FadeOutBGM(4);
            gTasks[taskId].func = Task_NewGameOakSpeech_ShrinkPlayer;
        }
    }
}

static void Task_NewGameOakSpeech_ShrinkPlayer(u8 taskId)
{
    gTasks[taskId].data[3] = 0;
    Task_NewGameOakSpeech_FadeEverythingButPlayerAndTextbox(taskId);
    Task_NewGameOakSpeech_StartFadePlayerToWhite(taskId);
    Task_NewGameOakSpeech_StartShrinkPlayer(taskId);
}

static void Task_NewGameOakSpeech_StartShrinkPlayer(u8 taskId)
{
    SetBgAttribute(2, 6, 1);
    gTasks[taskId].tTimer = 0;
    gTasks[taskId].data[1] = 0;
    gTasks[taskId].data[2] = 0x100;
    gTasks[taskId].data[15] = 0;
    gTasks[taskId].func = Task_NewGameOakSpeech_ShrinkBG2;
}

static void Task_NewGameOakSpeech_ShrinkBG2(u8 taskId)
{
    u16 isBetweenSteps;
    u16 oldScale;

    gTasks[taskId].data[3]++;
    
    isBetweenSteps = (u16)gTasks[taskId].data[3] % 20;

    if (!isBetweenSteps)
    {
        if (gTasks[taskId].data[3] == 40)
        {
            PlaySE(SE_WARP_IN);
        }

        oldScale = gTasks[taskId].data[2];
        gTasks[taskId].data[2] -= 0x20;

        SetBgAffine(2, 120 * 0x100, 84 * 0x100, 120, 84, MathUtil_Inv16(oldScale - 8), MathUtil_Inv16(gTasks[taskId].data[2] - 16), 0);

        if (gTasks[taskId].data[2] <= 0x60)
        {
            gTasks[taskId].data[15] = 1;
            gTasks[taskId].tTimer = 36;
            gTasks[taskId].func = Task_NewGameOakSpeech_FadePlayerToBlack;
        }
    }
}

static void Task_NewGameOakSpeech_FadeEverythingButPlayerAndTextbox(u8 taskId)
{
    u8 taskId2 = CreateTask(Task_NewGameOakSpeech_WaitToFadeTextbox, 1);

    gTasks[taskId2].data[0] = 0;
    gTasks[taskId2].data[1] = 0;
    gTasks[taskId2].data[2] = 0;
    gTasks[taskId2].data[15] = 0;

    BeginNormalPaletteFade(0xFFFF0FCF, 4, 0, 16, RGB_BLACK);
}

static void Task_NewGameOakSpeech_WaitToFadeTextbox(u8 taskId)
{
    u32 i;
    s16 *data = gTasks[taskId].data;


    if (!gPaletteFade.active)
    {
        if (data[1])
        {
            for (i = 0; i < 3; i++)
            {
                DestroySprite(&gSprites[data[i + 9]]);
            }

            FreeSpriteTilesByTag(0x1000);
            FreeSpritePaletteByTag(0x1000);

            DestroyTask(taskId);
        }
        else
        {
            data[1]++;
            BeginNormalPaletteFade(0x0000F000, 0, 0, 16, RGB_BLACK);
        }
    }
}

static void Task_NewGameOakSpeech_StartFadePlayerToWhite(u8 taskId)
{
    u8 taskId2 = CreateTask(Task_NewGameOakSpeech_FadePlayerToWhite, 2);

    gTasks[taskId2].data[0] = 8;
    gTasks[taskId2].data[1] = 0;
    gTasks[taskId2].data[2] = 8;
    gTasks[taskId2].data[14] = 0;
    gTasks[taskId2].data[15] = 0;
}

static void Task_NewGameOakSpeech_FadePlayerToWhite(u8 taskId)
{
    u32 i;
    s16 *data = gTasks[taskId].data;

    if (data[0])
    {
        data[0]--;
    }
    else
    {
        if (data[1] <= 0 && data[2] != 0)
        {
            data[2]--;
        }

        BlendPalette(0x40, 0x20, data[14], RGB_WHITE);

        data[14]++;
        data[1]--;
        data[0] = data[2];

        if (data[14] > 14)
        {
            for (i = 0; i < 32; i++)
            {
                gPlttBufferFaded[i + 0x40] = RGB_WHITE;
                gPlttBufferUnfaded[i + 0x40] = RGB_WHITE;
            }
            DestroyTask(taskId);
        }
    }
}

static void Task_NewGameOakSpeech_FadePlayerToBlack(u8 taskId)
{
    if (gTasks[taskId].tTimer)
    {
        gTasks[taskId].tTimer--;
    }
    else
    {
        BeginNormalPaletteFade(0x00000030, 2, 0, 16, RGB_BLACK);
        gTasks[taskId].func = Task_NewGameOakSpeech_Cleanup;
    }
}

static void Task_NewGameOakSpeech_Cleanup(u8 taskId)
{
    if (!gPaletteFade.active)
    {
        FreeAllWindowBuffers();
        ResetAllPicSprites();
        FREE_AND_SET_NULL(sOakIntro_BgBuffer);
        SetMainCallback2(CB2_NewGame);
        DestroyTask(taskId);
    }
}

static void CB2_NewGameOakSpeech_ReturnFromNamingScreen(void)
{
    u32 i;
    u8 taskId;
    u8 spriteId;
    u16 savedIme;

    ResetBgsAndClearDma3BusyFlags(0);
    SetGpuReg(REG_OFFSET_DISPCNT, 0);
    SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_OBJ_ON | DISPCNT_OBJ_1D_MAP);
    InitBgsFromTemplates(1, sOakBgTemplates, ARRAY_COUNT(sOakBgTemplates));
    SetBgTilemapBuffer(2, sOakIntro_BgBuffer);
    SetVBlankCallback(NULL);
    SetGpuReg(REG_OFFSET_BG2CNT, 0);
    SetGpuReg(REG_OFFSET_BG1CNT, 0);
    SetGpuReg(REG_OFFSET_BG0CNT, 0);
    SetGpuReg(REG_OFFSET_BG2HOFS, 0);
    SetGpuReg(REG_OFFSET_BG2VOFS, 0);
    SetGpuReg(REG_OFFSET_BG1HOFS, 0);
    SetGpuReg(REG_OFFSET_BG1VOFS, 0);
    SetGpuReg(REG_OFFSET_BG0HOFS, 0);
    SetGpuReg(REG_OFFSET_BG0VOFS, 0);
    DmaFill16(3, 0, VRAM, VRAM_SIZE);
    DmaFill32(3, 0, OAM, OAM_SIZE);
    DmaFill16(3, 0, PLTT, PLTT_SIZE);
    ResetPaletteFade();
    LZ77UnCompVram(sOakSpeechBgGfx, (u8*)VRAM);
    LZ77UnCompVram(sOakSpeechBgMap, (u8*)(BG_SCREEN_ADDR(30)));
    LoadPalette(sOakSpeechBgPal, BG_PLTT_ID(0), sizeof(sOakSpeechBgPal));
    ResetTasks();
    taskId = CreateTask(Task_NewGameOakSpeech_ReturnFromNamingScreenShowTextbox, 0);
    gTasks[taskId].tTimer = 5;
    gTasks[taskId].tSlideOffset = 60;
    ScanlineEffect_Stop();
    ResetSpriteData();
    FreeAllSpritePalettes();
    ResetAllPicSprites();
    NewGameOakSpeech_CreatePlatformSprites(taskId);
    LoadOakIntroBigSprite(gSaveBlock2Ptr->playerGender, 0);

    for (i = 0; i < 3; i++)
    {
        spriteId = gTasks[taskId].data[i + 9];
        gSprites[spriteId].x = 148 + (i * 32);
    }
    
    ChangeBgX(2, 60 * -0x100, 0);
    BeginNormalPaletteFade(0xFFFFFFFF, 0, 16, 0, RGB_BLACK);
    SetGpuReg(REG_OFFSET_WIN0H, 0);
    SetGpuReg(REG_OFFSET_WIN0V, 0);
    SetGpuReg(REG_OFFSET_WININ, 0);
    SetGpuReg(REG_OFFSET_WINOUT, 0);
    SetGpuReg(REG_OFFSET_BLDCNT, 0);
    SetGpuReg(REG_OFFSET_BLDALPHA, 0);
    SetGpuReg(REG_OFFSET_BLDY, 0);
    ShowBg(0);
    ShowBg(1);
    ShowBg(2);
    savedIme = REG_IME;
    REG_IME = 0;
    REG_IE |= 1;
    REG_IME = savedIme;
    SetVBlankCallback(VBlankCB_MainMenu);
    SetMainCallback2(CB2_MainMenu);
    InitWindows(sOakIntroTextWindows);
    LoadMainMenuWindowFrameTiles(0, 0xF3);
    LoadMessageBoxGfx(0, OAK_INTRO_DLG_BASE_TILE_NUM, BG_PLTT_ID(15));
    PutWindowTilemap(0);
    CopyWindowToVram(0, COPYWIN_FULL);
}

static void SpriteCB_Null(struct Sprite *sprite)
{
}

static void NewGameOakSpeech_CreateWooperSprite(u8 taskId)
{
    // expansion renamed CreatePicSprite2 and takes a shiny flag rather than an OT id.
    u8 wooperSprite = CreateMonPicSprite_Affine(SPECIES_WOOPER, FALSE, 0, MON_PIC_AFFINE_FRONT, 96, 96, 14, TAG_NONE);
    gSprites[wooperSprite].callback = SpriteCB_Null;
    gSprites[wooperSprite].oam.priority = 0;
    gSprites[wooperSprite].invisible = TRUE;
    gTasks[taskId].tWooperSpriteId = wooperSprite;
}

static void NewGameOakSpeech_CreatePlatformSprites(u8 taskId)
{
    u32 i;
    u8 spriteId;

    LoadCompressedSpriteSheet(&sCompressedSpriteSheet_OakPlatform);
    LoadSpritePalette(&sSpritePalette_OakPlatform);

    for (i = 0; i < 3; i++)
    {
        spriteId = CreateSprite(&sSpriteTemplate_OakPlatform, 88 + (i * 32), 112, 1);
        gTasks[taskId].data[i + 9] = spriteId;
        StartSpriteAnim(&gSprites[spriteId], i);
    }
}

#undef tPlayerSpriteId
#undef tSlideOffset
#undef tPlayerGender
#undef tWooperSpriteId
#undef tLotadSpriteId
#undef tBrendanSpriteId
#undef tMaySpriteId

#define tMainTask data[0]
#define tAlphaCoeff1 data[1]
#define tAlphaCoeff2 data[2]
#define tDelay data[3]
#define tDelayTimer data[4]

static void Task_NewGameOakSpeech_FadeOutTarget1InTarget2(u8 taskId)
{
    u32 i;
    int alphaCoeff2;

    if (gTasks[taskId].tAlphaCoeff1 == 0)
    {
        gTasks[gTasks[taskId].tMainTask].tIsDoneFadingSprites = TRUE;
        DestroyTask(taskId);
    }
    else if (gTasks[taskId].tDelayTimer)
    {
        gTasks[taskId].tDelayTimer--;
    }
    else
    {
        gTasks[taskId].tDelayTimer = gTasks[taskId].tDelay;
        gTasks[taskId].tAlphaCoeff1--;
        gTasks[taskId].tAlphaCoeff2++;
        
        if (gTasks[taskId].tAlphaCoeff1 == 8)
        {
            s16 *sprites = &gTasks[gTasks[taskId].tMainTask].data[9];
            for (i = 0; i < 3; i++)
            {
                gSprites[sprites[i]].invisible = TRUE;
            }
        }

        alphaCoeff2 = gTasks[taskId].tAlphaCoeff2 << 8;
        SetGpuReg(REG_OFFSET_BLDALPHA, gTasks[taskId].tAlphaCoeff1 + alphaCoeff2);
    }
}

static void NewGameOakSpeech_StartFadeOutTarget1InTarget2(u8 taskId, u8 delay)
{
    u8 taskId2;

    SetGpuReg(REG_OFFSET_BLDCNT, BLDCNT_TGT2_BG1 | BLDCNT_TGT2_OBJ | BLDCNT_EFFECT_BLEND | BLDCNT_TGT1_BG2);
    SetGpuReg(REG_OFFSET_BLDALPHA, BLDALPHA_BLEND(16, 0));
    SetGpuReg(REG_OFFSET_BLDY, 0);
    gTasks[taskId].tIsDoneFadingSprites = 0;
    taskId2 = CreateTask(Task_NewGameOakSpeech_FadeOutTarget1InTarget2, 0);
    gTasks[taskId2].tMainTask = taskId;
    gTasks[taskId2].tAlphaCoeff1 = 16;
    gTasks[taskId2].tAlphaCoeff2 = 0;
    gTasks[taskId2].tDelay = delay;
    gTasks[taskId2].tDelayTimer = delay;
}

static void Task_NewGameOakSpeech_FadeInTarget1OutTarget2(u8 taskId)
{
    u32 i;
    int alphaCoeff2;

    if (gTasks[taskId].tAlphaCoeff1 == 16)
    {
        gTasks[gTasks[taskId].tMainTask].tIsDoneFadingSprites = TRUE;
        DestroyTask(taskId);
    }
    else if (gTasks[taskId].tDelayTimer)
    {
        gTasks[taskId].tDelayTimer--;
    }
    else
    {
        gTasks[taskId].tDelayTimer = gTasks[taskId].tDelay;
        gTasks[taskId].tAlphaCoeff1++;
        gTasks[taskId].tAlphaCoeff2--;

        if (gTasks[taskId].tAlphaCoeff1 == 8)
        {
            s16 *sprites = &gTasks[gTasks[taskId].tMainTask].data[9];
            for (i = 0; i < 3; i++)
            {
                gSprites[sprites[i]].invisible = FALSE;
            }
        }

        alphaCoeff2 = gTasks[taskId].tAlphaCoeff2 << 8;
        SetGpuReg(REG_OFFSET_BLDALPHA, gTasks[taskId].tAlphaCoeff1 + alphaCoeff2);
    }
}

static void NewGameOakSpeech_StartFadeInTarget1OutTarget2(u8 taskId, u8 delay)
{
    u8 taskId2;

    SetGpuReg(REG_OFFSET_BLDCNT, BLDCNT_TGT2_BG1 | BLDCNT_TGT2_OBJ | BLDCNT_EFFECT_BLEND | BLDCNT_TGT1_BG2);
    SetGpuReg(REG_OFFSET_BLDALPHA, BLDALPHA_BLEND(0, 16));
    SetGpuReg(REG_OFFSET_BLDY, 0);
    gTasks[taskId].tIsDoneFadingSprites = 0;
    taskId2 = CreateTask(Task_NewGameOakSpeech_FadeInTarget1OutTarget2, 0);
    gTasks[taskId2].tMainTask = taskId;
    gTasks[taskId2].tAlphaCoeff1 = 0;
    gTasks[taskId2].tAlphaCoeff2 = 16;
    gTasks[taskId2].tDelay = delay;
    gTasks[taskId2].tDelayTimer = delay;
}

#undef tMainTask
#undef tAlphaCoeff1
#undef tAlphaCoeff2
#undef tDelay
#undef tDelayTimer

static void NewGameOakSpeech_ShowGenderMenu(void)
{
    DrawMainMenuWindowBorder(&sOakIntroTextWindows[1], 0xF3);
    FillWindowPixelBuffer(1, PIXEL_FILL(1));
    PrintMenuTable(1, ARRAY_COUNT(sMenuActions_Gender), sMenuActions_Gender);
    InitMenuInUpperLeftCornerPlaySoundWhenAPressed(1, 2, 0, 1, 16, 2, 0);
    PutWindowTilemap(1);
    CopyWindowToVram(1, COPYWIN_FULL);
}

static s8 NewGameOakSpeech_ProcessGenderMenuInput(void)
{
    return Menu_ProcessInputNoWrap();
}

static void NewGameOakSpeech_SetDefaultPlayerName(u8 nameId)
{
    const u8* name;
    u32 i;

    if (gSaveBlock2Ptr->playerGender == MALE)
        name = gMalePresetNames[nameId];
    else
        name = gFemalePresetNames[nameId];
    for (i = 0; i < PLAYER_NAME_LENGTH && name[i] != EOS; i++)
        gSaveBlock2Ptr->playerName[i] = name[i];
    for (; i < PLAYER_NAME_LENGTH + 1; i++)
        gSaveBlock2Ptr->playerName[i] = EOS;
}


static void NewGameOakSpeech_ClearGenderWindowTilemap(u8 a, u8 b, u8 c, u8 d, u8 e, u8 unused)
{
    FillBgTilemapBufferRect(a, 0, b + 0xFF, c + 0xFF, d + 2, e + 2, 2);
}

static void NewGameOakSpeech_ClearGenderWindow(u32 windowId, u32 copyToVram)
{
    CallWindowFunction(windowId, NewGameOakSpeech_ClearGenderWindowTilemap);
    FillWindowPixelBuffer(windowId, PIXEL_FILL(1));
    ClearWindowTilemap(windowId);
    if (copyToVram == TRUE)
        CopyWindowToVram(windowId, COPYWIN_FULL);
}


static void NewGameOakSpeech_ShowDialogueWindow(u32 windowId, u32 copyToVram)
{
    // Crystal Expansion (D110): CrystalDust drew this border itself, from a
    // twenty-tile message box laid out its own way. D106 replaced
    // message_box.png with expansion's fourteen-tile sheet, so that hand-
    // written tilemap was indexing art that no longer matched it. Draw the
    // same frame the field message box draws, at the base tile the intro
    // loads it to.
    DrawDialogFrameWithCustomTileAndPalette(windowId, copyToVram, OAK_INTRO_DLG_BASE_TILE_NUM, 15);
}

static void Task_NewGameOakSpeech_ReturnFromNamingScreenShowTextbox(u8 taskId)
{
    if (gTasks[taskId].tTimer-- <= 0)
    {
        gTasks[taskId].func = Task_NewGameOakSpeech_SoItsPlayerName;
    }
}

#undef tTimer

