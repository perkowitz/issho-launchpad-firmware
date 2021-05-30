#include "app.h"
#include "issho.h"

void colors_init(Color palette[PSIZE]) {

	palette[BLACK] = (Color){0, 0, 0};
	palette[DARK_GRAY] = (Color){C_LO, C_LO, C_LO};
	palette[WHITE] = (Color){C_HI, C_HI, C_HI};
	palette[GRAY] = (Color){C_MID, C_MID, C_MID};

	palette[RED] = (Color){C_HI, 0, 0};
	palette[ORANGE] = (Color){63, 20, 0};
	palette[YELLOW] = (Color){C_HI, C_HI, 0};
	palette[GREEN] = (Color){0, C_HI, 0};
	palette[CYAN] = (Color){0, C_HI, C_HI};
	palette[BLUE] = (Color){0, 0, C_HI};
	palette[PURPLE] = (Color){10, 0, 63};
	palette[MAGENTA] = (Color){C_HI, 0, C_HI};

	palette[DIM_RED] = (Color){C_MID, 0, 0};
	palette[DIM_ORANGE] = (Color){20, 8, 0};
	palette[DIM_YELLOW] = (Color){C_MID, C_MID, 0};
	palette[DIM_GREEN] = (Color){0, C_MID, 0};
	palette[DIM_CYAN] = (Color){0, C_MID, C_MID};
	palette[DIM_BLUE] = (Color){0, 0, C_MID};
	palette[DIM_PURPLE] = (Color){4, 0, 20};
	palette[DIM_MAGENTA] = (Color){C_MID, 0, C_MID};

	palette[SKY_BLUE] = (Color){8, 18, 63};
	palette[PINK] = (Color){32, 13, 22};
	palette[DIM_PINK] = (Color){16, 7, 11};
	palette[GRAY_GREEN] = (Color){18, 32, 22};
	palette[DIM_GRAY_GREEN] = (Color){10, 22, 14};

//	rainbow[0] = WHITE;
//	rainbow[1] = RED;
//	rainbow[2] = ORANGE;
//	rainbow[3] = YELLOW;
//	rainbow[4] = GREEN;
//	rainbow[5] = CYAN;
//	rainbow[6] = BLUE;
//	rainbow[7] = PURPLE;

}
