#include "global.h"
#include "constants/trainers.h"

// D108: the player is Gold or Kris, not Brendan or May. GAME_VERSION is
// VERSION_EMERALD here, so this is the branch every ordinary battle takes;
// the Hoenn and Kanto pics below are only reachable through a linked save.
static enum TrainerPicID GetJohtoTrainerPic(enum Gender gender)
{
    return gender == MALE ? TRAINER_PIC_GOLD : TRAINER_PIC_KRIS;
}

static enum TrainerPicID GetEmeraldTrainerPic(enum Gender gender)
{
    return gender == MALE ? TRAINER_PIC_BRENDAN : TRAINER_PIC_MAY;
}
static enum TrainerPicID GetRSTrainerPic(enum Gender gender)
{
    return gender == MALE ? TRAINER_PIC_RS_BRENDAN : TRAINER_PIC_RS_MAY;
}

static enum TrainerPicID GetKantoTrainerPic(enum Gender gender)
{
    return gender == MALE ? TRAINER_PIC_RED : TRAINER_PIC_LEAF;
}

enum TrainerPicID GetPlayerTrainerPic(enum Gender gender, enum GameVersion version)
{
    switch (version)
    {
        case VERSION_SAPPHIRE:
        case VERSION_RUBY:
            return GetRSTrainerPic(gender);
        case VERSION_LEAF_GREEN:
        case VERSION_FIRE_RED:
            return GetKantoTrainerPic(gender);
        case VERSION_EMERALD:
        default:
            return GetJohtoTrainerPic(gender);
    }
}
