#include "global.h"
#include "bug_catching_contest.h"
#include "main.h"
#include "text.h"
#include "menu.h"
#include "malloc.h"
#include "gpu_regs.h"
#include "palette.h"
#include "party_menu.h"
#include "trig.h"
#include "constants/maps.h"
#include "overworld.h"
#include "event_data.h"
#include "secret_base.h"
#include "string_util.h"
#include "international_string_util.h"
#include "sound.h"
#include "strings.h"
#include "text_window.h"
#include "constants/songs.h"
#include "m4a.h"
#include "field_effect.h"
#include "field_specials.h"
#include "fldeff.h"
#include "region_map.h"
#include "pokegear_map.h"
#include "regions.h"
#include "constants/region_map_sections.h"
#include "heal_location.h"
#include "constants/field_specials.h"
#include "constants/heal_locations.h"
#include "constants/map_types.h"
#include "constants/rgb.h"
#include "constants/weather.h"

/*
 *  This file handles region maps generally, and the map used when selecting a fly destination.
 *  Specific features of other region map uses are handled elsewhere
 *
 *  For the region map in the pokenav, see pokenav_region_map.c
 *  For the region map in the pokedex, see pokdex_area_screen.c/pokedex_area_region_map.c
 *  For the region map that can be viewed on the wall of pokemon centers, see field_region_map.c
 *
 */

#define MAP_WIDTH 22
#define MAP_HEIGHT 15
#define MAPCURSOR_X_MIN 4
#define MAPCURSOR_Y_MIN 4

struct WindowCoords
{
    u16 x1;
    u16 y1;
    u16 x2;
    u16 y2;
};

#define FLYDESTICON_RED_OUTLINE 6

enum {
    TAG_CURSOR,
    TAG_PLAYER_ICON,
    TAG_FLY_ICON,
};

// Static type declarations

// struct MultiNameFlyDest removed with CrystalDust's Fly map (D33).

// Static RAM declarations

static EWRAM_DATA struct CDRegionMap *gRegionMap = NULL;

// sFlyMap removed with CrystalDust's Fly map (D33).

// Static ROM declarations

static u8 CDMap_ProcessRegionMapInput_Full(void);
static u8 CDMap_MoveRegionMapCursor_Full(void);
static u8 CDMap_GetMapSecIdAt(s16 x, s16 y, u8 region, bool8 secondary);
static void RegionMap_SetBG2XAndBG2Y(s16 x, s16 y);
static void CDMap_InitMapBasedOnPlayerLocation(void);
static void CDMap_InitMapBasedOnPlayerLocation_(void);
static void CDMap_RegionMap_InitializeStateBasedOnSSTidalLocation(void);
static u8 CDMap_GetMapsecType(u16 mapSecId);
static u16 CDMap_CorrectSpecialMapSecId_Internal(u16 mapSecId);
static u16 CDMap_GetTerraOrMarineCaveMapSecId(void);
static void CDMap_GetMarineCaveCoords(u16 *x, u16 *y);
static bool32 CDMap_IsPlayerInAquaHideout(u8 mapSecId);
static void CDMap_GetPositionOfCursorWithinMapSec(void);
static bool8 CDMap_RegionMap_IsMapSecIdInNextRow(u16 y);
static void CDMap_SpriteCB_CursorMapFull(struct Sprite *sprite);
static void CDMap_HideRegionMapPlayerIcon(void);
static void CDMap_UnhideRegionMapPlayerIcon(void);
static void CDMap_SpriteCB_ShipIcon(struct Sprite *sprite);
static void CDMap_LoadPrimaryLayerMapSec(void);
static void CDMap_LoadSecondaryLayerMapSec(void);
static void CDMap_SetupShadowBoxes(u8 layerNum, const struct WindowCoords *coords);
static u8 CDMap_GetMapSecStatusByLayer(u8 layer);
static void CDMap_SetShadowBoxState(u8 offset, bool8 hide);

// .rodata
static const u16 sRegionMapCursorPal[] = INCBIN_U16("graphics/region_map/cursor.gbapal");
static const u32 sRegionMapCursorGfxLZ[] = INCBIN_U32("graphics/region_map/cursor.4bpp.lz");
static const u16 sRegionMapPal[] = INCBIN_U16("graphics/region_map/region_map.gbapal");
static const u32 sRegionMapTileset[] = INCBIN_U32("graphics/region_map/region_map.4bpp.lz");
static const u32 sRegionMapJohtoTilemap[] = INCBIN_U32("graphics/region_map/johto_map.bin.lz");
static const u32 sRegionMapKantoTilemap[] = INCBIN_U32("graphics/region_map/kanto_map.bin.lz");
static const u16 sRegionMapTownNames_Pal[] = INCBIN_U16("graphics/region_map/town_names.gbapal");
static const u16 sRegionMapPlayerIcon_GoldPal[] = INCBIN_U16("graphics/region_map/gold_icon.gbapal");
static const u32 sRegionMapPlayerIcon_GoldGfx[] = INCBIN_U32("graphics/region_map/gold_icon.4bpp");
static const u16 sRegionMapPlayerIcon_KrisPal[] = INCBIN_U16("graphics/region_map/kris_icon.gbapal");
static const u32 sRegionMapPlayerIcon_KrisGfx[] = INCBIN_U32("graphics/region_map/kris_icon.4bpp");
static const u16 sRegionMapPlayerIcon_ShipPal[] = INCBIN_U16("graphics/region_map/ship_icon.gbapal");
static const u32 sRegionMapPlayerIcon_ShipGfx[] = INCBIN_U32("graphics/region_map/ship_icon.4bpp");
static const u32 sRegionMapDots_Gfx[] = INCBIN_U32("graphics/region_map/dots.4bpp");
static const u16 sRegionMapDots_Pal[] = INCBIN_U16("graphics/region_map/dots.gbapal");
static const u32 sRegionMapNames_Gfx[] = INCBIN_U32("graphics/region_map/region_names.4bpp");
static const u32 sRegionMapNamesCurve_Gfx[] = INCBIN_U32("graphics/region_map/region_names_curve.4bpp");

static const u8 sMapSectionLayout_JohtoPrimary[] = INCBIN_U8("graphics/region_map/mapsec_layout_johto_primary.bin");
static const u8 sMapSectionLayout_JohtoSecondary[] = INCBIN_U8("graphics/region_map/mapsec_layout_johto_secondary.bin");
static const u8 sMapSectionLayout_KantoPrimary[] = INCBIN_U8("graphics/region_map/mapsec_layout_kanto_primary.bin");
static const u8 sMapSectionLayout_KantoSecondary[] = INCBIN_U8("graphics/region_map/mapsec_layout_kanto_secondary.bin");

// region_map_names_emerald.h dropped: its MAPSECEM_* constants were lost in the Phase 1 merge and the table is unused here (D33).
#include "data/region_map/mapsec_flags.h"
#include "data/region_map/mapsec_to_region.h"
// Both tables lost their MAPSEC_SEVII_ISLE_6..9 rows: both index MAPSEC_SEVII_ISLE_6..9,
// which this tree's region_map_sections.json does not define, and neither table is
// referenced from here. See D33.

static const u16 sRegionMap_SpecialPlaceLocations[][2] =
{
    {MAPSEC_NONE, MAPSEC_NONE}
};

static const u16 sMarineCaveMapSecIds[] =
{
    MAPSEC_MARINE_CAVE,
    MAPSEC_UNDERWATER_MARINE_CAVE,
    MAPSEC_UNDERWATER_MARINE_CAVE
};

static const u16 sTerraOrMarineCaveMapSecIds[ABNORMAL_WEATHER_LOCATIONS] =
{
    [ABNORMAL_WEATHER_ROUTE_114_NORTH - 1] = MAPSEC_ROUTE_42,
    [ABNORMAL_WEATHER_ROUTE_114_SOUTH - 1] = MAPSEC_ROUTE_42,
    [ABNORMAL_WEATHER_ROUTE_115_WEST  - 1] = MAPSEC_ROUTE_43,
    [ABNORMAL_WEATHER_ROUTE_115_EAST  - 1] = MAPSEC_ROUTE_43,
    [ABNORMAL_WEATHER_ROUTE_116_NORTH - 1] = MAPSEC_ROUTE_44,
    [ABNORMAL_WEATHER_ROUTE_116_SOUTH - 1] = MAPSEC_ROUTE_44,
    [ABNORMAL_WEATHER_ROUTE_118_EAST  - 1] = MAPSEC_ROUTE_46,
    [ABNORMAL_WEATHER_ROUTE_118_WEST  - 1] = MAPSEC_ROUTE_46,
    [ABNORMAL_WEATHER_ROUTE_105_NORTH - 1] = MAPSEC_ROUTE_33,
    [ABNORMAL_WEATHER_ROUTE_105_SOUTH - 1] = MAPSEC_ROUTE_33,
    [ABNORMAL_WEATHER_ROUTE_125_WEST  - 1] = MAPSEC_ROUTE_125,
    [ABNORMAL_WEATHER_ROUTE_125_EAST  - 1] = MAPSEC_ROUTE_125,
    [ABNORMAL_WEATHER_ROUTE_127_NORTH - 1] = MAPSEC_ROUTE_127,
    [ABNORMAL_WEATHER_ROUTE_127_SOUTH - 1] = MAPSEC_ROUTE_127,
    [ABNORMAL_WEATHER_ROUTE_129_WEST  - 1] = MAPSEC_ROUTE_129,
    [ABNORMAL_WEATHER_ROUTE_129_EAST  - 1] = MAPSEC_ROUTE_129
};

#define MARINE_CAVE_COORD(location)(ABNORMAL_WEATHER_##location - MARINE_CAVE_LOCATIONS_START)

static const struct UCoords16 sMarineCaveLocationCoords[MARINE_CAVE_LOCATIONS] =
{
    [MARINE_CAVE_COORD(ROUTE_105_NORTH)] = {0, 10},
    [MARINE_CAVE_COORD(ROUTE_105_SOUTH)] = {0, 12},
    [MARINE_CAVE_COORD(ROUTE_125_WEST)]  = {24, 3},
    [MARINE_CAVE_COORD(ROUTE_125_EAST)]  = {25, 4},
    [MARINE_CAVE_COORD(ROUTE_127_NORTH)] = {25, 6},
    [MARINE_CAVE_COORD(ROUTE_127_SOUTH)] = {25, 7},
    [MARINE_CAVE_COORD(ROUTE_129_WEST)]  = {24, 10},
    [MARINE_CAVE_COORD(ROUTE_129_EAST)]  = {24, 10}
};

static const u8 sMapSecAquaHideoutOld[] =
{
    MAPSEC_AQUA_HIDEOUT_OLD
};

static const struct OamData sRegionMapCursorOam =
{
    .shape = SPRITE_SHAPE(16x16),
    .size = SPRITE_SIZE(16x16),
    .priority = 2
};

static const union AnimCmd sRegionMapCursorAnim1[] =
{
    ANIMCMD_FRAME(0, 20),
    ANIMCMD_FRAME(4, 20),
    ANIMCMD_JUMP(0)
};

static const union AnimCmd *const sRegionMapCursorAnimTable[] = {
    sRegionMapCursorAnim1
};

static const struct SpritePalette sRegionMapCursorSpritePalette =
{
    .data = sRegionMapCursorPal,
    .tag = TAG_CURSOR
};

static const struct SpriteTemplate sRegionMapCursorSpriteTemplate =
{
    .tileTag = TAG_CURSOR,
    .paletteTag = TAG_CURSOR,
    .oam = &sRegionMapCursorOam,
    .anims = sRegionMapCursorAnimTable,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = CDMap_SpriteCB_CursorMapFull
};

static const struct OamData sRegionMapPlayerIconOam =
{
    .shape = SPRITE_SHAPE(16x16),
    .size = SPRITE_SIZE(16x16),
    .priority = 2
};

static const struct OamData sRegionMapShipIconOam =
{
    .shape = SPRITE_SHAPE(32x32),
    .size = SPRITE_SIZE(32x32),
    .priority = 2
};

static const struct OamData sRegionMapDotsOam = {
    .priority = 2
};

static const union AnimCmd sRegionMapDotsAnim1[] = {
    ANIMCMD_FRAME(0, 5),
    ANIMCMD_END
};

static const union AnimCmd sRegionMapDotsAnim2[] = {
    ANIMCMD_FRAME(1, 5),
    ANIMCMD_END
};

static const union AnimCmd *const sRegionMapDotsAnimTable[] = {
    sRegionMapDotsAnim1,
    sRegionMapDotsAnim2
};

static const struct OamData sRegionMapNameCurveOam = {
    .shape = ST_OAM_SQUARE, .size = 0, .priority = 2
};

static const struct SpriteTemplate sRegionMapNameCurveSpriteTemplate = {
    0,
    0,
    &sRegionMapNameCurveOam,
    gDummySpriteAnimTable,
    NULL,
    gDummySpriteAffineAnimTable,
    SpriteCallbackDummy
};

static const struct OamData sRegionMapNameOam = {
    .shape = ST_OAM_H_RECTANGLE, .size = 1, .priority = 2
};

static const union AnimCmd sRegionMapNameJohto[] = {
    ANIMCMD_FRAME(0, 5),
    ANIMCMD_END
};

static const union AnimCmd sRegionMapNameKanto[] = {
    ANIMCMD_FRAME(4, 5),
    ANIMCMD_END
};

static const union AnimCmd sRegionMapNameSevii[] = {
    ANIMCMD_FRAME(8, 5),
    ANIMCMD_END
};

static const union AnimCmd *const sRegionMapNameAnimTable[] = {
    sRegionMapNameJohto,
    sRegionMapNameKanto,
    sRegionMapNameSevii
};

static const struct SpriteTemplate sRegionMapNameSpriteTemplate = {
    0,
    0,
    &sRegionMapNameOam,
    sRegionMapNameAnimTable,
    NULL,
    gDummySpriteAffineAnimTable,
    SpriteCallbackDummy
};

// Event islands that don't appear on map. (Southern Island does)
static const u8 sMapSecIdsOffMap[] =
{
    MAPSEC_BIRTH_ISLAND,
    MAPSEC_FARAWAY_ISLAND,
    MAPSEC_NAVEL_ROCK
};

// Fly map data tables removed with CrystalDust's Fly map (D33).

static const u8 whiteTextColor[] = {
	0x00, 0x01, 0x02
};

static const struct WindowCoords blankWindowCoords = {
    .x1 = 0,
    .y1 = 0,
    .x2 = 0,
    .y2 = 0,
};

static const struct WindowCoords windowCoords[] = {
    {
        .x1 = 24,
        .y1 = 16,
        .x2 = 144,
        .y2 = 32,
    },
    {
        .x1 = 24,
        .y1 = 32,
        .x2 = 144,
        .y2 = 48,
    },
};

static const bool8 sRegionMapPermissions[3][4] = {
    {FALSE, TRUE , TRUE , FALSE},
    {TRUE,  FALSE, FALSE, FALSE},
    {TRUE,  FALSE, FALSE, TRUE }
};

// .text

void CDMap_InitRegionMap(struct CDRegionMap *regionMap, u8 mode, s8 xOffset, s8 yOffset)
{
    CDMap_InitRegionMapData(regionMap, NULL, mode, xOffset, yOffset);
    while (CDMap_LoadRegionMapGfx(gRegionMap->bgManaged));
    while (CDMap_LoadRegionMapGfx_Pt2());
}

void CDMap_InitRegionMapData(struct CDRegionMap *regionMap, const struct BgTemplate *template, u8 mapMode, s8 xOffset, s8 yOffset)
{
    u32 i;

    gRegionMap = regionMap;
    gRegionMap->initStep = 0;
    gRegionMap->xOffset = xOffset;
    gRegionMap->yOffset = yOffset;
    gRegionMap->currentRegion = CDMap_GetCurrentRegion();
    gRegionMap->mapMode = mapMode;
    gRegionMap->inputCallback = CDMap_ProcessRegionMapInput_Full;

    for (i = 0; i < 4; i++)
    {
        gRegionMap->permissions[i] = sRegionMapPermissions[gRegionMap->mapMode][i];
    }

    //TODO: Make conditional on visiting Kanto once
    gRegionMap->permissions[MAPPERM_SWITCH] = FALSE;

    for (i = 0; i < sizeof(gRegionMap->spriteIds); i++)
    {
        gRegionMap->spriteIds[i] = 0xFF;
    }

    if (template != NULL)
    {
        gRegionMap->bgNum = template->bg;
        gRegionMap->charBaseIdx = template->charBaseIndex;
        gRegionMap->mapBaseIdx = template->mapBaseIndex;
        gRegionMap->bgManaged = TRUE;
    }
    else
    {
        gRegionMap->bgNum = 2;
        gRegionMap->charBaseIdx = 2;
        gRegionMap->mapBaseIdx = 29;
        gRegionMap->bgManaged = FALSE;
    }
}

void CDMap_ShowRegionMapForPokedexAreaScreen(struct CDRegionMap *regionMap)
{
    gRegionMap = regionMap;
    CDMap_InitMapBasedOnPlayerLocation();
    gRegionMap->playerIconSpritePosX = gRegionMap->cursorPosX;
    gRegionMap->playerIconSpritePosY = gRegionMap->cursorPosY;
}

bool8 CDMap_ChangeDecompressedRegionMapGfx(u16* ptr, bool8* permissions)
{
    u8 x, y;

    if (permissions[MAPPERM_SWITCH])
    {
        ptr[25 + 17 * 32] = 0x90F4;
    }
    else if (!permissions[MAPPERM_CLOSE])
    {
        for (y = 16; y < 19; y++)
            for (x = 24; x < 27; x++)
                ptr[x + y * 32] = 0x9096;
    }

    return TRUE;
}

bool8 CDMap_LoadRegionMapGfx(bool8 shouldBuffer)
{
    const u8 *regionTilemap;
    u32 i;
    u16 *ptr;

    switch (gRegionMap->initStep)
    {
        case 0:
            if (shouldBuffer)
            {
                DecompressAndCopyTileDataToVram(gRegionMap->bgNum, sRegionMapTileset, 0, 0, 0);
            }
            else
            {
                LZ77UnCompVram(sRegionMapTileset, (u16 *)BG_CHAR_ADDR(gRegionMap->charBaseIdx));
            }
            break;
        case 1:
            /*regionTilemap = CDMap_GetRegionMapTilemap(gRegionMap->currentRegion);
            if (gRegionMap->bgManaged)
            {
                if (!FreeTempTileDataBuffersIfPossible())
                {
                    DecompressAndCopyTileDataToVram(gRegionMap->bgNum, regionTilemap, 0, 0, 1);
                }
            }
            else
            {
                LZ77UnCompVram(regionTilemap, (u16 *)BG_SCREEN_ADDR(28));
            }*/
            {
                u32 size;
                u16 *ptr = malloc_and_decompress(CDMap_GetRegionMapTilemap(gRegionMap->currentRegion), &size);
                CDMap_ChangeDecompressedRegionMapGfx(ptr, gRegionMap->permissions);

                if (shouldBuffer)
                {
                    if (!FreeTempTileDataBuffersIfPossible())
                    {
                        copy_decompressed_tile_data_to_vram(gRegionMap->bgNum, ptr, size, gRegionMap->xOffset, 1);
                    }
                }
                else
                {
                    CpuFastCopy(ptr, (u16 *)BG_SCREEN_ADDR(gRegionMap->mapBaseIdx) + gRegionMap->xOffset, size);
                }

                FREE_AND_SET_NULL(ptr);
            }
            break;
        case 2:
            if (!FreeTempTileDataBuffersIfPossible())
            {
                LoadPalette(sRegionMapPal, 0x70, sizeof(sRegionMapPal));
                LoadPalette(sRegionMapTownNames_Pal, 0xE0, sizeof(sRegionMapTownNames_Pal));
            }
            gRegionMap->initStep++;
        default:
            return FALSE;
    }
    gRegionMap->initStep++;
    return TRUE;
}

bool8 CDMap_LoadRegionMapGfx_Pt2(void)
{
    const struct WindowTemplate layerTemplates[] = {
        {0, 3, 2, 15, 2, 14, 1},
        {0, 3, 4, 15, 2, 14, 31}
    };

    struct WindowTemplate window;

    switch (gRegionMap->initStep)
    {
        case 3:
            SetGpuReg(REG_OFFSET_BLDCNT, BLDCNT_EFFECT_NONE);
            SetGpuReg(REG_OFFSET_BLDY, 6);
            SetGpuReg(REG_OFFSET_WININ, WININ_WIN0_BG0 |
                                        WININ_WIN0_BG1 |
                                        WININ_WIN0_BG2 |
                                        WININ_WIN0_BG3 |
                                        WININ_WIN0_OBJ |
                                        WININ_WIN0_CLR |
                                        WININ_WIN1_BG0 |
                                        WININ_WIN1_BG1 |
                                        WININ_WIN1_BG2 |
                                        WININ_WIN1_BG3 |
                                        WININ_WIN1_OBJ |
                                        WININ_WIN1_CLR);
            SetGpuReg(REG_OFFSET_WINOUT, WINOUT_WIN01_BG0 |
                                         WINOUT_WIN01_BG1 |
                                         WINOUT_WIN01_BG2 |
                                         WINOUT_WIN01_BG3 |
                                         WINOUT_WIN01_OBJ);
            CDMap_SetupShadowBoxes(0, &windowCoords[0]);
            CDMap_SetupShadowBoxes(1, &windowCoords[1]);

            window = layerTemplates[0];
            window.tilemapLeft += gRegionMap->xOffset;
            window.tilemapTop +=  gRegionMap->yOffset;
            gRegionMap->primaryWindowId = AddWindow(&window);

            window = layerTemplates[1];
            window.tilemapLeft += gRegionMap->xOffset;
            window.tilemapTop += gRegionMap->yOffset;
            gRegionMap->secondaryWindowId = AddWindow(&window);
        case 4:
            LZ77UnCompWram(sRegionMapCursorGfxLZ, gRegionMap->cursorImage);
            break;
        case 5:
            CDMap_InitMapBasedOnPlayerLocation();
            CDMap_SetShadowBoxState(0, FALSE);

            if (gRegionMap->secondaryMapSecId != MAPSEC_NONE)
                CDMap_SetShadowBoxState(1, FALSE);

            if(gMapHeader.regionMapSectionId == MAPSEC_FAST_SHIP)
            {
                gRegionMap->playerIconSpritePosX = 21;
                gRegionMap->playerIconSpritePosY = 12;
            }
            else
            {
                gRegionMap->playerIconSpritePosX = gRegionMap->cursorPosX;
                gRegionMap->playerIconSpritePosY = gRegionMap->cursorPosY;
            }
            gRegionMap->primaryMapSecId = CDMap_CorrectSpecialMapSecId_Internal(gRegionMap->primaryMapSecId);
            gRegionMap->primaryMapSecStatus = CDMap_GetMapsecType(gRegionMap->primaryMapSecId);
            gRegionMap->secondaryMapSecId = CDMap_CorrectSpecialMapSecId_Internal(gRegionMap->secondaryMapSecId);
            gRegionMap->secondaryMapSecStatus = CDMap_GetMapsecType(gRegionMap->secondaryMapSecId);

            ScheduleBgCopyTilemapToVram(0);
            break;
        case 6:
            CDMap_GetPositionOfCursorWithinMapSec();
            SetGpuReg(REG_OFFSET_BLDCNT, BLDCNT_TGT1_BG2 | BLDCNT_TGT1_OBJ | BLDCNT_EFFECT_DARKEN);

            CDMap_LoadPrimaryLayerMapSec();
            CDMap_LoadSecondaryLayerMapSec();

            gRegionMap->cursorMovementFrameCounter = 0;
            gRegionMap->blinkPlayerIcon = FALSE;
            gRegionMap->initStep++;
        default:
            return FALSE;
    }
    gRegionMap->initStep++;
    return TRUE;
}

const u16 *CDMap_GetRegionMapPalette(void)
{
    return sRegionMapPal;
}

const u32 *CDMap_GetRegionMapTileset(void)
{
    return sRegionMapTileset;
}

const u32 *CDMap_GetRegionMapTilemap(u8 region)
{
    const u32 *const tilemaps[] = {
        sRegionMapJohtoTilemap,
        sRegionMapKantoTilemap,
        sRegionMapJohtoTilemap,
        sRegionMapJohtoTilemap,
        sRegionMapJohtoTilemap,
    };

    return tilemaps[region];
}

void CDMap_BlendRegionMap(u16 color, u32 coeff)
{
    BlendPalettes(0x380, coeff, color);
    CpuCopy16(gPlttBufferFaded + 0x70, gPlttBufferUnfaded + 0x70, 0x60);
}

void CDMap_FreeRegionMapResources(void)
{
    u32 i;

    if (gRegionMap->spriteIds[0] != 0xFF)
    {
        DestroySprite(&gSprites[gRegionMap->spriteIds[0]]);
        FreeSpriteTilesByTag(gRegionMap->cursorTileTag);
        FreeSpritePaletteByTag(gRegionMap->cursorPaletteTag);
    }
    if (gRegionMap->spriteIds[1] != 0xFF)
    {
        DestroySprite(&gSprites[gRegionMap->spriteIds[1]]);
        FreeSpriteTilesByTag(gRegionMap->playerIconTileTag);
        FreeSpritePaletteByTag(gRegionMap->playerIconPaletteTag);
    }
    if (gRegionMap->spriteIds[2] != 0xFF)
    {
        DestroySprite(&gSprites[gRegionMap->spriteIds[2]]);
        FreeSpriteTilesByTag(gRegionMap->regionNameCurveTileTag);
    }
    if (gRegionMap->spriteIds[3] != 0xFF)
    {
        DestroySprite(&gSprites[gRegionMap->spriteIds[3]]);
        FreeSpriteTilesByTag(gRegionMap->regionNameMainTileTag);
    }

    for (i = 4; i < sizeof(gRegionMap->spriteIds); i++)
    {
        if (gRegionMap->spriteIds[i] != 0xFF)
        {
            DestroySprite(&gSprites[gRegionMap->spriteIds[i]]);
        }
    }

    FreeSpriteTilesByTag(gRegionMap->dotsTileTag);
    FreeSpritePaletteByTag(gRegionMap->miscSpritesPaletteTag);

    FillWindowPixelBuffer(gRegionMap->primaryWindowId, 0);
    ClearWindowTilemap(gRegionMap->primaryWindowId);
    CopyWindowToVram(gRegionMap->primaryWindowId, 2);
    RemoveWindow(gRegionMap->primaryWindowId);

    FillWindowPixelBuffer(gRegionMap->secondaryWindowId, 0);
    ClearWindowTilemap(gRegionMap->secondaryWindowId);
    CopyWindowToVram(gRegionMap->secondaryWindowId, 2);
    RemoveWindow(gRegionMap->secondaryWindowId);

    ScheduleBgCopyTilemapToVram(0);

    SetGpuReg(REG_OFFSET_BLDCNT, 0);
    SetGpuReg(REG_OFFSET_BLDY, 0);
    SetGpuReg(REG_OFFSET_WININ, 0);
    SetGpuReg(REG_OFFSET_WINOUT, 0);
    ClearGpuRegBits(REG_OFFSET_DISPCNT, DISPCNT_WIN0_ON | DISPCNT_WIN1_ON);

    // Crystal Expansion (D112): this used to Free(gRegionMap). It must not --
    // gRegionMap is whatever pointer the caller handed to CDMap_InitRegionMapData,
    // and only the caller knows whether it owns that memory. The Pokegear passes
    // its own AllocZeroed block and frees it again in FreePokegearData (double
    // free); src/field_region_map.c passes a struct *inside* its own allocation,
    // so freeing it handed the allocator a pointer eight bytes past a block
    // header and corrupted the heap. Ownership stays with the caller; this
    // function only releases the sprites, windows and GPU state it set up.
    gRegionMap = NULL;
}

u8 CDMap_DoRegionMapInputCallback(void)
{
    return gRegionMap->inputCallback();
}

static u8 CDMap_ProcessRegionMapInput_Full(void)
{
    u8 input;

    input = MAP_INPUT_NONE;
    gRegionMap->cursorDeltaX = 0;
    gRegionMap->cursorDeltaY = 0;

    if (JOY_HELD(DPAD_UP) && gRegionMap->cursorPosY > 0)
    {
        gRegionMap->cursorDeltaY = -1;
        input = MAP_INPUT_MOVE_START;
    }

    if (JOY_HELD(DPAD_DOWN) && gRegionMap->cursorPosY < MAP_HEIGHT - 1)
    {
        gRegionMap->cursorDeltaY = +1;
        input = MAP_INPUT_MOVE_START;
    }

    if (JOY_HELD(DPAD_LEFT) && gRegionMap->cursorPosX > 0)
    {
        gRegionMap->cursorDeltaX = -1;
        input = MAP_INPUT_MOVE_START;
    }

    if (JOY_HELD(DPAD_RIGHT) && gRegionMap->cursorPosX < MAP_WIDTH - 1)
    {
        gRegionMap->cursorDeltaX = +1;
        input = MAP_INPUT_MOVE_START;
    }

    if (JOY_NEW(A_BUTTON))
    {
        input = MAP_INPUT_A_BUTTON;
        if (gRegionMap->cursorPosX == CORNER_BUTTON_X && gRegionMap->cursorPosY == CORNER_BUTTON_Y)
        {
            if (gRegionMap->permissions[MAPPERM_CLOSE])
            {
                PlaySE(SE_M_HYPER_BEAM2);
                input = MAP_INPUT_CANCEL;
            }
            else if (gRegionMap->permissions[MAPPERM_SWITCH])
            {
                PlaySE(SE_M_HYPER_BEAM2);
                input = MAP_INPUT_SWITCH;
            }
        }
    }
    else if (JOY_NEW(B_BUTTON))
    {
        input = MAP_INPUT_CANCEL;
    }

    if (input == MAP_INPUT_MOVE_START)
    {
        gRegionMap->cursorMovementFrameCounter = 4;
        gRegionMap->inputCallback = CDMap_MoveRegionMapCursor_Full;
    }
    return input;
}

static void CDMap_LoadMapLayersFromPosition(u16 x, u16 y)
{
    u8 mapSecId = CDMap_GetMapSecIdAt(x, y, gRegionMap->currentRegion, FALSE);

    gRegionMap->enteredSecondary = FALSE;
    gRegionMap->primaryMapSecStatus = CDMap_GetMapsecType(mapSecId);

    if (mapSecId != gRegionMap->primaryMapSecId)
    {
        gRegionMap->primaryMapSecId = mapSecId;
        CDMap_LoadPrimaryLayerMapSec();
    }

    mapSecId = CDMap_GetMapSecIdAt(x, y, gRegionMap->currentRegion, TRUE);
    gRegionMap->secondaryMapSecStatus = CDMap_GetMapsecType(mapSecId);
    if (mapSecId != gRegionMap->secondaryMapSecId)
    {
        gRegionMap->secondaryMapSecId = mapSecId;
        gRegionMap->enteredSecondary = TRUE;
        CDMap_LoadSecondaryLayerMapSec();
    }

    ScheduleBgCopyTilemapToVram(0);
    CDMap_SetupShadowBoxes(1, &windowCoords[1]);
}

static u8 CDMap_MoveRegionMapCursor_Full(void)
{
    u8 inputEvent;

    if (gRegionMap->cursorMovementFrameCounter != 0)
        return MAP_INPUT_MOVE_CONT;

    if (gRegionMap->cursorDeltaX > 0)
    {
        gRegionMap->cursorPosX++;
    }
    if (gRegionMap->cursorDeltaX < 0)
    {
        gRegionMap->cursorPosX--;
    }
    if (gRegionMap->cursorDeltaY > 0)
    {
        gRegionMap->cursorPosY++;
    }
    if (gRegionMap->cursorDeltaY < 0)
    {
        gRegionMap->cursorPosY--;
    }

    CDMap_LoadMapLayersFromPosition(gRegionMap->cursorPosX, gRegionMap->cursorPosY);

    if (gRegionMap->primaryMapSecStatus != MAPSECTYPE_NONE)
    {
        CDMap_GetPositionOfCursorWithinMapSec();
    }

    gRegionMap->inputCallback = CDMap_ProcessRegionMapInput_Full;
    return MAP_INPUT_MOVE_END;
}

static void CDMap_LoadPrimaryLayerMapSec(void)
{
    ClearWindowTilemap(gRegionMap->primaryWindowId);
	FillWindowPixelBuffer(gRegionMap->primaryWindowId, 0);

	if (gRegionMap->primaryMapSecId != MAPSEC_NONE)
    {
		GetMapName(gRegionMap->primaryMapSecName, gRegionMap->primaryMapSecId, 0);
		AddTextPrinterParameterized3(gRegionMap->primaryWindowId, 2, 2, 2, whiteTextColor, 0, gRegionMap->primaryMapSecName);
		PutWindowTilemap(gRegionMap->primaryWindowId);
		CopyWindowToVram(gRegionMap->primaryWindowId, 3);
		CDMap_SetupShadowBoxes(0, &windowCoords[0]);
    }
	else
    {
		CDMap_SetupShadowBoxes(0, &blankWindowCoords);
	}
}

static void CDMap_LoadSecondaryLayerMapSec(void)
{
    static const u8 mapNamePalDataPointerTable[][3] = {
        {0x00, 0x07, 0x02}, // green (visited)
        {0x00, 0x0A, 0x02}  // red (not yet visited)
    };

    CDMap_SetShadowBoxState(1, TRUE);
    ClearWindowTilemap(gRegionMap->secondaryWindowId);

	if (gRegionMap->secondaryMapSecId != MAPSEC_NONE)
    {
        CDMap_SetShadowBoxState(1, FALSE);
	    FillWindowPixelBuffer(gRegionMap->secondaryWindowId, 0);
		GetMapName(gRegionMap->secondaryMapSecName, gRegionMap->secondaryMapSecId, 0);
		AddTextPrinterParameterized3(gRegionMap->secondaryWindowId, 2, 12, 2, mapNamePalDataPointerTable[CDMap_GetMapSecStatusByLayer(1) - 2], 0, gRegionMap->secondaryMapSecName);
		PutWindowTilemap(gRegionMap->secondaryWindowId);
		CopyWindowToVram(gRegionMap->secondaryWindowId, 3);
	}
}

static void CDMap_SetupShadowBoxes(u8 layerNum, const struct WindowCoords *coords)
{
    static const u8 windowIORegs[2][2] = {
        { REG_OFFSET_WIN0V, REG_OFFSET_WIN0H },
        { REG_OFFSET_WIN1V, REG_OFFSET_WIN1H }
    };

	SetGpuReg(windowIORegs[layerNum][0], WIN_RANGE(coords->y1, coords->y2));
	SetGpuReg(windowIORegs[layerNum][1], WIN_RANGE(coords->x1 + gRegionMap->xOffset * 8, coords->x2 + gRegionMap->xOffset * 8));
}

static void CDMap_SetShadowBoxState(u8 offset, bool8 hide)
{
    static const u16 windowBits[2] = {
        0x2000, 0x4000
    };

	if (!hide)
    {
		SetGpuRegBits(REG_OFFSET_DISPCNT, windowBits[offset]);
	}
	else
    {
		ClearGpuRegBits(REG_OFFSET_DISPCNT, windowBits[offset]);
	}
}

static u8 CDMap_GetMapSecStatusByLayer(u8 layer)
{
	if (layer == 1)
        return gRegionMap->secondaryMapSecStatus;
	else
        return gRegionMap->primaryMapSecStatus;
}

void CDMap_SetRegionMapDataForZoom(void)
{

}

bool8 CDMap_UpdateRegionMapZoom(void)
{
    return FALSE;
}

void CDMap_UpdateRegionMapVideoRegs(void)
{

}

void CDMap_PokedexAreaScreen_UpdateRegionMapVariablesAndVideoRegs(s16 x, s16 y)
{
}

static u8 CDMap_GetMapSecIdAt(s16 x, s16 y, u8 region, bool8 secondary)
{
    static const u8 *const layouts[][2] = {
        {sMapSectionLayout_JohtoPrimary, sMapSectionLayout_JohtoSecondary},
        {sMapSectionLayout_KantoPrimary, sMapSectionLayout_KantoSecondary},
        {sMapSectionLayout_JohtoPrimary, sMapSectionLayout_JohtoSecondary},
        {sMapSectionLayout_JohtoPrimary, sMapSectionLayout_JohtoSecondary},
        {sMapSectionLayout_JohtoPrimary, sMapSectionLayout_JohtoSecondary}
    };

    if (y < 0 || y >= MAP_HEIGHT || x < 0 || x >= MAP_WIDTH)
    {
        return MAPSEC_NONE;
    }
    return layouts[region][secondary][x + y * MAP_WIDTH];
}

static void CDMap_InitMapBasedOnPlayerLocation(void)
{
    // map group, map num, x, y
    static const u8 cursorPosOverrides[][4] = {
        {MAP_GROUP(MAP_ROUTE29_GATEHOUSE), MAP_NUM(MAP_ROUTE29_GATEHOUSE), 18, 10},
        {MAP_GROUP(MAP_ROUTE31_GATEHOUSE), MAP_NUM(MAP_ROUTE31_GATEHOUSE), 12, 5},
        {MAP_GROUP(MAP_ROUTE32_GATEHOUSE), MAP_NUM(MAP_ROUTE32_GATEHOUSE), 11, 6},
        {MAP_GROUP(MAP_ROUTE34_ILEX_EAST_GATEHOUSE), MAP_NUM(MAP_ROUTE34_ILEX_EAST_GATEHOUSE), 8, 13},
        {MAP_GROUP(MAP_ROUTE34_ILEX_NORTH_GATEHOUSE), MAP_NUM(MAP_ROUTE34_ILEX_NORTH_GATEHOUSE), 7, 12},
        {MAP_GROUP(MAP_ROUTE35_GOLDENROD_GATEHOUSE), MAP_NUM(MAP_ROUTE35_GOLDENROD_GATEHOUSE), 7, 8},
        {MAP_GROUP(MAP_ROUTE35_NATIONAL_PARK_GATEHOUSE), MAP_NUM(MAP_ROUTE35_NATIONAL_PARK_GATEHOUSE), 7, 6},
        {MAP_GROUP(MAP_ROUTE36_RUINS_OF_ALPH_GATEHOUSE), MAP_NUM(MAP_ROUTE36_RUINS_OF_ALPH_GATEHOUSE), 10, 5},
        {MAP_GROUP(MAP_ROUTE36_NATIONAL_PARK_GATEHOUSE), MAP_NUM(MAP_ROUTE36_NATIONAL_PARK_GATEHOUSE), 8, 5},
        {MAP_GROUP(MAP_ROUTE38_GATEHOUSE), MAP_NUM(MAP_ROUTE38_GATEHOUSE), 8, 3},
        {MAP_GROUP(MAP_ROUTE40_GATEHOUSE), MAP_NUM(MAP_ROUTE40_GATEHOUSE), 3, 6},
        {MAP_GROUP(MAP_ROUTE42_GATEHOUSE), MAP_NUM(MAP_ROUTE42_GATEHOUSE), 10, 3},
        {MAP_GROUP(MAP_VICTORY_ROAD_GATEHOUSE), MAP_NUM(MAP_VICTORY_ROAD_GATEHOUSE), 1, 7},
        {MAP_GROUP(MAP_ROUTE5_GATEHOUSE), MAP_NUM(MAP_ROUTE5_GATEHOUSE), 14, 5},
        {MAP_GROUP(MAP_ROUTE6_GATEHOUSE), MAP_NUM(MAP_ROUTE6_GATEHOUSE), 14, 7},
        {MAP_GROUP(MAP_ROUTE7_GATEHOUSE), MAP_NUM(MAP_ROUTE7_GATEHOUSE), 13, 6},
        {MAP_GROUP(MAP_ROUTE8_GATEHOUSE), MAP_NUM(MAP_ROUTE8_GATEHOUSE), 15, 6},
        {MAP_GROUP(MAP_ROUTE19_GATEHOUSE), MAP_NUM(MAP_ROUTE19_GATEHOUSE), 12, 13},
        {MAP_GROUP(MAP_UNDEFINED), MAP_NUM(MAP_UNDEFINED), 0, 0},
    };

    int i;

    if (gMapHeader.regionMapSectionId == MAPSEC_UNDERGROUND_PATH)
    {
        gRegionMap->cursorPosX = 14;
        gRegionMap->cursorPosY = 7;
        if(gSaveBlock1Ptr->location.mapGroup == MAP_GROUP(MAP_ROUTE5_UNDERGROUND_PATH_ENTRANCE))
        {
            gRegionMap->cursorPosY = 5;
        }
    }
    else
    {
        for (i = 0; cursorPosOverrides[i][0] != MAP_GROUP(MAP_UNDEFINED); i++)
        {
            if (gSaveBlock1Ptr->location.mapGroup == cursorPosOverrides[i][0] &&
                gSaveBlock1Ptr->location.mapNum == cursorPosOverrides[i][1])
            {
                gRegionMap->cursorPosX = cursorPosOverrides[i][2];
                gRegionMap->cursorPosY = cursorPosOverrides[i][3];
                break;
            }
        }

        if (cursorPosOverrides[i][0] == MAP_GROUP(MAP_UNDEFINED))
        {
            CDMap_InitMapBasedOnPlayerLocation_();
        }
    }

    gRegionMap->primaryMapSecId = CDMap_GetMapSecIdAt(gRegionMap->cursorPosX, gRegionMap->cursorPosY, gRegionMap->currentRegion, FALSE);
    gRegionMap->secondaryMapSecId = CDMap_GetMapSecIdAt(gRegionMap->cursorPosX, gRegionMap->cursorPosY, gRegionMap->currentRegion, TRUE);
}

static bool32 CDMap_IsOverriddenRegionMapLocation(void)
{
    switch(gMapHeader.regionMapSectionId)
    {
        case MAPSEC_MT_MORTAR:
        case MAPSEC_ICE_PATH:
        case MAPSEC_KANTO_VICTORY_ROAD:
        case MAPSEC_ROCK_TUNNEL:
        case MAPSEC_MT_MOON:
            return TRUE;
        default:
            return FALSE;
    }
}

static void CDMap_InitMapBasedOnPlayerLocation_(void)
{
    const struct MapHeader *mapHeader;
    u16 mapWidth;
    u16 mapHeight;
    u16 x;
    u16 y;
    u16 dimensionScale;
    u16 xOnMap;
    struct WarpData *warp;

    switch (GetMapTypeByGroupAndId(gSaveBlock1Ptr->location.mapGroup, gSaveBlock1Ptr->location.mapNum))
    {
        default:
        case MAP_TYPE_TOWN:
        case MAP_TYPE_CITY:
        case MAP_TYPE_ROUTE:
        case MAP_TYPE_UNDERWATER:
        case MAP_TYPE_OCEAN_ROUTE:
            gRegionMap->primaryMapSecId = gMapHeader.regionMapSectionId;
            gRegionMap->playerIsInCave = FALSE;
            mapWidth = gMapHeader.mapLayout->width;
            mapHeight = gMapHeader.mapLayout->height;
            x = gSaveBlock1Ptr->pos.x;
            y = gSaveBlock1Ptr->pos.y;
            if (gRegionMap->primaryMapSecId == MAPSEC_UNDERWATER_SEAFLOOR_CAVERN || gRegionMap->primaryMapSecId == MAPSEC_UNDERWATER_MARINE_CAVE)
            {
                gRegionMap->playerIsInCave = TRUE;
            }
            break;
        case MAP_TYPE_UNDERGROUND:
        case MAP_TYPE_UNKNOWN:
            if (gMapHeader.allowEscaping && !CDMap_IsOverriddenRegionMapLocation())
            {
                mapHeader = Overworld_GetMapHeaderByGroupAndId(gSaveBlock1Ptr->escapeWarp.mapGroup, gSaveBlock1Ptr->escapeWarp.mapNum);
                gRegionMap->primaryMapSecId = mapHeader->regionMapSectionId;
                gRegionMap->playerIsInCave = TRUE;
                mapWidth = mapHeader->mapLayout->width;
                mapHeight = mapHeader->mapLayout->height;
                x = gSaveBlock1Ptr->escapeWarp.x;
                y = gSaveBlock1Ptr->escapeWarp.y;
            }
            else
            {
                gRegionMap->primaryMapSecId = gMapHeader.regionMapSectionId;
                gRegionMap->playerIsInCave = TRUE;
                mapWidth = 1;
                mapHeight = 1;
                x = 1;
                y = 1;
            }
            break;
        case MAP_TYPE_SECRET_BASE:
            mapHeader = Overworld_GetMapHeaderByGroupAndId((u16)gSaveBlock1Ptr->dynamicWarp.mapGroup, (u16)gSaveBlock1Ptr->dynamicWarp.mapNum);
            gRegionMap->primaryMapSecId = mapHeader->regionMapSectionId;
            gRegionMap->playerIsInCave = TRUE;
            mapWidth = mapHeader->mapLayout->width;
            mapHeight = mapHeader->mapLayout->height;
            x = gSaveBlock1Ptr->dynamicWarp.x;
            y = gSaveBlock1Ptr->dynamicWarp.y;
            break;
        case MAP_TYPE_INDOOR:
            gRegionMap->primaryMapSecId = gMapHeader.regionMapSectionId;
            if (gRegionMap->primaryMapSecId != MAPSEC_DYNAMIC)
            {
                warp = &gSaveBlock1Ptr->escapeWarp;
                mapHeader = Overworld_GetMapHeaderByGroupAndId(warp->mapGroup, warp->mapNum);
            }
            else
            {
                warp = &gSaveBlock1Ptr->dynamicWarp;
                mapHeader = Overworld_GetMapHeaderByGroupAndId(warp->mapGroup, warp->mapNum);
                gRegionMap->primaryMapSecId = mapHeader->regionMapSectionId;
            }

            if (CDMap_IsPlayerInAquaHideout(gRegionMap->primaryMapSecId))
                gRegionMap->playerIsInCave = TRUE;
            else
                gRegionMap->playerIsInCave = FALSE;

            mapWidth = mapHeader->mapLayout->width;
            mapHeight = mapHeader->mapLayout->height;
            x = warp->x;
            y = warp->y;
            break;
    }

    xOnMap = x;

    dimensionScale = mapWidth / gRegionMapEntries[gRegionMap->primaryMapSecId].width;
    if (dimensionScale == 0)
    {
        dimensionScale = 1;
    }
    x /= dimensionScale;
    if (x >= gRegionMapEntries[gRegionMap->primaryMapSecId].width)
    {
        x = gRegionMapEntries[gRegionMap->primaryMapSecId].width - 1;
    }

    dimensionScale = mapHeight / gRegionMapEntries[gRegionMap->primaryMapSecId].height;
    if (dimensionScale == 0)
    {
        dimensionScale = 1;
    }
    y /= dimensionScale;
    if (y >= gRegionMapEntries[gRegionMap->primaryMapSecId].height)
    {
        y = gRegionMapEntries[gRegionMap->primaryMapSecId].height - 1;
    }

    switch (gRegionMap->primaryMapSecId)
    {
        case MAPSEC_ROUTE_33:
            x = 0;
            if (gSaveBlock1Ptr->pos.x > 8)
                x = 1;
            break;
    }
    if(gMapHeader.regionMapSectionId == MAPSEC_FAST_SHIP)
    {   // init cursor on New Bark Town if on Fast Ship
        gRegionMap->cursorPosX = 21;
        gRegionMap->cursorPosY = 10;
    }
    else
    {
        gRegionMap->cursorPosX = gRegionMapEntries[gRegionMap->primaryMapSecId].x + x;
        gRegionMap->cursorPosY = gRegionMapEntries[gRegionMap->primaryMapSecId].y + y;
    }
}

static void CDMap_RegionMap_InitializeStateBasedOnSSTidalLocation(void)
{
    u16 y;
    u16 x;
    s8 mapGroup;
    s8 mapNum;
    u16 dimensionScale;
    s16 xOnMap;
    s16 yOnMap;
    const struct MapHeader *mapHeader;

    y = 0;
    x = 0;
    switch (GetSSTidalLocation(&mapGroup, &mapNum, &xOnMap, &yOnMap))
    {
        case SS_TIDAL_LOCATION_SLATEPORT:
            gRegionMap->primaryMapSecId = MAPSEC_ECRUTEAK_CITY;
            break;
        case SS_TIDAL_LOCATION_LILYCOVE:
            gRegionMap->primaryMapSecId = MAPSEC_BLACKTHORN_CITY;
            break;
        case SS_TIDAL_LOCATION_ROUTE124:
            gRegionMap->primaryMapSecId = MAPSEC_ROUTE_124;
            break;
        case SS_TIDAL_LOCATION_ROUTE131:
            gRegionMap->primaryMapSecId = MAPSEC_ROUTE_131;
            break;
        default:
        case SS_TIDAL_LOCATION_CURRENTS:
            mapHeader = Overworld_GetMapHeaderByGroupAndId(mapGroup, mapNum);

            gRegionMap->primaryMapSecId = mapHeader->regionMapSectionId;
            dimensionScale = mapHeader->mapLayout->width / gRegionMapEntries[gRegionMap->primaryMapSecId].width;
            if (dimensionScale == 0)
                dimensionScale = 1;
            x = xOnMap / dimensionScale;
            if (x >= gRegionMapEntries[gRegionMap->primaryMapSecId].width)
                x = gRegionMapEntries[gRegionMap->primaryMapSecId].width - 1;

            dimensionScale = mapHeader->mapLayout->height / gRegionMapEntries[gRegionMap->primaryMapSecId].height;
            if (dimensionScale == 0)
                dimensionScale = 1;
            y = yOnMap / dimensionScale;
            if (y >= gRegionMapEntries[gRegionMap->primaryMapSecId].height)
                y = gRegionMapEntries[gRegionMap->primaryMapSecId].height - 1;
            break;
    }
    gRegionMap->playerIsInCave = FALSE;
    gRegionMap->cursorPosX = gRegionMapEntries[gRegionMap->primaryMapSecId].x + x;
    gRegionMap->cursorPosY = gRegionMapEntries[gRegionMap->primaryMapSecId].y + y;
}

static u8 CDMap_GetMapsecType(u16 mapSecId)
{
    u8 mapSecStatus = MAPSECTYPE_NONE;

    // ensure no landmark sound on any map besides fly map
    if (mapSecId != MAPSEC_NONE)
    {
        u16 flag = sMapSecFlags[mapSecId];
        mapSecStatus = MAPSECTYPE_ROUTE;

        if (flag != 0)
        {
            mapSecStatus = MAPSECTYPE_NOT_VISITED;
            if (FlagGet(flag))
            {
                mapSecStatus = MAPSECTYPE_VISITED;
            }
        }
    }

    return mapSecStatus;
}

bool8 CDMap_MapsecWasVisited(u16 mapSecId)
{
    return CDMap_GetMapsecType(mapSecId) == MAPSECTYPE_VISITED;
}

u16 CDMap_GetRegionMapSectionIdAt(u16 x, u16 y, u8 region)
{
    return CDMap_GetMapSecIdAt(x, y, region, FALSE);
}

static u16 CDMap_CorrectSpecialMapSecId_Internal(u16 mapSecId)
{
    u32 i;

    for (i = 0; i < ARRAY_COUNT(sMarineCaveMapSecIds); i++)
    {
        if (sMarineCaveMapSecIds[i] == mapSecId)
        {
            return CDMap_GetTerraOrMarineCaveMapSecId();
        }
    }
    for (i = 0; sRegionMap_SpecialPlaceLocations[i][0] != MAPSEC_NONE; i++)
    {
        if (sRegionMap_SpecialPlaceLocations[i][0] == mapSecId)
        {
            return sRegionMap_SpecialPlaceLocations[i][1];
        }
    }
    return mapSecId;
}

static u16 CDMap_GetTerraOrMarineCaveMapSecId(void)
{
    s16 idx;

    idx = VarGet(VAR_ABNORMAL_WEATHER_LOCATION) - 1;

    if (idx < 0 || idx > ABNORMAL_WEATHER_LOCATIONS - 1)
        idx = 0;

    return sTerraOrMarineCaveMapSecIds[idx];
}

static void CDMap_GetMarineCaveCoords(u16 *x, u16 *y)
{
    u16 idx;

    idx = VarGet(VAR_ABNORMAL_WEATHER_LOCATION);
    if (idx < MARINE_CAVE_LOCATIONS_START || idx > ABNORMAL_WEATHER_LOCATIONS)
    {
        idx = MARINE_CAVE_LOCATIONS_START;
    }
    idx -= MARINE_CAVE_LOCATIONS_START;

    *x = sMarineCaveLocationCoords[idx].x;
    *y = sMarineCaveLocationCoords[idx].y;
}

// Probably meant to be an "IsPlayerInIndoorDungeon" function, but in practice it only has the one mapsec
// Additionally, because the mapsec doesnt exist in Emerald, this function always returns FALSE
static bool32 CDMap_IsPlayerInAquaHideout(u8 mapSecId)
{
    u32 i;

    for (i = 0; i < ARRAY_COUNT(sMapSecAquaHideoutOld); i++)
    {
        if (sMapSecAquaHideoutOld[i] == mapSecId)
            return TRUE;
    }
    return FALSE;
}

u16 CDMap_CorrectSpecialMapSecId(u16 mapSecId)
{
    return CDMap_CorrectSpecialMapSecId_Internal(mapSecId);
}

static void CDMap_GetPositionOfCursorWithinMapSec(void)
{
    u16 x;
    u16 y;
    u16 posWithinMapSec;

    if (gRegionMap->primaryMapSecId == MAPSEC_NONE)
    {
        gRegionMap->posWithinMapSec = 0;
        return;
    }
    x = gRegionMap->cursorPosX;
    y = gRegionMap->cursorPosY;
    posWithinMapSec = 0;
    while (1)
    {
        if (x <= 0)
        {
            if (CDMap_RegionMap_IsMapSecIdInNextRow(y))
            {
                y--;
                x = MAP_WIDTH;
            }
            else
            {
                break;
            }
        }
        else
        {
            x--;
            if (CDMap_GetMapSecIdAt(x, y, gRegionMap->currentRegion, FALSE) == gRegionMap->primaryMapSecId)
            {
                posWithinMapSec++;
            }
        }
    }
    gRegionMap->posWithinMapSec = posWithinMapSec;
}

static bool8 CDMap_RegionMap_IsMapSecIdInNextRow(u16 y)
{
    u16 x;

    if (y-- == 0)
    {
        return FALSE;
    }
    for (x = 0; x < MAP_WIDTH; x++)
    {
        if (CDMap_GetMapSecIdAt(x, y, gRegionMap->currentRegion, FALSE) == gRegionMap->primaryMapSecId)
        {
            return TRUE;
        }
    }
    return FALSE;
}

static void CDMap_SpriteCB_CursorMapFull(struct Sprite *sprite)
{
    if (gRegionMap->cursorMovementFrameCounter != 0)
    {
        sprite->x += 2 * gRegionMap->cursorDeltaX;
        sprite->y += 2 * gRegionMap->cursorDeltaY;
        gRegionMap->cursorMovementFrameCounter--;
    }
}

void CDMap_CreateRegionMapCursor(u16 tileTag, u16 paletteTag, bool8 visible)
{
    u8 spriteId;
    struct Sprite *sprite;
    struct SpriteTemplate template;
    struct SpritePalette palette;
    struct SpriteSheet sheet;

    palette = sRegionMapCursorSpritePalette;
    template = sRegionMapCursorSpriteTemplate;
    sheet.tag = tileTag;
    template.tileTag = tileTag;
    gRegionMap->cursorTileTag = tileTag;
    palette.tag = paletteTag;
    template.paletteTag = paletteTag;
    gRegionMap->cursorPaletteTag = paletteTag;
    sheet.data = gRegionMap->cursorImage;
    sheet.size = sizeof(gRegionMap->cursorImage);
    LoadSpriteSheet(&sheet);
    LoadSpritePalette(&palette);
    spriteId = CreateSprite(&template, 0x38, 0x48, 0);
    if (spriteId != MAX_SPRITES)
    {
        gRegionMap->spriteIds[0] = spriteId;
        sprite = &gSprites[spriteId];
        sprite->oam.size = 1;

        if (visible)
        {
            sprite->x = (gRegionMap->cursorPosX + gRegionMap->xOffset + MAPCURSOR_X_MIN) * 8 + 4;
            sprite->y = (gRegionMap->cursorPosY + gRegionMap->yOffset + MAPCURSOR_Y_MIN) * 8 + 4;
        }
        else
        {
            sprite->invisible = TRUE;
            sprite->callback = SpriteCallbackDummy;
        }

        sprite->data[1] = 2;
        sprite->data[2] = (IndexOfSpritePaletteTag(paletteTag) << 4) + 0x101;
        sprite->data[3] = TRUE;
    }
}

void CDMap_ShowRegionMapCursorSprite(void)
{
    if (gRegionMap->spriteIds[0] != 0xFF)
    {
        struct Sprite *sprite = &gSprites[gRegionMap->spriteIds[0]];

        sprite->x = (gRegionMap->cursorPosX + gRegionMap->xOffset + MAPCURSOR_X_MIN) * 8 + 4;
        sprite->y = (gRegionMap->cursorPosY + gRegionMap->yOffset + MAPCURSOR_Y_MIN) * 8 + 4;
        sprite->callback = CDMap_SpriteCB_CursorMapFull;
        StartSpriteAnim(sprite, 0);
        sprite->invisible = FALSE;
    }
}

void CDMap_HideRegionMapCursorSprite(void)
{
    if (gRegionMap->spriteIds[0] != 0xFF)
    {
        struct Sprite *sprite = &gSprites[gRegionMap->spriteIds[0]];

        gRegionMap->cursorPosX = gRegionMap->playerIconSpritePosX;
        gRegionMap->cursorPosY = gRegionMap->playerIconSpritePosY;
        CDMap_LoadMapLayersFromPosition(gRegionMap->cursorPosX, gRegionMap->cursorPosY);

        sprite->invisible = TRUE;
        sprite->callback = SpriteCallbackDummy;
    }
}

// Unused
static void CDMap_SetUnkCursorSpriteData(void)
{
    gSprites[gRegionMap->spriteIds[0]].data[3] = TRUE;
}

// Unused
static void CDMap_ClearUnkCursorSpriteData(void)
{
    gSprites[gRegionMap->spriteIds[0]].data[3] = FALSE;
}

void CDMap_CreateRegionMapPlayerIcon(u16 tileTag, u16 paletteTag)
{
    struct Sprite *sprite;
    struct SpriteSheet sheet = {sRegionMapPlayerIcon_GoldGfx, 0x80, tileTag};
    struct SpritePalette palette = {sRegionMapPlayerIcon_GoldPal, paletteTag};
    struct SpriteTemplate template = {tileTag, paletteTag, &sRegionMapPlayerIconOam, gDummySpriteAnimTable, NULL, gDummySpriteAffineAnimTable, SpriteCallbackDummy};

    if (CDMap_IsEventIslandMapSecId(gMapHeader.regionMapSectionId))
    {
        gRegionMap->spriteIds[1] = 0xFF;
        return;
    }
    if (gSaveBlock2Ptr->playerGender == FEMALE)
    {
        sheet.data = sRegionMapPlayerIcon_KrisGfx;
        palette.data = sRegionMapPlayerIcon_KrisPal;
    }
    if (gMapHeader.regionMapSectionId == MAPSEC_FAST_SHIP)
    {
        sheet.data = sRegionMapPlayerIcon_ShipGfx;
        sheet.size = 0x200;
        palette.data = sRegionMapPlayerIcon_ShipPal;
        template.oam = &sRegionMapShipIconOam;
    }
    LoadSpriteSheet(&sheet);
    LoadSpritePalette(&palette);
    gRegionMap->spriteIds[1] = CreateSprite(&template, 0, 0, 2);
    sprite = &gSprites[gRegionMap->spriteIds[1]];
    sprite->x = (gRegionMap->playerIconSpritePosX + gRegionMap->xOffset + MAPCURSOR_X_MIN) * 8 + 4;
    sprite->y = (gRegionMap->playerIconSpritePosY + gRegionMap->yOffset + MAPCURSOR_Y_MIN) * 8 + 4;
    if(FlagGet(FLAG_FAST_SHIP_DESTINATION_OLIVINE) && gMapHeader.regionMapSectionId == MAPSEC_FAST_SHIP)
        sprite->oam.matrixNum |= ST_OAM_HFLIP;
    if(gMapHeader.regionMapSectionId == MAPSEC_FAST_SHIP)
    {
        sprite->callback = CDMap_SpriteCB_ShipIcon;
        sprite->y -= gRegionMap->yOffset * 8;
    }
}

static void CDMap_HideRegionMapPlayerIcon(void)
{
    if (gRegionMap->spriteIds[1] != 0xFF)
    {
        struct Sprite *sprite = &gSprites[gRegionMap->spriteIds[1]];

        sprite->invisible = TRUE;
        sprite->callback = SpriteCallbackDummy;
    }
}

static void CDMap_UnhideRegionMapPlayerIcon(void)
{
    if (gRegionMap->spriteIds[1] != 0xFF)
    {
        struct Sprite *sprite = &gSprites[gRegionMap->spriteIds[1]];

        sprite->x = (gRegionMap->playerIconSpritePosX + gRegionMap->xOffset + MAPCURSOR_X_MIN) * 8 + 4;
        sprite->y = (gRegionMap->playerIconSpritePosY + gRegionMap->yOffset + MAPCURSOR_Y_MIN) * 8 + 4;
        sprite->x2 = 0;
        sprite->y2 = 0;
        sprite->invisible = FALSE;
    }
}

static void CDMap_SpriteCB_ShipIcon(struct Sprite *sprite)
{
    ++sprite->data[7];
    if (sprite->data[7] % 8 == 0)
    {
        if(sprite->data[7] == 16)
        {
            sprite->y++;
            sprite->data[7] = 0;
        }
        else
            sprite->y--;
    }
}

void CDMap_TrySetPlayerIconBlink(void)
{
    if (gRegionMap->playerIsInCave)
        gRegionMap->blinkPlayerIcon = TRUE;
}

void CDMap_CreateRegionMapName(u16 tileTagCurve, u16 tileTagMain)
{
    u8 nameToDisplay;

    struct SpriteTemplate template;
    struct SpriteSheet curveSheet = {sRegionMapNamesCurve_Gfx, sizeof(sRegionMapNamesCurve_Gfx), tileTagCurve};
    struct SpriteSheet mainSheet = {sRegionMapNames_Gfx, sizeof(sRegionMapNames_Gfx), tileTagMain};

    template = sRegionMapNameCurveSpriteTemplate;
    template.tileTag = tileTagCurve;
    template.paletteTag = gRegionMap->miscSpritesPaletteTag;
    LoadSpriteSheet(&curveSheet);
    gRegionMap->spriteIds[2] = CreateSprite(&template, 180 + gRegionMap->xOffset * 8, 20 + gRegionMap->yOffset * 8, 0);
    gRegionMap->regionNameCurveTileTag = tileTagCurve;

    template = sRegionMapNameSpriteTemplate;
    template.tileTag = tileTagMain;
    template.paletteTag = gRegionMap->miscSpritesPaletteTag;
    LoadSpriteSheet(&mainSheet);
    gRegionMap->spriteIds[3] = CreateSprite(&template, 200 + gRegionMap->xOffset * 8, 20 + gRegionMap->yOffset * 8, 0);
    gRegionMap->regionNameMainTileTag = tileTagMain;

    if (gRegionMap->currentRegion >= CDMAP_REGION_SEVII1)
    {
        nameToDisplay = CDMAP_REGION_SEVII1;
    }
    else
    {
        nameToDisplay = gRegionMap->currentRegion;
    }

    StartSpriteAnim(&gSprites[gRegionMap->spriteIds[3]], gRegionMap->currentRegion);
}

void CDMap_CreateSecondaryLayerDots(u16 tileTag, u16 paletteTag)
{
    u32 i = 0;
    u16 x, y, newX, newY;

    struct SpriteSheet sheet = {sRegionMapDots_Gfx, sizeof(sRegionMapDots_Gfx), tileTag};
    struct SpritePalette palette = {sRegionMapDots_Pal, paletteTag};
    struct SpriteTemplate template = {tileTag, paletteTag, &sRegionMapDotsOam, sRegionMapDotsAnimTable, NULL, gDummySpriteAffineAnimTable, SpriteCallbackDummy};

    LoadSpriteSheet(&sheet);
    LoadSpritePalette(&palette);
    gRegionMap->dotsTileTag = tileTag;
    gRegionMap->miscSpritesPaletteTag = paletteTag;

    for (y = 0; y < MAP_HEIGHT; y++)
    {
        for (x = 0; x < MAP_WIDTH; x++)
        {
            u8 secondaryMapSec = CDMap_GetMapSecIdAt(x, y, gRegionMap->currentRegion, TRUE);

            if (secondaryMapSec != MAPSEC_NONE)
            {
                u8 spriteId;

                if ((gRegionMapEntries[secondaryMapSec].width > 1 || gRegionMapEntries[secondaryMapSec].height > 1))
                {
                    if (x == gRegionMapEntries[secondaryMapSec].x && y == gRegionMapEntries[secondaryMapSec].y)
                    {
                        newX = (gRegionMapEntries[secondaryMapSec].width * 8) / 2 + (gRegionMapEntries[secondaryMapSec].x + MAPCURSOR_X_MIN + gRegionMap->xOffset) * 8;
                        newY = (gRegionMapEntries[secondaryMapSec].height * 8) / 2 + (gRegionMapEntries[secondaryMapSec].y + MAPCURSOR_Y_MIN + gRegionMap->yOffset) * 8;
                        spriteId = CreateSprite(&template, newX, newY, 3);
                    }
                    else
                    {
                        continue;
                    }
                }
                else
                {
                    u8 offset = 0;

                    if (CDMap_GetMapsecType(CDMap_GetMapSecIdAt(x, y, gRegionMap->currentRegion, FALSE)) >= MAPSECTYPE_VISITED)
                    {
                        // CrystalDust left the cursor unshifted on
                        // MAPSEC_ROUTE_10_FLYDUP and MAPSEC_ROUTE_3_FLYDUP.
                        // Neither section exists here; region_map_sections.json
                        // is at its 252-entry ceiling. See D33.
                        offset = 2;
                    }

                    spriteId = CreateSprite(&template, (x + MAPCURSOR_X_MIN + gRegionMap->xOffset) * 8 + offset + 4, (y + MAPCURSOR_Y_MIN + gRegionMap->yOffset) * 8 + offset + 4, 3);
                }

                if (CDMap_GetMapsecType(secondaryMapSec) == MAPSECTYPE_VISITED)
                {
                    StartSpriteAnim(&gSprites[spriteId], 1);
                }

                gRegionMap->spriteIds[i++ + 4] = spriteId;

                if (i + 4 > sizeof(gRegionMap->spriteIds))
                {
                    return;
                }
            }
        }
    }
}




bool8 CDMap_IsGoldenrodDeptStore(u16 mapSec)
{
    if (mapSec != MAPSEC_GOLDENROD_CITY)
        return FALSE;
    if (gSaveBlock1Ptr->location.mapGroup != MAP_GROUP(MAP_GOLDENROD_CITY_DEPT_STORE_1F))
        return FALSE;
    if (gSaveBlock1Ptr->location.mapNum != MAP_NUM(MAP_GOLDENROD_CITY_DEPT_STORE_1F)
     && gSaveBlock1Ptr->location.mapNum != MAP_NUM(MAP_GOLDENROD_CITY_DEPT_STORE_2F)
     && gSaveBlock1Ptr->location.mapNum != MAP_NUM(MAP_GOLDENROD_CITY_DEPT_STORE_3F)
     && gSaveBlock1Ptr->location.mapNum != MAP_NUM(MAP_GOLDENROD_CITY_DEPT_STORE_4F)
     && gSaveBlock1Ptr->location.mapNum != MAP_NUM(MAP_GOLDENROD_CITY_DEPT_STORE_5F)
     && gSaveBlock1Ptr->location.mapNum != MAP_NUM(MAP_GOLDENROD_CITY_DEPT_STORE_6F)
     && gSaveBlock1Ptr->location.mapNum != MAP_NUM(MAP_GOLDENROD_CITY_DEPT_STORE_ROOFTOP)
     && gSaveBlock1Ptr->location.mapNum != MAP_NUM(MAP_GOLDENROD_CITY_DEPT_STORE_ELEVATOR))
        return FALSE;
    return TRUE;
}

bool8 CDMap_IsCeladonDeptStore(u16 mapSec)
{
    if (mapSec != MAPSEC_CELADON_CITY)
        return FALSE;
    if (gSaveBlock1Ptr->location.mapGroup != MAP_GROUP(MAP_CELADON_CITY_DEPARTMENT_STORE_1F))
        return FALSE;
    if (gSaveBlock1Ptr->location.mapNum != MAP_NUM(MAP_CELADON_CITY_DEPARTMENT_STORE_1F)
     && gSaveBlock1Ptr->location.mapNum != MAP_NUM(MAP_CELADON_CITY_DEPARTMENT_STORE_2F)
     && gSaveBlock1Ptr->location.mapNum != MAP_NUM(MAP_CELADON_CITY_DEPARTMENT_STORE_3F)
     && gSaveBlock1Ptr->location.mapNum != MAP_NUM(MAP_CELADON_CITY_DEPARTMENT_STORE_4F)
     && gSaveBlock1Ptr->location.mapNum != MAP_NUM(MAP_CELADON_CITY_DEPARTMENT_STORE_5F)
     && gSaveBlock1Ptr->location.mapNum != MAP_NUM(MAP_CELADON_CITY_DEPARTMENT_STORE_ROOF)
     && gSaveBlock1Ptr->location.mapNum != MAP_NUM(MAP_CELADON_CITY_DEPARTMENT_STORE_ELEVATOR))
        return FALSE;
    return TRUE;
}


static void CDMap_GetMapSecDimensions(u16 mapSecId, u16 *x, u16 *y, u16 *width, u16 *height)
{
    *x = gRegionMapEntries[mapSecId].x;
    *y = gRegionMapEntries[mapSecId].y;
    *width = gRegionMapEntries[mapSecId].width;
    *height = gRegionMapEntries[mapSecId].height;
}

bool8 CDMap_IsRegionMapZoomed(void)
{
    return FALSE;
}

bool32 CDMap_IsEventIslandMapSecId(u8 mapSecId)
{
    u32 i;

    for (i = 0; i < ARRAY_COUNT(sMapSecIdsOffMap); i++)
    {
        if (mapSecId == sMapSecIdsOffMap[i])
            return TRUE;
    }
    return FALSE;
}

// CrystalDust's Fly map (CB2_OpenFlyMap and friends) is not ported here: this
// module only backs the Pokegear map card. Expansion's own Fly map in
// region_map.c stays in use, and CrystalDust's Johto heal locations do not
// exist yet. See D33.

u8 CDMap_GetMapRegion(u16 mapSectionId)
{
    return sMapSecToRegion[mapSectionId];
}

u8 CDMap_GetCurrentRegion(void)
{
    return CDMap_GetMapRegion(gMapHeader.regionMapSectionId);
}

static bool32 CDMap_SelectedMapsecSEEnabled(void)
{
    if (gRegionMap->primaryMapSecId == MAPSEC_ROUTE_32_FLYDUP)
        return FALSE;
    else
        return TRUE;
}

void CDMap_PlaySEForSelectedMapsec(void)
{
    if (CDMap_SelectedMapsecSEEnabled())
    {
        if ((gRegionMap->primaryMapSecStatus != MAPSECTYPE_ROUTE && gRegionMap->primaryMapSecStatus != MAPSECTYPE_NONE)
         || (gRegionMap->secondaryMapSecStatus != MAPSECTYPE_ROUTE && gRegionMap->secondaryMapSecStatus != MAPSECTYPE_NONE && gRegionMap->enteredSecondary))
            PlaySE(SE_DEX_SCROLL);
        else if ((gRegionMap->permissions[MAPPERM_CLOSE] || gRegionMap->permissions[MAPPERM_SWITCH]) &&
                  gRegionMap->cursorPosX == CORNER_BUTTON_X && gRegionMap->cursorPosY == CORNER_BUTTON_Y)
            PlaySE(SE_M_SPIT_UP);
    }
}

u8 CDMap_GetSelectedMapsecLandmarkState(void)
{
    if (gRegionMap->secondaryMapSecId != MAPSEC_NONE)
    {
        if (gRegionMap->permissions[MAPPERM_LANDMARKINFO] == TRUE && gRegionMap->secondaryMapSecStatus == MAPSECTYPE_VISITED)
        {
            return LANDMARK_STATE_INFO;
        }
    }
    else if (gRegionMap->cursorPosX == CORNER_BUTTON_X && gRegionMap->cursorPosY == CORNER_BUTTON_Y)
    {
        if (gRegionMap->permissions[MAPPERM_SWITCH])
        {
            return LANDMARK_STATE_SWITCH;
        }
        else if (gRegionMap->permissions[MAPPERM_CLOSE])
        {
            return LANDMARK_STATE_CLOSE;
        }
    }

    return LANDMARK_STATE_NONE;
}
