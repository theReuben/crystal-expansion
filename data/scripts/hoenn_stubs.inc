@ Hoenn stubs (D48).
@
@ D37 removed CrystalDust's unused Hoenn maps, but pokeemerald-expansion's C
@ code and a handful of shared script files still name scripts and texts that
@ lived on those maps. The guards in front of them (Hoenn metatile behaviours,
@ Hoenn story flags, Hoenn-only local IDs) can never fire in a Johto game, so
@ these are inert placeholders that keep the link honest. If Hoenn content is
@ ever wanted back, delete this file and restore the real maps.

	LOCALID_LILYCOVE_LADY = 1
	LOCALID_LILYCOVE_LADYS_MON = 2
	LOCALID_RUSTURF_TUNNEL_WANDA = 1
	LOCALID_RUSTURF_TUNNEL_WANDAS_BF = 2

EventScript_ClosedSootopolisDoor::
SkyPillar_Outside_EventScript_ClosedDoor::
Route110_TrickHousePuzzle_EventScript_Door::
EventScript_RunningShoesManual::
IslandCave_EventScript_OpenRegiEntrance::
MauvilleCity_EventScript_RegisterWallyCall::
Route119_EventScript_ScottWonAtFortreeGymCall::
LittlerootTown_ProfessorBirchsLab_EventScript_ScottAboardSSTidalCall::
RustboroCity_Gym_EventScript_RegisterRoxanne::
MossdeepCity_SpaceCenter_2F_EventScript_RivalRayquazaCall::
SSTidalCorridor_EventScript_ReachedStepCount::
MauvilleCity_GameCorner_EventScript_NoCoinCase::
	end

MossdeepCity_SpaceCenter_2F_EventScript_ChoosePartyForMultiBattle::
	return

Kecleon_Text_SomethingUnseeable::
	.string "Something unseeable is in the way!$"

Kecleon_Text_WantToUseDevonScope::
	.string "Want to use the DEVON SCOPE?$"

Kecleon_Text_UseDevonScopeMonAttacked::
	.string "{PLAYER} used the DEVON SCOPE.\p$"

PlayersHouse_1F_Text_TheresAMovieOnTV::
	.string "There's a movie on TV.$"

PlayersHouse_1F_Text_LatiEmergencyNewsFlash::
	.string "A news flash is on TV.$"
