#ifndef GUARD_CONSTANTS_ASM_ENUMS_H
#define GUARD_CONSTANTS_ASM_ENUMS_H

// enum MapType and enum TimeOfDay are real C types, so their members are
// invisible to the assembler. CrystalDust's phone scripts compare against them,
// so this header restates the values as preprocessor macros for assembly only.
// Keep in sync with include/constants/map_types.h and include/constants/rtc.h.
// See docs/port/decisions.md D47.

#define MAP_TYPE_NONE          0
#define MAP_TYPE_TOWN          1
#define MAP_TYPE_CITY          2
#define MAP_TYPE_ROUTE         3
#define MAP_TYPE_UNDERGROUND   4
#define MAP_TYPE_UNDERWATER    5
#define MAP_TYPE_OCEAN_ROUTE   6
#define MAP_TYPE_UNKNOWN       7
#define MAP_TYPE_INDOOR        8
#define MAP_TYPE_SECRET_BASE   9

#define TIME_MORNING           0
#define TIME_DAY               1
#define TIME_EVENING           2
#define TIME_NIGHT             3
#define TIMES_OF_DAY_COUNT     4

#endif // GUARD_CONSTANTS_ASM_ENUMS_H
