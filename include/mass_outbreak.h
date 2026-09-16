#ifndef GUARD_MASS_OUTBREAK_H
#define GUARD_MASS_OUTBREAK_H

#include "global.h"
#include "constants/mass_outbreak.h"

struct MassOutbreak
{
    enum Species species;
    enum Move moves[MAX_MON_MOVES];
    u8 level;
    u8 probability;
    u8 duration;
    u16 location;
    u8 wildState;    // CrystalDust (D71): walking, surfing or fishing.
    u8 specialLevel1; // Old Rod level for a fishing swarm.
    u8 specialLevel2; // Super Rod level for a fishing swarm.
};

void PrepareTvShowForRandomOutbreak(TVShow *show);
void UpdateMassOutbreakDaysLeft(u16 days);
bool32 IsMassOutbreakActive(void);
bool8 SetUpMassOutbreakEncounter(u8 flags);
bool8 DoMassOutbreakEncounterTest(u8 wildState);
u8 GetMassOutbreakFishingLevel(u8 rod);
void StartStaticMassOutbreak(enum MassOutbreakIndex outbreakIdx);
enum Species GetStaticOutbreakSpecies(enum MassOutbreakIndex outbreakIdx);
u8 *GetStaticOutbreakMapName(u8 *dest, enum MassOutbreakIndex outbreakIdx);
void ZeroMassOutbreak(void);
struct MassOutbreak GetStaticOutbreak(enum MassOutbreakIndex outbreakIdx);

#endif // GUARD_MASS_OUTBREAK_H

