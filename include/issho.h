#ifndef ISSHO_H
#define ISSHO_H

#include <stdbool.h>


/***** launchpad constants *****/
#define BUTTON_COUNT 100
#define ROW_COUNT 8
#define COLUMN_COUNT 8

/***** MIDI *****/
#define MIDI_ALL_NOTES_OFF_CC 120
#define MIDI_RESET_ALL_CONTROLLERS 121
#define PORT_DIN 0
#define PORT_USB 1
#define PORT_STANDALONE 2

/***** application constants *****/
#define INTERNAL 0
#define EXTERNAL 1
#define OUT_OF_RANGE 255
#define LONG_PRESS_MILLIS 2000
#define MIDI_USB_MASK 1
#define MIDI_DIN_MASK 2
#define MIDI_STANDALONE_MASK 4

/***** timing constants *****/
#define BEATS_PER_MEASURE 4
#define TICKS_PER_BEAT 24
#define TICKS_PER_16TH (TICKS_PER_BEAT/4)

// button groups
#define TOP 0
#define BOTTOM 1
#define LEFT 2
#define RIGHT 3
#define GROUP_COUNT 4
#define OFFSET_COUNT 8

// settings
#define SETTINGS_MIDI_ROW_1 1
#define SETTINGS_MIDI_ROW_2 0
#define SETTINGS_VERSION_ROW 3
#define SETTINGS_MISC_ROW 2
#define SETTINGS_AUTO_LOAD_COLUMN 0
#define SETTINGS_MIDI_DIN_COLUMN 1
#define SETTINGS_MIDI_USB_COLUMN 2
#define SETTINGS_MIDI_STANDALONE_COLUMN 3
#define SETTINGS_AUTOLOAD_COLOR DARK_GRAY
#define SETTINGS_MIDI_PORT_COLOR DIM_BLUE

// top buttons
#define PLAY_BUTTON 91
#define PANIC_BUTTON 92
#define SETTINGS_BUTTON 93
#define LOAD_BUTTON 97
#define CLEAR_BUTTON 98


/***** colors *****/
typedef struct Color {
	u8 red;
	u8 green;
	u8 blue;
} Color;

#define C_HI 63
#define C_MID 12
#define C_LO 2
#define PSIZE 27

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
#define BROWN 25
#define GRAY_BROWN 26

void colors_init();


/***** standard color stuff *****/
#define BUTTON_OFF_COLOR DARK_GRAY
#define BUTTON_ON_COLOR WHITE
#define PANIC_BUTTON_OFF_COLOR DIM_RED
#define PANIC_BUTTON_ON_COLOR WHITE
#define LOAD_BUTTON_COLOR DARK_GRAY
#define CLEAR_BUTTON_COLOR DARK_GRAY


/***** lighting up stuff *****/
void plot_led(u8 type, u8 index, Color color);
void plot_by_index(u8 index, Color color);
void status_light(Color color);
void draw_by_index(u8 index, u8 palette_index);
void status(u8 palette_index);


/***** midi *****/
bool get_port(u8 index);
void set_port(u8 index, bool enabled);
void flip_port(u8 index);
void send_midi(u8 status, u8 data1, u8 data2);
void midi_note(u8 channel, u8 note, u8 velocity);
void all_notes_off(u8 channel);


/***** pads: functions for setting colors on the central hardware grid *****/
u8 pad_index(u8 row, u8 column);
u8 index_to_row_column(u8 index, u8 *row, u8 *column);
void draw_pad(u8 row, u8 column, u8 c);


/***** buttons: functions for setting colors for the round buttons on each side *****/
u8 button_index(u8 group, u8 offset);
u8 index_to_group_offset(u8 index, u8 *group, u8 *offset);
void draw_button(u8 group, u8 offset, u8 value);
void set_and_draw_button(u8 group, u8 offset, u8 value);
u8 get_button(u8 group, u8 offset);


/***** draw *****/
void clear_pads();
void clear_buttons();
void draw_binary_row(u8 row, u8 value);



#endif
