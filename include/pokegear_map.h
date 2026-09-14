#ifndef GUARD_POKEGEAR_MAP_H
#define GUARD_POKEGEAR_MAP_H

#include "bg.h"
#include "region_map.h"
#include "regions.h"

// CrystalDust's region map indexes its own map images, not expansion's core
// enum Region. Kept separate so the two cannot be confused (D33).
enum {
    CDMAP_REGION_JOHTO,
    CDMAP_REGION_KANTO,
    CDMAP_REGION_SEVII1,
    CDMAP_REGION_SEVII2,
    CDMAP_REGION_SEVII3
};

// CrystalDust's map-input results that expansion's enum does not name (D33).
enum {
    MAP_INPUT_SWITCH = MAP_INPUT_B_BUTTON,
    MAP_INPUT_CANCEL = MAP_INPUT_R_BUTTON
};

#define MAPSECTYPE_VISITED     MAPSECTYPE_CITY_CANFLY
#define MAPSECTYPE_NOT_VISITED MAPSECTYPE_CITY_CANTFLY

#define CORNER_BUTTON_X 21
#define CORNER_BUTTON_Y 13



// Exported type declarations


enum {
    MAPPERM_CLOSE,
    MAPPERM_SWITCH,
    MAPPERM_LANDMARKINFO,
    MAPPERM_FLY
};


enum {
    MAPMODE_POKEGEAR,
    MAPMODE_FIELD,
    MAPMODE_FLY
};

enum {
    LANDMARK_STATE_NONE,
    LANDMARK_STATE_INFO,
    LANDMARK_STATE_CLOSE,
    LANDMARK_STATE_SWITCH
};

struct CDRegionMap {
    u8 primaryMapSecId;
    u8 secondaryMapSecId;
    u8 primaryWindowId;
    u8 secondaryWindowId;
    u8 primaryMapSecStatus;
    u8 secondaryMapSecStatus;
    u8 posWithinMapSec;
    u8 enteredSecondary;
    u8 currentRegion;
    u8 mapMode;
    bool8 permissions[4];
    u8 primaryMapSecName[0x14];
    u8 secondaryMapSecName[0x14];
    u8 (*inputCallback)(void);
    u8 spriteIds[20];
    u16 regionNameCurveTileTag;
    u16 regionNameMainTileTag;
    u16 dotsTileTag;
    u16 miscSpritesPaletteTag;
    s32 unk_03c;
    s32 unk_040;
    s32 unk_044;
    s32 unk_048;
    s32 unk_04c;
    s32 unk_050;
    s16 cursorPosX;
    s16 cursorPosY;
    u16 cursorTileTag;
    u16 cursorPaletteTag;
    s16 scrollX;
    s16 scrollY;
    s16 unk_060;
    s16 unk_062;
    u16 unk_06e;
    u16 playerIconTileTag;
    u16 playerIconPaletteTag;
    u16 playerIconSpritePosX;
    u16 playerIconSpritePosY;
    u8 initStep;
    s8 cursorMovementFrameCounter;
    s8 cursorDeltaX;
    s8 cursorDeltaY;
    bool8 needUpdateVideoRegs;
    bool8 blinkPlayerIcon;
    bool8 playerIsInCave;
    u8 bgNum;
    u8 charBaseIdx;
    u8 mapBaseIdx;
    bool8 bgManaged;
    s8 xOffset;
    s8 yOffset;
    bool8 onButton;
    u8 ALIGNED(4) cursorImage[0x100];
}; // size = 0x884


// Exported RAM declarations

// Exported ROM declarations
void CDMap_InitRegionMapData(struct CDRegionMap *regionMap, const struct BgTemplate *template, u8 mapMode, s8 xOffset, s8 yOffset);
bool8 CDMap_ChangeDecompressedRegionMapGfx(u16* ptr, bool8* permissions);
bool8 CDMap_LoadRegionMapGfx(bool8 shouldBuffer);
bool8 CDMap_LoadRegionMapGfx_Pt2(void);
void CDMap_UpdateRegionMapVideoRegs(void);
void CDMap_InitRegionMap(struct CDRegionMap *regionMap, u8 mode, s8 xOffset, s8 yOffset);
u8 CDMap_DoRegionMapInputCallback(void);
bool8 CDMap_UpdateRegionMapZoom(void);
void CDMap_FreeRegionMapResources(void);
bool8 CDMap_MapsecWasVisited(u16 mapSecId);
u16 CDMap_GetRegionMapSectionIdAt(u16 x, u16 y, u8 region);
void CDMap_CreateRegionMapPlayerIcon(u16 x, u16 y);
void CDMap_CreateRegionMapCursor(u16 tileTag, u16 paletteTag, bool8 visible);
bool32 CDMap_IsEventIslandMapSecId(u8 mapSecId);
u8 CDMap_GetMapRegion(u16 mapSectionId);
u8 CDMap_GetCurrentRegion(void);
void CDMap_ShowRegionMapCursorSprite(void);
void CDMap_HideRegionMapCursorSprite(void);
void CDMap_CreateRegionMapName(u16 tileTagCurve, u16 tileTagMain);
void CDMap_CreateSecondaryLayerDots(u16 tileTag, u16 paletteTag);
u16 CDMap_CorrectSpecialMapSecId(u16 mapSecId);
void CDMap_ShowRegionMapForPokedexAreaScreen(struct CDRegionMap *regionMap);
void CDMap_PokedexAreaScreen_UpdateRegionMapVariablesAndVideoRegs(s16 x, s16 y);
bool8 CDMap_IsRegionMapZoomed(void);
void CDMap_TrySetPlayerIconBlink(void);
const u16 *CDMap_GetRegionMapPalette(void);
const u32 *CDMap_GetRegionMapTileset(void);
const u32 *CDMap_GetRegionMapTilemap(u8 region);
void CDMap_BlendRegionMap(u16 color, u32 coeff);
void CDMap_SetRegionMapDataForZoom(void);
void CDMap_PlaySEForSelectedMapsec(void);
u8 CDMap_GetSelectedMapsecLandmarkState(void);



#endif //GUARD_POKEGEAR_MAP_H
