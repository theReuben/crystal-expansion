@ CrystalDust's Prof. Oak Pokedex rating, restored in Phase 2 (D43).
@ CrystalDust rated the Johto dex in 15-mon bands; expansion's file rated the
@ Kanto dex in 10s. CrystalDust wins, rewritten here in expansion's macro syntax.

PokedexRating_EventScript_ShowRatingMessage::
	copyvar VAR_0x8004, VAR_0x8009
	special ShowPokedexRatingMessage
	waitmessage
	call_if_eq VAR_RESULT, FALSE, PokedexRating_EventScript_NormalFanfare
	call_if_eq VAR_RESULT, TRUE, PokedexRating_EventScript_DexCompleteFanfare
	waitfanfare
	waitbuttonpress
	return

PokedexRating_EventScript_NormalFanfare::
	playfanfare MUS_RG_DEX_RATING
	return

PokedexRating_EventScript_DexCompleteFanfare::
	playfanfare MUS_EVOLVED
	return

PokedexRating_EventScript_RatePokedex::
	setvar VAR_0x8004, 0
	specialvar VAR_RESULT, ScriptGetPokedexInfo
	copyvar VAR_0x8008, VAR_0x8005
	copyvar VAR_0x8009, VAR_0x8006
	copyvar VAR_0x800A, VAR_RESULT
	buffernumberstring STR_VAR_1, VAR_0x8008 @ Num Johto seen
	buffernumberstring STR_VAR_2, VAR_0x8009 @ Num Johto caught
	msgbox gPokedexRatingText_SoYouveSeenAndCaught, MSGBOX_DEFAULT
	call PokedexRating_EventScript_ShowRatingMessage
	goto_if_eq VAR_0x800A, FALSE, Common_EventScript_NopReturn @ National dex not enabled
	setvar VAR_0x8004, 1
	specialvar VAR_RESULT, ScriptGetPokedexInfo
	copyvar VAR_0x8008, VAR_0x8005
	copyvar VAR_0x8009, VAR_0x8006
	buffernumberstring STR_VAR_1, VAR_0x8008 @ Num National seen
	buffernumberstring STR_VAR_2, VAR_0x8009 @ Num National caught
	msgbox gPokedexRatingText_OnANationwideBasis, MSGBOX_DEFAULT
	specialvar VAR_RESULT, HasAllMons
	goto_if_eq VAR_RESULT, FALSE, PokedexRating_EventScript_DexIncomplete
	goto_if_eq VAR_RESULT, TRUE, PokedexRating_EventScript_DexComplete
	return

PokedexRating_EventScript_DexIncomplete::
	msgbox PokedexRating_Text_LookForwardToFilledNationalDex, MSGBOX_DEFAULT
	return

PokedexRating_EventScript_DexComplete::
	setflag FLAG_OAK_SAW_DEX_COMPLETION
	msgbox PokedexRating_Text_YouveCompletedDex, MSGBOX_DEFAULT
	return
