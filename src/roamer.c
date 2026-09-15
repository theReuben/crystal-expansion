#include "global.h"
#include "event_data.h"
#include "ow_abilities.h"
#include "pokemon.h"
#include "random.h"
#include "roamer.h"

// Despite having a variable to track it, the roamer is
// hard-coded to only ever be in map group 0
#define ROAMER_MAP_GROUP 0

enum
{
    MAP_GRP, // map group
    MAP_NUM, // map number
};

#define ROAMER(index) (&gSaveBlock1Ptr->roamer[index])
EWRAM_DATA static u8 sLocationHistory[ROAMER_COUNT][3][2] = {0};
EWRAM_DATA static u8 sRoamerLocation[ROAMER_COUNT][2] = {0};
EWRAM_DATA u8 gEncounteredRoamerIndex = 0;

#define ___ MAP_NUM(MAP_UNDEFINED) // For empty spots in the location table

// Note: There are two potential softlocks that can occur with this table if its maps are
//       changed in particular ways. They can be avoided by ensuring the following:
//       - There must be at least 2 location sets that start with a different map,
//         i.e. every location set cannot start with the same map. This is because of
//         the while loop in RoamerMoveToOtherLocationSet.
//       - Each location set must have at least 3 unique maps. This is because of
//         the while loop in RoamerMove. In this loop the first map in the set is
//         ignored, and an additional map is ignored if the roamer was there recently.
//       - Additionally, while not a softlock, it's worth noting that if for any
//         map in the location table there is not a location set that starts with
//         that map then the roamer will be significantly less likely to move away
//         from that map when it lands there.
// CrystalDust's Johto roaming routes (D64). Route 39 gains Route 42 as a
// third option: CrystalDust only gave it Route 38, which hangs the
// move loop when the player was on Route 38 two moves ago.
static const u8 sRoamerLocations[][6] =
{
    { MAP_NUM(MAP_ROUTE29), MAP_NUM(MAP_ROUTE30), MAP_NUM(MAP_ROUTE46), ___, ___, ___ },
    { MAP_NUM(MAP_ROUTE30), MAP_NUM(MAP_ROUTE29), MAP_NUM(MAP_ROUTE31), ___, ___, ___ },
    { MAP_NUM(MAP_ROUTE31), MAP_NUM(MAP_ROUTE30), MAP_NUM(MAP_ROUTE32), MAP_NUM(MAP_ROUTE36), ___, ___ },
    { MAP_NUM(MAP_ROUTE32), MAP_NUM(MAP_ROUTE36), MAP_NUM(MAP_ROUTE31), MAP_NUM(MAP_ROUTE33), ___, ___ },
    { MAP_NUM(MAP_ROUTE33), MAP_NUM(MAP_ROUTE32), MAP_NUM(MAP_ROUTE34), ___, ___, ___ },
    { MAP_NUM(MAP_ROUTE34), MAP_NUM(MAP_ROUTE33), MAP_NUM(MAP_ROUTE35), ___, ___, ___ },
    { MAP_NUM(MAP_ROUTE35), MAP_NUM(MAP_ROUTE34), MAP_NUM(MAP_ROUTE36), ___, ___, ___ },
    { MAP_NUM(MAP_ROUTE36), MAP_NUM(MAP_ROUTE35), MAP_NUM(MAP_ROUTE31), MAP_NUM(MAP_ROUTE32), MAP_NUM(MAP_ROUTE37), ___ },
    { MAP_NUM(MAP_ROUTE37), MAP_NUM(MAP_ROUTE36), MAP_NUM(MAP_ROUTE38), MAP_NUM(MAP_ROUTE42), ___, ___ },
    { MAP_NUM(MAP_ROUTE38), MAP_NUM(MAP_ROUTE37), MAP_NUM(MAP_ROUTE39), MAP_NUM(MAP_ROUTE42), ___, ___ },
    { MAP_NUM(MAP_ROUTE39), MAP_NUM(MAP_ROUTE38), MAP_NUM(MAP_ROUTE42), ___, ___, ___ },
    { MAP_NUM(MAP_ROUTE42), MAP_NUM(MAP_ROUTE43), MAP_NUM(MAP_ROUTE44), MAP_NUM(MAP_ROUTE37), MAP_NUM(MAP_ROUTE38), ___ },
    { MAP_NUM(MAP_ROUTE43), MAP_NUM(MAP_ROUTE42), MAP_NUM(MAP_ROUTE44), ___, ___, ___ },
    { MAP_NUM(MAP_ROUTE44), MAP_NUM(MAP_ROUTE42), MAP_NUM(MAP_ROUTE43), MAP_NUM(MAP_ROUTE45), ___, ___ },
    { MAP_NUM(MAP_ROUTE45), MAP_NUM(MAP_ROUTE44), MAP_NUM(MAP_ROUTE45), ___, ___, ___ },
    { MAP_NUM(MAP_ROUTE46), MAP_NUM(MAP_ROUTE45), MAP_NUM(MAP_ROUTE29), ___, ___, ___ },
    { ___, ___, ___, ___, ___, ___ },
};

#undef ___
#define NUM_LOCATION_SETS (ARRAY_COUNT(sRoamerLocations) - 1)
#define NUM_LOCATIONS_PER_SET (ARRAY_COUNT(sRoamerLocations[0]))

void DeactivateAllRoamers(void)
{
    u32 i;

    for (i = 0; i < ROAMER_COUNT; i++)
        SetRoamerInactive(i);
}

static void ClearRoamerLocationHistory(u32 roamerIndex)
{
    u32 i;

    for (i = 0; i < ARRAY_COUNT(sLocationHistory[roamerIndex]); i++)
    {
        sLocationHistory[roamerIndex][i][MAP_GRP] = 0;
        sLocationHistory[roamerIndex][i][MAP_NUM] = 0;
    }
}

void MoveAllRoamersToOtherLocationSets(void)
{
    u32 i;

    for (i = 0; i < ROAMER_COUNT; i++)
        RoamerMoveToOtherLocationSet(i);
}

void MoveAllRoamers(void)
{
    u32 i;

    for (i = 0; i < ROAMER_COUNT; i++)
        RoamerMove(i);
}

static void CreateInitialRoamerMon(u8 index, enum Species species, u8 level)
{
    ClearRoamerLocationHistory(index);
    u32 personality = GetMonPersonality(species,
        GetSynchronizedGender(ROAMER_ORIGIN, species),
        GetSynchronizedNature(ROAMER_ORIGIN, species),
        RANDOM_UNOWN_LETTER);
    CreateMonWithIVs(&gParties[B_TRAINER_OPPONENT_A][0], species, level, personality, OTID_STRUCT_PLAYER_ID, USE_RANDOM_IVS);
    GiveMonInitialMoveset(&gParties[B_TRAINER_OPPONENT_A][0]);
    ROAMER(index)->ivs = GetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_IVS);
    ROAMER(index)->personality = GetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_PERSONALITY);
    ROAMER(index)->species = species;
    ROAMER(index)->level = level;
    ROAMER(index)->statusA = 0;
    ROAMER(index)->statusB = 0;
    ROAMER(index)->hp = GetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_MAX_HP);
    ROAMER(index)->cool = GetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_COOL);
    ROAMER(index)->beauty = GetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_BEAUTY);
    ROAMER(index)->cute = GetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_CUTE);
    ROAMER(index)->smart = GetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_SMART);
    ROAMER(index)->tough = GetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_TOUGH);
    ROAMER(index)->shiny = GetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_IS_SHINY);
    ROAMER(index)->active = TRUE;
    sRoamerLocation[index][MAP_GRP] = ROAMER_MAP_GROUP;
    sRoamerLocation[index][MAP_NUM] = sRoamerLocations[Random() % NUM_LOCATION_SETS][0];
}

static u8 GetFirstInactiveRoamerIndex(void)
{
    u32 i;

    for (i = 0; i < ROAMER_COUNT; i++)
    {
        if (!ROAMER(i)->active)
            return i;
    }
    return ROAMER_COUNT;
}

bool8 TryAddRoamer(enum Species species, u8 level)
{
    u8 index = GetFirstInactiveRoamerIndex();

    if (index < ROAMER_COUNT)
    {
        // Create the roamer and stop searching
        CreateInitialRoamerMon(index, species, level);
        return TRUE;
    }

    // Maximum active roamers found: do nothing and let the calling function know
    return FALSE;
}

// CrystalDust releases Raikou and Entei together from the Burned Tower (D64).
// Suicune is scripted rather than roaming, so it takes no roamer slot.
void InitRoamer(void)
{
    DeactivateAllRoamers();
    TryAddRoamer(SPECIES_RAIKOU, 40);
    TryAddRoamer(SPECIES_ENTEI, 40);
}

void UpdateLocationHistoryForRoamer(void)
{
    u32 i;

    for (i = 0; i < ROAMER_COUNT; i++)
    {
        sLocationHistory[i][2][MAP_GRP] = sLocationHistory[i][1][MAP_GRP];
        sLocationHistory[i][2][MAP_NUM] = sLocationHistory[i][1][MAP_NUM];

        sLocationHistory[i][1][MAP_GRP] = sLocationHistory[i][0][MAP_GRP];
        sLocationHistory[i][1][MAP_NUM] = sLocationHistory[i][0][MAP_NUM];

        sLocationHistory[i][0][MAP_GRP] = gSaveBlock1Ptr->location.mapGroup;
        sLocationHistory[i][0][MAP_NUM] = gSaveBlock1Ptr->location.mapNum;
    }
}

void RoamerMoveToOtherLocationSet(u32 roamerIndex)
{
    u8 mapNum = 0;

    if (!ROAMER(roamerIndex)->active)
        return;

    sRoamerLocation[roamerIndex][MAP_GRP] = ROAMER_MAP_GROUP;

    // Choose a location set that starts with a map
    // different from the roamer's current map
    do
    {
        mapNum = sRoamerLocations[Random() % NUM_LOCATION_SETS][0];
        if (sRoamerLocation[roamerIndex][MAP_NUM] != mapNum)
        {
            sRoamerLocation[roamerIndex][MAP_NUM] = mapNum;
            return;
        }
    } while (sRoamerLocation[roamerIndex][MAP_NUM] == mapNum);
    sRoamerLocation[roamerIndex][MAP_NUM] = mapNum;
}

void RoamerMove(u32 roamerIndex)
{
    u8 locSet = 0;

    if ((Random() % 16) == 0)
    {
        RoamerMoveToOtherLocationSet(roamerIndex);
    }
    else
    {
        if (!ROAMER(roamerIndex)->active)
            return;

        while (locSet < NUM_LOCATION_SETS)
        {
            // Find the location set that starts with the roamer's current map
            if (sRoamerLocation[roamerIndex][MAP_NUM] == sRoamerLocations[locSet][0])
            {
                u8 mapNum;
                // Choose a new map (excluding the first) within this set
                // Also exclude a map if the roamer was there 2 moves ago
                do
                {
                    mapNum = sRoamerLocations[locSet][(Random() % (NUM_LOCATIONS_PER_SET - 1)) + 1];
                } while ((sLocationHistory[roamerIndex][2][MAP_GRP] == ROAMER_MAP_GROUP
                        && sLocationHistory[roamerIndex][2][MAP_NUM] == mapNum)
                        || mapNum == MAP_NUM(MAP_UNDEFINED));
                sRoamerLocation[roamerIndex][MAP_NUM] = mapNum;
                return;
            }
            locSet++;
        }
    }
}

bool8 IsRoamerAt(u32 roamerIndex, u8 mapGroup, u8 mapNum)
{
    if (ROAMER(roamerIndex)->active && mapGroup == sRoamerLocation[roamerIndex][MAP_GRP] && mapNum == sRoamerLocation[roamerIndex][MAP_NUM])
        return TRUE;
    else
        return FALSE;
}

void CreateRoamerMonInstance(u32 roamerIndex)
{
    u32 status = ROAMER(roamerIndex)->statusA + (ROAMER(roamerIndex)->statusB << 8);
    struct Pokemon *mon = &gParties[B_TRAINER_OPPONENT_A][0];
    ZeroEnemyPartyMons();
    CreateMonWithIVsPersonality(mon, ROAMER(roamerIndex)->species, ROAMER(roamerIndex)->level, ROAMER(roamerIndex)->ivs, ROAMER(roamerIndex)->personality);
    SetMonData(mon, MON_DATA_STATUS, &status);
    SetMonData(mon, MON_DATA_HP, &ROAMER(roamerIndex)->hp);
    SetMonData(mon, MON_DATA_COOL, &ROAMER(roamerIndex)->cool);
    SetMonData(mon, MON_DATA_BEAUTY, &ROAMER(roamerIndex)->beauty);
    SetMonData(mon, MON_DATA_CUTE, &ROAMER(roamerIndex)->cute);
    SetMonData(mon, MON_DATA_SMART, &ROAMER(roamerIndex)->smart);
    SetMonData(mon, MON_DATA_TOUGH, &ROAMER(roamerIndex)->tough);
    SetMonData(mon, MON_DATA_IS_SHINY, &ROAMER(roamerIndex)->shiny);
}

bool8 TryStartRoamerEncounter(void)
{
    u32 i;

    for (i = 0; i < ROAMER_COUNT; i++)
    {
        if (IsRoamerAt(i, gSaveBlock1Ptr->location.mapGroup, gSaveBlock1Ptr->location.mapNum) == TRUE && (Random() % 4) == 0)
        {
            CreateRoamerMonInstance(i);
            gEncounteredRoamerIndex = i;
            return TRUE;
        }
    }
    return FALSE;
}

void UpdateRoamerHPStatus(struct Pokemon *mon)
{
    u32 status = GetMonData(mon, MON_DATA_STATUS);

    ROAMER(gEncounteredRoamerIndex)->hp = GetMonData(mon, MON_DATA_HP);
    ROAMER(gEncounteredRoamerIndex)->statusA = status;
    ROAMER(gEncounteredRoamerIndex)->statusB = status >> 8;

    RoamerMoveToOtherLocationSet(gEncounteredRoamerIndex);
}

void SetRoamerInactive(u32 roamerIndex)
{
    ROAMER(roamerIndex)->active = FALSE;
}

void GetRoamerLocation(u32 roamerIndex, u8 *mapGroup, u8 *mapNum)
{
    *mapGroup = sRoamerLocation[roamerIndex][MAP_GRP];
    *mapNum = sRoamerLocation[roamerIndex][MAP_NUM];
}

// CrystalDust's beast specials, restored in Phase 2 (D48). The beasts can be
// scared off and re-roam, so they are regenerated at their fixed level 40 from
// the IVs and personality already stored in the save.
static void RegenerateRoamer(u32 index, u16 species)
{
    struct Pokemon mon;
    struct Roamer *roamer = ROAMER(index);

    CreateMonWithIVsPersonality(&mon, species, 40, roamer->ivs, roamer->personality);
    roamer->species = species;
    roamer->level = 40;
    roamer->hp = GetMonData(&mon, MON_DATA_MAX_HP);
    roamer->statusA = 0;
    roamer->statusB = 0;
    roamer->active = TRUE;
}

void RegenerateRaikou(void)
{
    RegenerateRoamer(ROAMER_RAIKOU, SPECIES_RAIKOU);
}

void RegenerateEntei(void)
{
    RegenerateRoamer(ROAMER_ENTEI, SPECIES_ENTEI);
}

void IsRaikouActive(void)
{
    gSpecialVar_Result = ROAMER(ROAMER_RAIKOU)->active;
}

void IsEnteiActive(void)
{
    gSpecialVar_Result = ROAMER(ROAMER_ENTEI)->active;
}
