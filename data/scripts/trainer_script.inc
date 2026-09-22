@ Crystal Expansion (D114): this is CrystalDust's version. Emerald's overwrote
@ STR_VAR_1 with buffertrainerclassname/buffertrainername keyed off VAR_0x8000,
@ which Johto's registerphonecontact never sets -- so the phone message printed a
@ garbage trainer class and name over the contact name that FindPhoneContactNameFromFlag
@ had already put in STR_VAR_1.
Std_RegisteredInMatchCall::
	waitse
	playfanfare MUS_REGISTER_PHONE
	textcolor MSG_COLOR_SYS
	msgbox EventScript_Text_Trainer_Registered, MSGBOX_DEFAULT
	waitfanfare
	closemessage
	textcolor MSG_COLOR_PREV
	return

EventScript_Text_Trainer_Registered:
	.string "{PLAYER} registered\n"
	.string "{STR_VAR_1}'s phone number!$"

EventScript_TryGetTrainerScript::
	special ShouldTryGetTrainerScript
	goto_if_eq VAR_RESULT, TRUE, EventScript_GotoTrainerScript
	releaseall
	end

EventScript_GotoTrainerScript::
	gotobeatenscript
	releaseall
	end

EventScript_ObjectApproachPlayer::
	lock
	special DoTrainerApproach
	gotonative LoadTrainerObjectScript
