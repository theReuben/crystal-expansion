#include "global.h"
#include "bg.h"
#include "gpu_regs.h"
#include "international_string_util.h"
#include "main.h"
#include "malloc.h"
#include "menu.h"
#include "m4a.h"
#include "palette.h"
#include "pokegear_map.h"
#include "strings.h"
#include "text.h"
#include "text_window.h"
#include "window.h"
#include "constants/rgb.h"
#include "constants/songs.h"

/*
 *  The map shown when the player looks at a wall-mounted Town Map -- the one
 *  in the bedroom, and the ones beside the PC in every Pokemon Center. It does
 *  not zoom, and A or B closes it.
 *
 *  This is CrystalDust's version, restored in D107: the Phase 1 merge kept
 *  expansion's, which draws Hoenn. It drives the Johto/Kanto map in
 *  pokegear_map.c, hence the CDMap_ prefixes (see D33).
 */

static EWRAM_DATA struct {
    MainCallback callback;
    u32 unused;
    struct CDRegionMap regionMap;
    u16 state;
} *sFieldRegionMapHandler = NULL;

static void MCB2_InitRegionMapRegisters(void);
static void VBCB_FieldUpdateRegionMap(void);
static void MCB2_FieldUpdateRegionMap(void);
static void FieldUpdateRegionMap(void);
static void ShowHelpBar(bool8 onButton);

static const struct BgTemplate sFieldRegionMapBgTemplates[] = {
    {
        .bg = 0,
        .charBaseIndex = 0,
        .mapBaseIndex = 31,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 0,
        .baseTile = 0
    },
    {
        .bg = 2,
        .charBaseIndex = 2,
        .mapBaseIndex = 29,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 2,
        .baseTile = 0
    },
    {
        .bg = 3,
        .charBaseIndex = 3,
        .mapBaseIndex = 28,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 3,
        .baseTile = 0
    }
};

static const struct WindowTemplate sFieldRegionMapWindowTemplates[] =
{
    {
        .bg = 0,
        .tilemapLeft = 0,
        .tilemapTop = 0,
        .width = 30,
        .height = 2,
        .paletteNum = 14,
        .baseBlock = 61
    },
    DUMMY_WIN_TEMPLATE
};

void FieldInitRegionMap(MainCallback callback)
{
    SetVBlankCallback(NULL);
    sFieldRegionMapHandler = Alloc(sizeof(*sFieldRegionMapHandler));
    sFieldRegionMapHandler->state = 0;
    sFieldRegionMapHandler->callback = callback;
    SetMainCallback2(MCB2_InitRegionMapRegisters);
}

static void MCB2_InitRegionMapRegisters(void)
{
    SetGpuReg(REG_OFFSET_DISPCNT, 0);
    SetGpuReg(REG_OFFSET_BG0HOFS, 0);
    SetGpuReg(REG_OFFSET_BG0VOFS, 0);
    SetGpuReg(REG_OFFSET_BG1HOFS, 0);
    SetGpuReg(REG_OFFSET_BG1VOFS, 0);
    SetGpuReg(REG_OFFSET_BG2HOFS, 0);
    SetGpuReg(REG_OFFSET_BG2VOFS, 0);
    SetGpuReg(REG_OFFSET_BG3HOFS, 0);
    SetGpuReg(REG_OFFSET_BG3VOFS, 0);
    ResetSpriteData();
    FreeAllSpritePalettes();
    ResetBgsAndClearDma3BusyFlags(0);
    InitBgsFromTemplates(0, sFieldRegionMapBgTemplates, ARRAY_COUNT(sFieldRegionMapBgTemplates));
    InitWindows(sFieldRegionMapWindowTemplates);
    DeactivateAllTextPrinters();
    ClearScheduledBgCopiesToVram();
    SetMainCallback2(MCB2_FieldUpdateRegionMap);
    SetVBlankCallback(VBCB_FieldUpdateRegionMap);
}

static void VBCB_FieldUpdateRegionMap(void)
{
    LoadOam();
    ProcessSpriteCopyRequests();
    TransferPlttBuffer();
}

static void MCB2_FieldUpdateRegionMap(void)
{
    FieldUpdateRegionMap();
    AnimateSprites();
    BuildOamBuffer();
    UpdatePaletteFade();
    DoScheduledBgTilemapCopiesToVram();
}

static void FieldUpdateRegionMap(void)
{
    switch (sFieldRegionMapHandler->state)
    {
        case 0:
            CDMap_InitRegionMap(&sFieldRegionMapHandler->regionMap, MAPMODE_FIELD, 0, 0);
            CDMap_CreateRegionMapPlayerIcon(0, 0);
            CDMap_CreateRegionMapCursor(1, 1, TRUE);
            CDMap_CreateSecondaryLayerDots(2, 2);
            CDMap_CreateRegionMapName(3, 4);
            ShowHelpBar(FALSE);
            sFieldRegionMapHandler->state++;
            break;
        case 1:
            BeginNormalPaletteFade(PALETTES_ALL, 0, 16, 0, RGB_BLACK);
            sFieldRegionMapHandler->state++;
            break;
        case 2:
            SetGpuRegBits(REG_OFFSET_DISPCNT, DISPCNT_OBJ_1D_MAP | DISPCNT_OBJ_ON);
            ShowBg(0);
            ShowBg(2);
            sFieldRegionMapHandler->state++;
            break;
        case 3:
            if (!gPaletteFade.active)
            {
                sFieldRegionMapHandler->state++;
            }
            break;
        case 4:
            switch (CDMap_DoRegionMapInputCallback())
            {
                case MAP_INPUT_MOVE_END:
                    CDMap_PlaySEForSelectedMapsec();
                    switch (CDMap_GetSelectedMapsecLandmarkState())
                    {
                        case LANDMARK_STATE_CLOSE:
                            ShowHelpBar(TRUE);
                            break;
                        default:
                            ShowHelpBar(FALSE);
                            break;
                    }
                    break;
                case MAP_INPUT_CANCEL:
                    sFieldRegionMapHandler->state++;
                    break;
            }
            break;
        case 5:
            BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
            sFieldRegionMapHandler->state++;
            break;
        case 6:
            if (!gPaletteFade.active)
            {
                CDMap_FreeRegionMapResources();
                SetMainCallback2(sFieldRegionMapHandler->callback);
                if (sFieldRegionMapHandler != NULL)
                {
                    FREE_AND_SET_NULL(sFieldRegionMapHandler);
                }
                FreeAllWindowBuffers();
            }
            break;
    }
}

static void ShowHelpBar(bool8 onButton)
{
    const u8 color[3] = { 15, 1, 2 };

    FillWindowPixelBuffer(0, PIXEL_FILL(15));
    AddTextPrinterParameterized3(0, FONT_NORMAL, 144, 0, color, 0, gText_DpadMove);

    if (onButton)
    {
        AddTextPrinterParameterized3(0, FONT_NORMAL, 192, 0, color, 0, gText_ACancel);
    }

    PutWindowTilemap(0);
    CopyWindowToVram(0, COPYWIN_FULL);
}
