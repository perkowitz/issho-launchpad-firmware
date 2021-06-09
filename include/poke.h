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

// game modes
#define TWO_PLAYER_ALL 0
#define TWO_PLAYER_HALF 1



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

