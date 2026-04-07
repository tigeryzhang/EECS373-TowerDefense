#include "game_types.h"

const char *pvz_get_plant_name(PlantType plant_type) {
	switch (plant_type) {
	case PLANT_SUNFLOWER:
		return "Sunflower";
	case PLANT_PEASHOOTER:
		return "Peashooter";
	case PLANT_WALLNUT:
		return "Wallnut";
	case PLANT_NONE:
	default:
		return "None";
	}
}

const char *pvz_get_zombie_name(ZombieType zombie_type) {
	switch (zombie_type) {
	case ZOMBIE_REGULAR:
		return "Regular";
	case ZOMBIE_CONE:
		return "Cone";
	case ZOMBIE_BUCKETHEAD:
		return "Bucket";
	case ZOMBIE_NONE:
	default:
		return "None";
	}
}
