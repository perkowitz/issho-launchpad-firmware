#ifndef LAUNCHPAD_STEP_H
#define LAUNCHPAD_STEP_H

#include <stdbool.h>

/***** launchpad constants *****/
#define BUTTON_COUNT 100
#define ROW_COUNT 8
#define COLUMN_COUNT 8

/***** application constants *****/
#define APP_VERSION 1
#define INTERNAL 0
#define EXTERNAL 1
#define OUT_OF_RANGE 255
#define STAGE_COUNT 8
#define PATTERN_COUNT 8
#define MIDI_OUT_PORT USBMIDI

#define MIDI_ALL_NOTES_OFF_CC 120
#define MIDI_RESET_ALL_CONTROLLERS 121


#define BEATS_PER_MEASURE 4
#define TICKS_PER_BEAT 24
#define TICKS_PER_16TH (TICKS_PER_BEAT/4)

// stage modifiers
#define DEFAULT_OCTAVE 5
#define DEFAULT_VELOCITY 63
#define VELOCITY_DELTA 31

// button groups
#define TOP 0
#define BOTTOM 1
#define LEFT 2
#define RIGHT 3
#define GROUP_COUNT 4
#define OFFSET_COUNT 8

// markers
#define MARKER_GROUP BOTTOM
#define OFF_MARKER BLACK
#define NOTE_MARKER SKY_BLUE
#define SHARP_MARKER CYAN
#define FLAT_MARKER DIM_CYAN
#define OCTAVE_UP_MARKER ORANGE
#define OCTAVE_DOWN_MARKER DIM_ORANGE
#define VELOCITY_UP_MARKER GREEN
#define VELOCITY_DOWN_MARKER DIM_GREEN
#define EXTEND_MARKER DIM_BLUE
#define REPEAT_MARKER PINK
#define TIE_MARKER PURPLE
#define SKIP_MARKER RED
#define LEGATO_MARKER DARK_GRAY
#define RANDOM_MARKER YELLOW

// other element colors
#define BUTTON_OFF_COLOR DARK_GRAY
#define BUTTON_ON_COLOR WHITE
#define PANIC_BUTTON_OFF_COLOR DIM_RED
#define PANIC_BUTTON_ON_COLOR WHITE
#define RESET_BUTTON_OFF_COLOR DARK_GRAY
#define RESET_BUTTON_1_COLOR DIM_GREEN
#define RESET_BUTTON_2_COLOR GREEN
#define SONG_BUTTON_COLOR BLUE
#define SHUFFLE_BUTTON_COLOR ORANGE
#define FILL_BUTTON_COLOR YELLOW
#define PERF_BUTTON_ON_COLOR WHITE
#define PLAYING_NOTE_COLOR WHITE
#define PATTERN_COLOR_1 DIM_BLUE
#define PATTERN_COLOR_2 DIM_PURPLE
#define PATTERN_SELECTED_COLOR_1 SKY_BLUE
#define PATTERN_SELECTED_COLOR_2 MAGENTA
#define WATER_BUTTON_COLOR SKY_BLUE

// elements
#define PLAY_BUTTON 95
#define PANIC_BUTTON 92
#define SETTINGS_BUTTON 93
#define RESET_BUTTON 40
#define SONG_BUTTON 30
#define SHUFFLE_BUTTON 20
#define FILL_BUTTON 10
#define LOAD_BUTTON 97
#define CLEAR_BUTTON 98
#define WATER_BUTTON 94

#define GRID_COLUMNS (COLUMN_COUNT + 1)
#define PATTERN_MOD_GROUP RIGHT
#define PATTERN_MOD_COLUMN COLUMN_COUNT

#define SETTINGS_MIDI_ROW_1 1
#define SETTINGS_MIDI_ROW_2 0
#define SETTINGS_VERSION_ROW 5
#define SETTINGS_MISC_ROW 4
#define SETTINGS_AUTO_LOAD_COLUMN 0
#define SETTINGS_PATTERN_ROW 7
#define PATTERNS_GROUP LEFT
#define PATTERNS_OFFSET_LO 4
#define PATTERNS_OFFSET_HI 7

typedef struct Stage {
	s8 note_count;
	u8 note;
	s8 octave;
	s8 velocity;
	s8 accidental;
	s8 extend;
	s8 repeat;
	s8 tie;
	s8 legato;
	s8 skip;
	s8 random;
} Stage;

typedef struct Pattern {
	u8 reset;
	u8 grid[ROW_COUNT][COLUMN_COUNT + 1];
} Pattern;

typedef struct Settings {
	u8 version;
	u8 auto_load;
	u8 tempo;
	u8 clock_divide;
	u8 midi_channel;
	u8 midi_ports;
	u8 reserved1;
	u8 reserved2;
	u8 reserved3;
	u8 reserved4;
} Settings;

typedef struct Memory {
	Settings settings;
	Pattern patterns[PATTERN_COUNT];
} Memory;

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
