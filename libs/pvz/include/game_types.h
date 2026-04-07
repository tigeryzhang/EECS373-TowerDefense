#pragma once

#include <stdbool.h>

#define PVZ_NUM_PLANT_TYPES 3

typedef enum {
	PLANT_NONE = 0,
	PLANT_SUNFLOWER,
	PLANT_PEASHOOTER,
	PLANT_WALLNUT,
} PlantType;

typedef enum {
	ZOMBIE_NONE = 0,
	ZOMBIE_REGULAR,
	ZOMBIE_CONE,
	ZOMBIE_BUCKETHEAD,
} ZombieType;

typedef enum {
	GAME_STATUS_RUNNING = 0,
	GAME_STATUS_WON,
	GAME_STATUS_LOST,
} GameStatus;

typedef struct {
	int row;
	int col;
} BoardCoord;

typedef enum {
	GAME_COMMAND_NONE = 0,
	GAME_COMMAND_SELECT_PLANT,
	GAME_COMMAND_PLACE_PLANT,
	GAME_COMMAND_REMOVE_PLANT,
	GAME_COMMAND_TOGGLE_PAUSE,
	GAME_COMMAND_RESTART,
} GameCommandType;

typedef struct {
	GameCommandType type;
	PlantType plant_type;
	BoardCoord coord;
} GameCommand;

typedef enum {
	GAME_COMMAND_RESULT_OK = 0,
	GAME_COMMAND_RESULT_IGNORED,
	GAME_COMMAND_RESULT_OUT_OF_BOUNDS,
	GAME_COMMAND_RESULT_OCCUPIED,
	GAME_COMMAND_RESULT_NO_SELECTION,
	GAME_COMMAND_RESULT_NOT_ENOUGH_SUN,
	GAME_COMMAND_RESULT_PLANT_ON_COOLDOWN,
	GAME_COMMAND_RESULT_NOT_FOUND,
} GameCommandResult;

const char *pvz_get_plant_name(PlantType plant_type);
const char *pvz_get_zombie_name(ZombieType zombie_type);
