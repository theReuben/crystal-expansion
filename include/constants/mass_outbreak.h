#ifndef GUARD_CONSTANTS_MASS_OUTBREAK_H
#define GUARD_CONSTANTS_MASS_OUTBREAK_H

// CrystalDust's swarms, announced by Pokegear phone calls (D71). Expansion's
// Hoenn outbreak list keyed on stub map constants, so it could never fire.
enum MassOutbreakIndex
{
    OUTBREAK_ID_DARK_CAVE,
    OUTBREAK_ID_ROUTE32,
    OUTBREAK_ID_ROUTE35,
    OUTBREAK_COUNT
};

// Which encounter kind a swarm replaces. CrystalDust's Qwilfish swarm is
// fished up rather than walked into (D71).
#define OUTBREAK_WALKING 1
#define OUTBREAK_SURFING 2
#define OUTBREAK_FISHING 3

enum MassOutbreakData
{
    OUTBREAK_DATA_SPECIES,
    OUTBREAK_DATA_MOVE1,
    OUTBREAK_DATA_MOVE2,
    OUTBREAK_DATA_MOVE3,
    OUTBREAK_DATA_MOVE4,
    OUTBREAK_DATA_LEVEL,
    OUTBREAK_DATA_PROBABILITY,
    OUTBREAK_DATA_DAYS_LEFT,
    OUTBREAK_DATA_MAP
};

#endif // GUARD_MASS_OUTBREAK_H

