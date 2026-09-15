#ifndef GUARD_CONSTANTS_TRADE_H
#define GUARD_CONSTANTS_TRADE_H

#define TRADE_PLAYER  0
#define TRADE_PARTNER 1

// In-game Trade IDs
enum InGameTradeID
{
    // CrystalDust's Johto trades, restored in Phase 2 (D41). Expansion's
    // Hoenn/FRLG ids went with the maps that used them.
    INGAME_TRADE_ONIX,
    INGAME_TRADE_MACHOP,
    INGAME_TRADE_MEOWTH,
    INGAME_TRADE_VOLTORB,
    INGAME_TRADE_DODRIO,
    INGAME_TRADE_AERODACTYL,
    INGAME_TRADE_XATU,
    INGAME_TRADE_MAGNETON,
    // Gift mons: given outright, with the level in requestedSpecies.
    INGAME_TRADE_GIFT_SPEAROW,
    INGAME_TRADE_GIFT_SHUCKLE,
};

// Return values for CheckForGiftMonAndTakeMail
#define GIFTMON_MATCH       0
#define GIFTMON_WRONG_MON   1
#define GIFTMON_NO_MAIL     2
#define GIFTMON_WRONG_MAIL  3
#define GIFTMON_LAST_MON    4
// Return values for CanTradeSelectedMon and CanSpinTradeMon
enum CanTradeMon
{
    CAN_TRADE_MON,
    CANT_TRADE_LAST_MON,
    CANT_TRADE_NATIONAL,
    CANT_TRADE_EGG_YET,
    CANT_TRADE_INVALID_MON,
    CANT_TRADE_PARTNER_EGG_YET
};

// Return values for CheckValidityOfTradeMons
#define PLAYER_MON_INVALID   0
#define BOTH_MONS_VALID      1
#define PARTNER_MON_INVALID  2

// Return values for GetGameProgressForLinkTrade
#define TRADE_BOTH_PLAYERS_READY      0
#define TRADE_PLAYER_NOT_READY        1
#define TRADE_PARTNER_NOT_READY       2

// Message indexes for sUnionRoomTradeMessages
#define UR_TRADE_MSG_NONE                         0
#define UR_TRADE_MSG_NOT_MON_PARTNER_WANTS        1
#define UR_TRADE_MSG_NOT_EGG                      2
#define UR_TRADE_MSG_MON_CANT_BE_TRADED_NOW       3
#define UR_TRADE_MSG_MON_CANT_BE_TRADED           4
#define UR_TRADE_MSG_PARTNERS_MON_CANT_BE_TRADED  5
#define UR_TRADE_MSG_EGG_CANT_BE_TRADED           6
#define UR_TRADE_MSG_PARTNER_CANT_ACCEPT_MON      7
#define UR_TRADE_MSG_CANT_TRADE_WITH_PARTNER_1    8
#define UR_TRADE_MSG_CANT_TRADE_WITH_PARTNER_2    9

// Return values for CanRegisterMonForTradingBoard
#define CAN_REGISTER_MON      0
#define CANT_REGISTER_MON_NOW 1
#define CANT_REGISTER_MON     2
#define CANT_REGISTER_EGG     3


#endif //GUARD_CONSTANTS_TRADE_H
