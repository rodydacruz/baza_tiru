class_name Settings
extends RefCounted

const SCREEN_WIDTH = 320
const SCREEN_HEIGHT = 180



#Enemy Positions
const BOSS_ATACK_POSITION = SCREEN_WIDTH - 60
const BOSS_LEFT_HIDE_POSITION = -SCREEN_WIDTH
const BOSS_LEFF_ATACK_POSITION = 60


#Boss Faze
enum Phase {
	APEAR,
	HIDE,
	AlL_IN,
	DEFEATED
}
