Route5_SouthEntrance_MapScripts:: @ 816F900
	.byte 0

Route5_SouthEntrance_EventScript_16F901:: @ 816F901
	msgbox gUnknown_819E16D, 2
	end

Route5_SouthEntrance_EventScript_16F90A:: @ 816F90A
	lockall
	setvar VAR_0x4001, 0
	goto EventScript_16F92E
	end

Route5_SouthEntrance_EventScript_16F916:: @ 816F916
	lockall
	setvar VAR_0x4001, 1
	goto EventScript_16F92E
	end

Route5_SouthEntrance_EventScript_16F922:: @ 816F922
	lockall
	setvar VAR_0x4001, 2
	goto EventScript_16F92E
	end

EventScript_16F92E:: @ 816F92E
	textcolor 0
	applymovement 255, Movement_1A75E7
	waitmovement 0
	checkflag FLAG_0x2A6
	goto_if_eq EventScript_16F958
	msgbox gUnknown_819E04B
	closemessage
	applymovement 255, Movement_16F9C4
	waitmovement 0
	releaseall
	end

EventScript_16F958:: @ 816F958
	removeitem ITEM_TEA, 1
	goto EventScript_16F963
	end

EventScript_16F963:: @ 816F963
	msgbox gUnknown_819E09B
	closemessage
	compare_var_to_value VAR_0x4001, 0
	call_if 1, EventScript_16F99C
	compare_var_to_value VAR_0x4001, 1
	call_if 1, EventScript_16F9A7
	compare_var_to_value VAR_0x4001, 2
	call_if 1, EventScript_16F9B2
	msgbox gUnknown_819E0C1
	setvar VAR_0x4062, 1
	releaseall
	end

EventScript_16F99C:: @ 816F99C
	applymovement 255, Movement_16F9C2
	waitmovement 0
	return

EventScript_16F9A7:: @ 816F9A7
	applymovement 255, Movement_16F9BD
	waitmovement 0
	return

EventScript_16F9B2:: @ 816F9B2
	applymovement 255, Movement_16F9BF
	waitmovement 0
	return

Movement_16F9BD:: @ 816F9BD
	step_12
	step_end

Movement_16F9BF:: @ 816F9BF
	step_12
	step_12
	step_end

Movement_16F9C2:: @ 816F9C2
	step_27
	step_end

Movement_16F9C4:: @ 816F9C4
	step_11
	step_end
