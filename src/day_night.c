#include "global.h"
#include "day_night.h"
#include "decompress.h"
#include "event_data.h"
#include "event_object_movement.h"
#include "field_tasks.h"
#include "field_weather.h"
#include "fruit_tree.h"
#include "overworld.h"
#include "palette.h"
#include "rtc.h"
#include "strings.h"
#include "string_util.h"
#include "constants/day_night.h"
#include "constants/layouts.h"
#include "constants/maps.h"
#include "constants/region_map_sections.h"
#include "constants/rgb.h"


EWRAM_DATA struct PaletteOverride *gPaletteOverrides[4] = {NULL};

void NewInitObjectEventPalettes(void);

static EWRAM_DATA struct {
    u8 timeOfDay;
} sDNSystemControl = {0};

#if DEBUG
EWRAM_DATA bool8 gPaletteOverrideDisabled = 0;
// CrystalDust's time-cycle debug screen. gDNPeriodOverride still works: it is
// pushed into expansion's sHoursOverride. gDNTintOverride is inert now that
// expansion owns the tinting - see D31.
EWRAM_DATA s16 gDNPeriodOverride = 0;
EWRAM_DATA u16 gDNTintOverride[3] = {0};
#endif


const u8 *const gDayOfWeekTable[] = 
{
    gText_Sunday,
    gText_Monday,
    gText_Tuesday,
    gText_Wednesday,
    gText_Thursday,
    gText_Friday,
    gText_Saturday
};



const u8 *GetDayOfWeekString(u8 dayOfWeek)
{
    return gDayOfWeekTable[dayOfWeek];
}

void CopyDayOfWeekStringToVar1(void)
{
    if (gSpecialVar_0x8004 <= DAY_SATURDAY)
        StringCopy(gStringVar1, gDayOfWeekTable[gSpecialVar_0x8004]);
    else
        StringCopy(gStringVar1, gText_None);
}

void CopyCurrentDayOfWeekStringToVar1(void)
{
    RtcCalcLocalTime();
    if (GetDayOfWeek() <= DAY_SATURDAY)
        StringCopy(gStringVar1, gDayOfWeekTable[GetDayOfWeek()]);
    else
        StringCopy(gStringVar1, gText_None);
}

bool32 ShouldSetTintToNight(void)
{
    switch(gMapHeader.mapLayoutId)
    {
        case LAYOUT_ILEX_FOREST:
        case LAYOUT_DRAGONS_DEN_ENTRANCE:
        case LAYOUT_DRAGONS_DEN:
        case LAYOUT_DRAGONS_DEN_SHRINE:
        case LAYOUT_FUCHSIA_CITY_SAFARI_ZONE_OFFICE:
            return TRUE;
    }
    if(gSaveBlock1Ptr->location.mapGroup == MAP_GROUP(MAP_LIGHTHOUSE_6F) && gSaveBlock1Ptr->location.mapNum == MAP_NUM(MAP_LIGHTHOUSE_6F) && !FlagGet(FLAG_CURED_AMPHY))
        return TRUE;
    return FALSE;
}

static void LoadPaletteOverrides(void)
{
    u32 i, j;
    const u16* src;
    u16* dest;
    s8 hour;

#if DEBUG
    if (gPaletteOverrideDisabled)
        return;
#endif

    if (ShouldSetTintToNight())
    {
        hour = 0;
    }
    else
    {
        hour = gLocalTime.hours;
    }

    for (i = 0; i < ARRAY_COUNT(gPaletteOverrides); i++)
    {
        const struct PaletteOverride *curr = gPaletteOverrides[i];
        if (curr != NULL)
        {
            while (curr->slot != PALOVER_LIST_TERM && curr->palette != NULL)
            {
                if ((curr->startHour < curr->endHour && hour >= curr->startHour && hour < curr->endHour) ||
                    (curr->startHour > curr->endHour && (hour >= curr->startHour || hour < curr->endHour)))
                {
                    for (j = 0, src = curr->palette, dest = gPlttBufferUnfaded + (curr->slot * 16); j < 16; j++, src++, dest++)
                    {
                        if (*src != RGB_BLACK)
                            *dest = *src;
                    }
                }
                curr++;
            }
        }
    }
}

static bool8 ShouldTintOverworld(void)
{
    if (IsMapTypeOutdoors(gMapHeader.mapType))
        return TRUE;
    return ShouldSetTintToNight();
}



// CrystalDust's LoadPaletteDayNight / LoadCompressedPaletteDayNight /
// DoLoadSpritePaletteDayNight were removed in Phase 2: nothing in this tree
// called them. Expansion loads every palette through its own time-of-day aware
// path instead. See D31.

void CheckClockForImmediateTimeEvents(void)
{
    if (ShouldTintOverworld())
        RtcCalcLocalTime();
}

void ProcessImmediateTimeEvents(void)
{
    u8 timeOfDay;

#if DEBUG
    if (gDNPeriodOverride > 0)
        SetTimeOfDay((gDNPeriodOverride - 1) / TINT_PERIODS_PER_HOUR);
    else if (gDNPeriodOverride < 0)
        SetTimeOfDay(0);
#endif

    timeOfDay = GetTimeOfDay();

    if (ShouldTintOverworld())
    {
        LoadPaletteOverrides();

        if (gWeatherPtr->palProcessingState != WEATHER_PAL_STATE_SCREEN_FADING_IN &&
            gWeatherPtr->palProcessingState != WEATHER_PAL_STATE_SCREEN_FADING_OUT)
            CpuCopy16(gPlttBufferUnfaded, gPlttBufferFaded, PLTT_SIZE);
    }

    if (sDNSystemControl.timeOfDay != timeOfDay)
    {
        sDNSystemControl.timeOfDay = timeOfDay;
        ForceChooseAmbientCrySpecies(); // so a time-of-day appropriate mon is chosen
        ForceTimeBasedEvents();    // misc events that should run on time-of-day boundaries
    }
}


void NewInitObjectEventPalettes(void)
{
    InitObjectEventPalettes(0);
}