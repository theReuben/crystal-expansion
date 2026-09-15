@ The wall clock scripts, kept from Emerald's players_house.inc when the rest of
@ that file went with the Hoenn maps in Phase 2 (D48). src/debug.c calls both.
PlayersHouse_2F_EventScript_CheckWallClock::
	incrementgamestat GAME_STAT_CHECKED_CLOCK
	fadescreen FADE_TO_BLACK
	special Special_ViewWallClock
	releaseall
	end

PlayersHouse_2F_EventScript_SetWallClock::
	fadescreen FADE_TO_BLACK
	special StartWallClock
	return
