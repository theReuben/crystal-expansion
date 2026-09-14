#ifndef GUARD_DAY_NIGHT_H
#define GUARD_DAY_NIGHT_H

#define PALOVER_LIST_TERM 0xFF

struct PaletteOverride
{
    u8 slot;
    u8 startHour;
    u8 endHour;
    void *palette;
};

extern EWRAM_DATA struct PaletteOverride *gPaletteOverrides[];

// GetCurrentTimeOfDay / GetTimeOfDay(s8) removed in Phase 2: expansion's
// GetTimeOfDay(void) in rtc.h is now the single clock reader. See D7.
bool32 ShouldSetTintToNight(void); // CrystalDust (D31)
void CheckClockForImmediateTimeEvents(void);
void ProcessImmediateTimeEvents(void);
const u8 *GetDayOfWeekString(u8 timeOfDay);

#endif // GUARD_DAY_NIGHT_H
