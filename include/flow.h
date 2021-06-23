//#ifndef FLOW_H
//#define FLOW_H
#ifndef LAUNCHPAD_STEP_H
#define LAUNCHPAD_STEP_H

#include <stdbool.h>
#include "issho.h"

/***** application constants *****/
#define FLOW_APP_ID = 0
#define APP_VERSION 4
#define STAGE_COUNT 8
#define PATTERN_COUNT 8
#define FLOW_LENGTH 8

// stage modifiers
#define DEFAULT_OCTAVE 5
#define DEFAULT_VELOCITY 63
#define VELOCITY_DELTA 31

// markers
#define MARKER_GROUP BOTTOM
#define OFF_MARKER BLACK
#define NOTE_MARKER SKY_BLUE
#define SHARP_MARKER CYAN
#define FLAT_MARKER DIM_CYAN
//#define OCTAVE_UP_MARKER GRAY_GREEN
//#define OCTAVE_DOWN_MARKER DIM_GRAY_GREEN
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
#define RESET_BUTTON_OFF_COLOR DARK_GRAY
#define RESET_BUTTON_1_COLOR DIM_BLUE
#define RESET_BUTTON_2_COLOR SKY_BLUE
#define LENGTH_BUTTON_1_COLOR DARK_GRAY
#define LENGTH_BUTTON_2_COLOR DIM_BLUE
#define LENGTH_BUTTON_3_COLOR SKY_BLUE
#define LENGTH_BUTTON_4_COLOR WHITE
#define SONG_BUTTON_COLOR DIM_BLUE
#define SHUFFLE_BUTTON_COLOR DIM_BLUE
#define JUMP_BUTTON_COLOR DIM_BLUE
#define FLOW_BUTTON_COLOR SKY_BLUE
#define PERF_BUTTON_ON_COLOR WHITE
#define PLAYING_NOTE_COLOR WHITE
#define PATTERN_COLOR_1 DIM_BLUE
#define PATTERN_COLOR_2 DIM_PURPLE
#define PATTERN_SELECTED_COLOR_1 SKY_BLUE
#define PATTERN_SELECTED_COLOR_2 MAGENTA
#define WATER_BUTTON_COLOR SKY_BLUE
#define STATUS_LIGHT SKY_BLUE
#define STATUS_LIGHT_DIM DIM_BLUE
#define FLOW1_0_COLOR DARK_GRAY
#define FLOW1_1_COLOR PATTERN_COLOR_1
#define FLOW1_2_COLOR PATTERN_SELECTED_COLOR_1
#define FLOW1_3_COLOR WHITE
#define FLOW2_0_COLOR DARK_GRAY
#define FLOW2_1_COLOR PATTERN_COLOR_2
#define FLOW2_2_COLOR PATTERN_SELECTED_COLOR_2
#define FLOW2_3_COLOR WHITE

// top buttons
#define WATER_BUTTON 94
#define RESET_BUTTON 95
#define LENGTH_BUTTON 96

// left buttons
#define JUMP_BUTTON 10
#define SHUFFLE_BUTTON 20
#define ARP_BUTTON 30
#define FLOW_BUTTON 40

#define GRID_COLUMNS (COLUMN_COUNT + 1)
#define PATTERN_MOD_GROUP RIGHT
#define PATTERN_MOD_COLUMN COLUMN_COUNT

#define SETTINGS_PATTERN_ROW 7
#define SETTINGS_FLOW1_ROW 6
#define SETTINGS_FLOW2_ROW 5
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

typedef struct StageOrder {
	u8 order[STAGE_COUNT];
} StageOrder;

typedef struct Pattern {
	u8 reset;
	u8 grid[ROW_COUNT][COLUMN_COUNT + 1];
} Pattern;

typedef struct Settings {
	u8 app_id;
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
	u8 flow1[FLOW_LENGTH];
	u8 flow2[FLOW_LENGTH];
	Pattern patterns[PATTERN_COUNT];
} Memory;



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
