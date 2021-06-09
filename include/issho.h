#ifndef ISSHO_H
#define ISSHO_H

/***** launchpad constants *****/
#define BUTTON_COUNT 100
#define ROW_COUNT 8
#define COLUMN_COUNT 8
#define PORT_DIN 0
#define PORT_USB 1
#define PORT_STANDALONE 2

#define MIDI_ALL_NOTES_OFF_CC 120
#define MIDI_RESET_ALL_CONTROLLERS 121



/***** colors *****/
typedef struct Color {
	u8 red;
	u8 green;
	u8 blue;
} Color;

#define C_HI 63
#define C_MID 12
#define C_LO 2
#define PSIZE 25

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
#define GRAY_GREEN 23
#define DIM_GRAY_GREEN 24

void colors_init(Color palette[PSIZE]);

#endif
