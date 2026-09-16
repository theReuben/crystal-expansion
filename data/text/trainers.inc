@ Crystal Expansion (D77): this file held 1145 Emerald trainer-dialogue labels
@ for Hoenn routes and gyms. Only the three VS Seeker strings below were still
@ referenced from anywhere in the tree -- every Johto trainer's lines live in
@ its own map's scripts.pory -- and the rest was emitted into the ROM regardless
@ of whether anything pointed at it. Roughly 130 KB of dead text removed.

VSSeeker_Text_BatteryNotChargedNeedXSteps::
	.string "The battery isn't charged enough.\p"
	.string "No. of steps required to fully\n"
	.string "charge the battery: {STR_VAR_1}{PAUSE_UNTIL_PRESS}$"

VSSeeker_Text_NoTrainersWithinRange::
	.string "There are no Trainers within range\n"
	.string "who can battle…\p"
	.string "The VS Seeker was turned off.{PAUSE_UNTIL_PRESS}$"

VSSeeker_Text_TrainersNotReady::
	.string "The other Trainers don't appear\n"
	.string "to be ready for battle.\p"
	.string "Let's wait till later.{PAUSE_UNTIL_PRESS}$"
