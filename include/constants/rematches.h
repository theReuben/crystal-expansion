#ifndef GUARD_REMATCHES_H
#define GUARD_REMATCHES_H

// These were an anonymous enum; CrystalDust's phone scripts compare against
// REMATCH values in assembly, where enum members are invisible (D47).
#define REMATCH_ROSE                       0
#define REMATCH_ANDRES                     1
#define REMATCH_DUSTY                      2
#define REMATCH_LOLA                       3
#define REMATCH_RICKY                      4
#define REMATCH_LILA_AND_ROY               5
#define REMATCH_CRISTIN                    6
#define REMATCH_BROOKE                     7
#define REMATCH_WILTON                     8
#define REMATCH_VALERIE                    9
#define REMATCH_CINDY                      10
#define REMATCH_THALIA                     11
#define REMATCH_JESSICA                    12
#define REMATCH_WINSTON                    13
#define REMATCH_STEVE                      14
#define REMATCH_TONY                       15
#define REMATCH_NOB                        16
#define REMATCH_KOJI                       17
#define REMATCH_FERNANDO                   18
#define REMATCH_DALTON                     19
#define REMATCH_BERNIE                     20
#define REMATCH_ETHAN                      21
#define REMATCH_JOHN_AND_JAY               22
#define REMATCH_JEFFREY                    23
#define REMATCH_CAMERON                    24
#define REMATCH_JACKI                      25
#define REMATCH_WALTER                     26
#define REMATCH_KAREN                      27
#define REMATCH_JERRY                      28
#define REMATCH_ANNA_AND_MEG               29
#define REMATCH_ISABEL                     30
#define REMATCH_MIGUEL                     31
#define REMATCH_TIMOTHY                    32
#define REMATCH_SHELBY                     33
#define REMATCH_CALVIN                     34
#define REMATCH_ELLIOT                     35
#define REMATCH_ISAIAH                     36
#define REMATCH_MARIA                      37
#define REMATCH_ABIGAIL                    38
#define REMATCH_DYLAN                      39
#define REMATCH_KATELYN                    40
#define REMATCH_BENJAMIN                   41
#define REMATCH_PABLO                      42
#define REMATCH_NICOLAS                    43
#define REMATCH_ROBERT                     44
#define REMATCH_LAO                        45
#define REMATCH_CYNDY                      46
#define REMATCH_MADELINE                   47
#define REMATCH_JENNY                      48
#define REMATCH_DIANA                      49
#define REMATCH_AMY_AND_LIV                50
#define REMATCH_ERNEST                     51
#define REMATCH_CORY                       52
#define REMATCH_EDWIN                      53
#define REMATCH_LYDIA                      54
#define REMATCH_ISAAC                      55
#define REMATCH_GABRIELLE                  56
#define REMATCH_CATHERINE                  57
#define REMATCH_JACKSON                    58
#define REMATCH_HALEY                      59
#define REMATCH_JAMES                      60
#define REMATCH_TRENT                      61
#define REMATCH_SAWYER                     62
#define REMATCH_KIRA_AND_DAN               63
    // CrystalDust phone-rematch trainers (D19). Placed before
    // REMATCH_SPECIAL_TRAINER_START so they count as normal trainers.
#define REMATCH_JOEY                       64
#define REMATCH_WADE                       65
#define REMATCH_LIZ                        66
#define REMATCH_RALPH                      67
#define REMATCH_ANTHONY                    68
#define REMATCH_TODD                       69
#define REMATCH_GINA                       70
#define REMATCH_ARNIE                      71
#define REMATCH_JACK                       72
#define REMATCH_ALAN                       73
#define REMATCH_DANA                       74
#define REMATCH_CHAD                       75
#define REMATCH_HUEY                       76
#define REMATCH_TULLY                      77
#define REMATCH_TIFFANY                    78
#define REMATCH_BRENT                      79
#define REMATCH_WILTON_GSC                 80
#define REMATCH_VANCE                      81
#define REMATCH_PARRY                      82
#define REMATCH_ERIN                       83
#define REMATCH_JOSE                       84
#define REMATCH_REENA                      85
#define REMATCH_GAVEN                      86
#define REMATCH_BETH                       87
#define REMATCH_WALLY_VR                   88 // Entries above WALLY are considered normal trainers, from Wally below are special trainers
#define REMATCH_ROXANNE                    89
#define REMATCH_BRAWLY                     90
#define REMATCH_WATTSON                    91
#define REMATCH_FLANNERY                   92
#define REMATCH_NORMAN                     93
#define REMATCH_WINONA                     94
#define REMATCH_TATE_AND_LIZA              95
#define REMATCH_JUAN                       96
#define REMATCH_SIDNEY                     97 // Entries from SIDNEY below are considered part of REMATCH_ELITE_FOUR_ENTRIES.
#define REMATCH_PHOEBE                     98
#define REMATCH_GLACIA                     99
#define REMATCH_DRAKE                      100
#define REMATCH_WALLACE                    101
#define REMATCH_TABLE_ENTRIES              102 // The total number of rematch entries. Must be last in enum


#define REMATCH_SPECIAL_TRAINER_START   REMATCH_WALLY_VR
#define REMATCH_ELITE_FOUR_ENTRIES      REMATCH_SIDNEY

#endif // GUARD_REMATCHES_H
