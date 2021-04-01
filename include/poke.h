#ifndef LAUNCHPAD_STEP_H
#define LAUNCHPAD_STEP_H

#include <stdbool.h>

/***** launchpad constants *****/
#define BUTTON_COUNT 100
#define ROW_COUNT 8
#define COLUMN_COUNT 8

/***** application constants *****/
#define APP_VERSION 1
#define OUT_OF_RANGE 255
#define LONG_PRESS_MILLIS 2000
#define TICK_INTERVAL 125
#define BLINK_INTERVAL 125

// button groups
#define TOP 0
#define BOTTOM 1
#define LEFT 2
#define RIGHT 3
#define GROUP_COUNT 4
#define OFFSET_COUNT 8

// elements
#define PLAY_BUTTON 91

// game stages
#define START 0
#define PLAYING 1
#define SCORING 2

// game modes
#define TWO_PLAYER_ALL 0
#define TWO_PLAYER_HALF 1


/***** colors *****/

typedef struct Color {
	u8 red;
	u8 green;
	u8 blue;
} Color;

#define C_HI 63
#define C_MID 12
#define C_LO 2
#define PSIZE 23

#define BLACK 0
#define DARK_GRAY 1
#define GRAY 2
#define WHITE 3
#define RED 4
#define ORANGE 5
#define YELLOW 6
#define GREEN 7
#define CYAN 8
#define BLUE 9
#define PURPLE 10
#define MAGENTA 11
#define DIM_RED 12
#define DIM_ORANGE 13
#define DIM_YELLOW 14
#define DIM_GREEN 15
#define DIM_CYAN 16
#define DIM_BLUE 17
#define DIM_PURPLE 18
#define DIM_MAGENTA 19
#define SKY_BLUE 20
#define PINK 21
#define DIM_PINK 22


/***** function declarations *****/

// buttons
bool is_button(u8 group, u8 offset);
u8 button_index(u8 group, u8 offset);

// pads
u8 pad_index(u8 row, u8 column);
bool is_pad(u8 row, u8 column);

// drawing
void plot_button(u8 group, u8 offset, u8 c);




#endif

