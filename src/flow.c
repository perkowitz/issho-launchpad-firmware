/******************************************************************************
 
 Copyright (c) 2015, Focusrite Audio Engineering Ltd.
 All rights reserved.
 
 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:
 
 * Redistributions of source code must retain the above copyright notice, this
 list of conditions and the following disclaimer.
 
 * Redistributions in binary form must reproduce the above copyright notice,
 this list of conditions and the following disclaimer in the documentation
 and/or other materials provided with the distribution.
 
 * Neither the name of Focusrite Audio Engineering Ltd., nor the names of its
 contributors may be used to endorse or promote products derived from
 this software without specific prior written permission.
 
 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 
 *****************************************************************************/

//______________________________________________________________________________
//
// Headers
//______________________________________________________________________________

#include "app.h"
#include "issho.h"
#include "flow.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>


//#define DEBUG 1

/***** global variables *****/
static const u16 *g_ADC = 0;   // ADC frame pointer
static u8 hw_buttons[BUTTON_COUNT] = {0};
static u8 clock_source = INTERNAL;
static Stage stages[GRID_COLUMNS];
static Color palette[PSIZE];
//static u8 rainbow[8];
const u8 note_map[8] = { 0, 2, 4, 5, 7, 9, 11, 12 };  // maps the major scale to note intervals
const u8 marker_map[8] = { OFF_MARKER, NOTE_MARKER, SHARP_MARKER, OCTAVE_UP_MARKER,
		VELOCITY_UP_MARKER, EXTEND_MARKER, TIE_MARKER, LEGATO_MARKER };

#define RANDOM_MARKER_COUNT 10
const u8 random_markers[RANDOM_MARKER_COUNT] = {
		OFF_MARKER,
		OCTAVE_UP_MARKER, OCTAVE_DOWN_MARKER,
		VELOCITY_UP_MARKER, VELOCITY_DOWN_MARKER,
		EXTEND_MARKER, REPEAT_MARKER,
		TIE_MARKER, SKIP_MARKER,
		LEGATO_MARKER
};

static unsigned long app_clock = 0;

static Memory memory;
static u8 c_pattern = 0;
static u8 c_pattern_group = 0;

static u8 warning_level = 0;
static u8 warning_blink = 0;

static bool is_running = false;
static bool in_settings = false;
static bool flow_on = false;
static bool shuffle_on = false;
static bool pattern_shuffles[PATTERN_COUNT] = { false, false, false, false, false, false, false, false };
static bool jump_on = false;
static bool water_on = false;
static u8 current_marker = OFF_MARKER;
static u8 current_marker_index = 0;
static u8 current_note = OUT_OF_RANGE;
static u8 current_note_column;
static u8 current_note_row;
static u8 reset = 1;
static u8 length = 1;

static u8 pressed_button_index = OUT_OF_RANGE;
static unsigned long pressed_button_time = 0;

static StageOrder stage_orders[PATTERN_COUNT];
static u8 c_stage = 0;
static u8 c_repeat = 0;
static u8 c_extend = 0;

static int c_measure = 0;
static u8 c_beat = 0;
static u8 c_tick = 0;

static u8 flow_index = 0;
static u8 flow1[FLOW_LENGTH] = { 0, 1, 0, 2, 0, 1, 0, 3 };
static u8 flow2[FLOW_LENGTH] = { 0, 1, 2, 3, 0, 1, 2, 3 };
static u8 flow1_colors[4] = { FLOW1_0_COLOR, FLOW1_1_COLOR, FLOW1_2_COLOR, FLOW1_3_COLOR };
static u8 flow2_colors[4] = { FLOW2_0_COLOR, FLOW2_1_COLOR, FLOW2_2_COLOR, FLOW2_3_COLOR };


/***** helper functions *****/

#define send_midi(s, d1, d2)    (hal_send_midi(MIDI_OUT_PORT, (s), (d1), (d2)))

/**
 * plot_led lights up a hardware button with a color.
 */
void plot_led(u8 type, u8 index, Color color) {
	if (index >= 0 && index < BUTTON_COUNT) {
		hal_plot_led(type, index, color.red, color.green, color.blue);
	}
}

void draw_by_index(u8 index, u8 c) {
	if (index >= 0 && index < BUTTON_COUNT && c >= 0 && c < PSIZE) {
		Color color = palette[c];
		hal_plot_led(TYPEPAD, index, color.red, color.green, color.blue);
	}
}


void status_light(u8 palette_index) {
	if (palette_index < PSIZE) {
		plot_led(TYPESETUP, 0, palette[palette_index]);
	}
}

void warning(u8 level) {
	status_light(level);
    warning_level = level;
    warning_blink = 0;
}

#ifdef DEBUG
void debug(u8 index, u8 level) {
	if (level != OUT_OF_RANGE) {
		plot_led(TYPEPAD, (index + 1) * 10 + 9, palette[level]);
	}
}
#endif

/***** midi *****/


void midi_note(u8 channel, u8 note, u8 velocity) {
	send_midi(NOTEON | channel, note, velocity);
}

void all_notes_off(u8 channel) {
	send_midi(CC | channel, MIDI_ALL_NOTES_OFF_CC, 0);
	send_midi(CC | channel, MIDI_RESET_ALL_CONTROLLERS, 0);
}


/***** pads: functions for setting colors on the central hardware grid *****/

/**
 * pad_index computes the button index from the row and column.
 */
u8 pad_index(u8 row, u8 column) {
	if (row >= 0 && row < ROW_COUNT && column >= 0 && column < COLUMN_COUNT) {
		return (row + 1) * 10 + column + 1;
	} else {
		return OUT_OF_RANGE;
	}
}

/**
 * index_to_row_column computes the pad row & column from an index.
 * Sets values by reference, returns "0" if successful, OUT_OF_RANGE otherwise.
 */
u8 index_to_row_column(u8 index, u8 *row, u8 *column) {
	*row = index / 10 - 1;
	*column = index % 10 - 1;
	if (*column >= 0 && *column < COLUMN_COUNT && *row >= 0 && *row < ROW_COUNT) {
		return 0;
	}
	return OUT_OF_RANGE;
}

bool is_pad(u8 row, u8 column) {
	return row != OUT_OF_RANGE && column != OUT_OF_RANGE;
}

/**
 * draw_pad lights up the pad with the indexed color.
 */
void draw_pad(u8 row, u8 column, u8 c) {
	u8 index = pad_index(row, column);
	if (index != OUT_OF_RANGE) {
		draw_by_index(index, c);
	}
}


/***** buttons: functions for setting colors for the round buttons on each side *****/

/**
 * button_index computes the index from the given group and offset.
 */
u8 button_index(u8 group, u8 offset) {
	if (group >= 0 && group < GROUP_COUNT && offset >= 0 && offset < OFFSET_COUNT) {
		switch (group) {
			case TOP:
				return 91 + offset;
			case BOTTOM:
				return 1 + offset;
			case LEFT:
				return (offset + 1) * 10;
			case RIGHT:
				return (offset + 1) * 10 + 9;
		}
	}
	return OUT_OF_RANGE;
}

/**
 * index_to_group_offset computes the group and offset from the index.
 * Sets values by reference; returns "0" of successful, OUT_OF_RANGE otherwise.
 */
u8 index_to_group_offset(u8 index, u8 *group, u8 *offset) {
	u8 r = index / 10;
	u8 c = index % 10;
	if (r == 0) {
		*group = BOTTOM;
	} else if (r == 9) {
		*group = TOP;
	} else if (c == 0) {
		*group = LEFT;
	} else if (c == 9) {
		*group = RIGHT;
	} else {
		return OUT_OF_RANGE;
	}

	if (*group == TOP || *group == BOTTOM) {
		*offset = (index % 10) - 1;
		return 0;
	}
	if (*group == LEFT || *group == RIGHT) {
		*offset = (index / 10) - 1;
		return 0;
	}
	return OUT_OF_RANGE;
}

void draw_button(u8 group, u8 offset, u8 value) {
	u8 index = button_index(group, offset);
	if (index != OUT_OF_RANGE) {
		draw_by_index(index, value);
	}
}

void set_and_draw_button(u8 group, u8 offset, u8 value) {
	u8 index = button_index(group, offset);
	if (index != OUT_OF_RANGE) {
		hw_buttons[index] = value;
		draw_by_index(index, value);
	}
}

u8 get_button(u8 group, u8 offset) {
	u8 index = button_index(group, offset);
	if (index != OUT_OF_RANGE) {
		return hw_buttons[index];
	}
	return OUT_OF_RANGE;
}


/***** patterns & grids *****/

void set_pattern_grid(u8 p_index, u8 row, u8 column, u8 value) {
	if (p_index >= 0 && p_index < PATTERN_COUNT &&
			row >= 0 && row < ROW_COUNT &&
			column >= 0 && column < GRID_COLUMNS) {
		memory.patterns[p_index].grid[row][column] = value;
	}
}

u8 get_pattern_grid(u8 p_index, u8 row, u8 column) {
	if (p_index >= 0 && p_index < PATTERN_COUNT &&
			row >= 0 && row < ROW_COUNT &&
			column >= 0 && column < GRID_COLUMNS) {
		return memory.patterns[p_index].grid[row][column];
	} else {
		return OUT_OF_RANGE;
	}
}

void set_grid(u8 row, u8 column, u8 value) {
	set_pattern_grid(c_pattern, row, column, value);
}

//void set_and_draw_grid(u8 row, u8 column, u8 value) {
//	set_pattern_grid(c_pattern, row, column, value);
//	if (column == PATTERN_MOD_COLUMN) {
//		draw_button(PATTERN_MOD_GROUP, row, value);
//	} else {
//		draw_pad(row, column, value);
//	}
//}
//
//u8 get_grid(u8 row, u8 column) {
//	return get_pattern_grid(c_pattern, row, column);
//}


/***** stages *****/

void reset_stage_orders() {
	for (u8 p = 0; p < PATTERN_COUNT; p++) {
		pattern_shuffles[p] = false;
		for (u8 s = 0; s < STAGE_COUNT; s++) {
			stage_orders[p].order[s] = s;
		}
	}
}

u8 stage_index(u8 index) {
	return stage_orders[c_pattern].order[index];
}

/***** draw *****/

void clear_pads() {
	for (int row = 0; row < ROW_COUNT; row++) {
		for (int column = 0; column < COLUMN_COUNT; column++) {
			draw_pad(row, column, BLACK);
		}
	}
}

void clear_buttons() {
	for (u8 group = 0; group < 4; group++) {
		for (u8 offset = 0; offset < 8; offset++) {
			draw_button(group, offset, BLACK);
		}
	}
}

void draw_binary_row(u8 row, u8 value) {
	u8 nd = 1;
	for (int column = 0; column < 8; column++) {
		draw_pad(row, 7 - column, value & nd ? WHITE : DIM_BLUE);
		nd = nd << 1;
	}
}

void draw_markers() {
	if (jump_on) {
		for (u8 offset = 0; offset < OFFSET_COUNT; offset++) {
			set_and_draw_button(MARKER_GROUP, offset, OFF_MARKER);
		}
	} else {
		set_and_draw_button(MARKER_GROUP, 0, OFF_MARKER);
		set_and_draw_button(MARKER_GROUP, 1, NOTE_MARKER);
		set_and_draw_button(MARKER_GROUP, 2, SHARP_MARKER);
		set_and_draw_button(MARKER_GROUP, 3, OCTAVE_UP_MARKER);
		set_and_draw_button(MARKER_GROUP, 4, VELOCITY_UP_MARKER);
		set_and_draw_button(MARKER_GROUP, 5, EXTEND_MARKER);
		set_and_draw_button(MARKER_GROUP, 6, TIE_MARKER);
		set_and_draw_button(MARKER_GROUP, 7, LEGATO_MARKER);
	}
}

void draw_function_button(u8 button_index) {
	u8 c;

	switch (button_index) {
		case PLAY_BUTTON:
			draw_by_index(button_index, is_running ? BUTTON_ON_COLOR : BUTTON_OFF_COLOR);
			break;
		case PANIC_BUTTON:
			draw_by_index(button_index, PANIC_BUTTON_OFF_COLOR);
			break;
		case SETTINGS_BUTTON:
			draw_by_index(button_index, in_settings ? BUTTON_ON_COLOR : BUTTON_OFF_COLOR);
			break;
		case WATER_BUTTON:
			draw_by_index(button_index, WATER_BUTTON_COLOR);
			break;
		case RESET_BUTTON:
			c = RESET_BUTTON_OFF_COLOR;
			if (reset == 1) {
				c = RESET_BUTTON_1_COLOR;
			} else if (reset == 2) {
				c = RESET_BUTTON_2_COLOR;
			}
			draw_by_index(button_index, c);
			break;
		case LENGTH_BUTTON:
			c = LENGTH_BUTTON_1_COLOR;;
			if (length == 2) {
				c = LENGTH_BUTTON_2_COLOR;
			} else if (length == 3) {
				c = LENGTH_BUTTON_3_COLOR;
			} else if (length == 4) {
				c = LENGTH_BUTTON_4_COLOR;
			}
			draw_by_index(button_index, c);
			break;
		case LOAD_BUTTON:
			draw_by_index(button_index, LOAD_BUTTON_COLOR);
			break;
		case CLEAR_BUTTON:
			draw_by_index(button_index, CLEAR_BUTTON_COLOR);
			break;
		case JUMP_BUTTON:
			draw_by_index(button_index, jump_on ? PERF_BUTTON_ON_COLOR : JUMP_BUTTON_COLOR);
			break;
		case SHUFFLE_BUTTON:
			draw_by_index(button_index, shuffle_on ? PERF_BUTTON_ON_COLOR : SHUFFLE_BUTTON_COLOR);
			break;
//		case ARP_BUTTON:
//			break;
		case FLOW_BUTTON:
			draw_by_index(button_index, flow_on ? PERF_BUTTON_ON_COLOR : FLOW_BUTTON_COLOR);
			break;
	}
}

void draw_function_buttons() {
	// top buttons
	draw_function_button(PLAY_BUTTON);
	draw_function_button(PANIC_BUTTON);
	draw_function_button(SETTINGS_BUTTON);
	draw_function_button(WATER_BUTTON);
	draw_function_button(RESET_BUTTON);
	draw_function_button(LENGTH_BUTTON);
	draw_function_button(LOAD_BUTTON);
	draw_function_button(CLEAR_BUTTON);

	// left buttons
	draw_function_button(JUMP_BUTTON);
	draw_function_button(SHUFFLE_BUTTON);
	draw_function_button(ARP_BUTTON);
	draw_function_button(FLOW_BUTTON);
}

void draw_stage(u8 column) {
	for (u8 row = 0; row < ROW_COUNT; row++) {
		if (column == PATTERN_MOD_COLUMN) {
			u8 c = get_pattern_grid(c_pattern, row, PATTERN_MOD_COLUMN);
			draw_button(PATTERN_MOD_GROUP, row, c);
		} else {
			u8 c = get_pattern_grid(c_pattern, row, stage_index(column));
			draw_pad(row, column, c);
		}
	}
}

void draw_stages() {
	if (in_settings) {
		return;
	}

	for (u8 column = 0; column < STAGE_COUNT + 1; column++) {
		draw_stage(column);
	}
}

void draw_settings() {
	int i = 0;

	// midi channel
	u8 c = DARK_GRAY;
	for (int row = SETTINGS_MIDI_ROW_1; row >= SETTINGS_MIDI_ROW_2; row--) {
		for (int column = 0; column < COLUMN_COUNT; column++) {
			c = DARK_GRAY;
			if (i == memory.settings.midi_channel) {
				c = WHITE;
			}
			draw_pad(row, column, c);
			i++;
		}
	}

	// version
	draw_binary_row(SETTINGS_VERSION_ROW, memory.settings.version);

	// auto-load button
	draw_pad(SETTINGS_MISC_ROW, SETTINGS_AUTO_LOAD_COLUMN, memory.settings.auto_load ? WHITE : DARK_GRAY);

	// patterns
	for (int column = 0; column < 4; column++) {
		draw_pad(SETTINGS_PATTERN_ROW, column, column == c_pattern ? PATTERN_SELECTED_COLOR_1 : PATTERN_COLOR_1);
	}
	for (int column = 4; column < COLUMN_COUNT; column++) {
		draw_pad(SETTINGS_PATTERN_ROW, column, column == c_pattern ? PATTERN_SELECTED_COLOR_2 : PATTERN_COLOR_2);
	}

	// flow sequences
	for (int column = 0; column < COLUMN_COUNT; column++) {
		draw_pad(SETTINGS_FLOW1_ROW, column, flow1_colors[memory.flow1[column]]);
		draw_pad(SETTINGS_FLOW2_ROW, column, flow2_colors[memory.flow2[column]]);
	}
}

void draw_patterns() {
	for (u8 p = 0; p < 4; p++) {
		u8 offset = PATTERNS_OFFSET_LO + (3 - p);
		if (c_pattern_group == 0) {
			draw_button(PATTERNS_GROUP, offset, p == c_pattern ? PATTERN_SELECTED_COLOR_1 : PATTERN_COLOR_1);
		} else {
			draw_button(PATTERNS_GROUP, offset, (p + 4) == c_pattern ? PATTERN_SELECTED_COLOR_2 : PATTERN_COLOR_2);
		}

	}
}

void draw_water() {
	clear_buttons();
	draw_patterns();
//	draw_function_buttons();
	Color c;
	for (u8 row = 0; row < ROW_COUNT; row++) {
		for (u8 column = 0; column < COLUMN_COUNT; column++) {
			u8 index = pad_index(row, column);
			if (index != OUT_OF_RANGE) {
				u8 r = rand() % 3;
				u8 g = rand() % 16;
				u8 b = rand() % 32 + 16;
				c = (Color){r, g, b};
				plot_led(TYPEPAD, index, c);
			}
		}
	}
	plot_led(TYPEPAD, WATER_BUTTON, c);
}

void draw() {
	if (!water_on) {
		draw_function_buttons();
		draw_markers();
		draw_patterns();
		draw_stages();
	}
}

/***** stages *****/

u8 get_note(Stage stage) {
	if (stage.note == OUT_OF_RANGE) {
		return OUT_OF_RANGE;
	}
	u8 n = stages[PATTERN_MOD_COLUMN].note == OUT_OF_RANGE ? 0 : stages[PATTERN_MOD_COLUMN].note;
	return (DEFAULT_OCTAVE + stage.octave + stages[PATTERN_MOD_COLUMN].octave) * 12 +
			note_map[stage.note + n] +
			stage.accidental + stages[PATTERN_MOD_COLUMN].accidental;
}

u8 get_velocity(Stage stage) {
	s8 v = DEFAULT_VELOCITY + (stage.velocity + stages[PATTERN_MOD_COLUMN].velocity) * VELOCITY_DELTA;
	v = v < 1 ? 1 : (v > 127 ? 127 : v);
	return v;
}

void note_off() {
	if (current_note != OUT_OF_RANGE) {
		hal_send_midi(USBMIDI, NOTEOFF | memory.settings.midi_channel, current_note, 0);
		current_note = OUT_OF_RANGE;
	}
}

// update_stage updates stage settings when a marker is changed.
//   if turn_on=true, a marker was added; if false, a marker was removed.
//   for some markers (NOTE), row placement matters.
// Note: the column param is where to draw any update; when shuffling, send
//   the column number, not the mapped value (stage_index)
void update_stage(Stage *stage, u8 row, u8 column, u8 marker, bool turn_on) {

	s8 inc = turn_on ? 1 : -1;

	switch (marker) {
		case NOTE_MARKER:
			if (turn_on && stage->note_count > 0) {
				u8 old_note = stage->note;
				stage->note_count--;
				stage->note = OUT_OF_RANGE;
				set_pattern_grid(c_pattern, old_note, stage_index(column), OFF_MARKER);
				draw_pad(old_note, column, OFF_MARKER);
			}
			stage->note_count += inc;
			if (turn_on) {
				stage->note = row;
			} else {
				stage->note = OUT_OF_RANGE;
			}
			break;

		case SHARP_MARKER:
			stage->accidental += inc;
			break;

		case FLAT_MARKER:
			stage->accidental -= inc;
			break;

		case OCTAVE_UP_MARKER:
			stage->octave += inc;
			break;

		case OCTAVE_DOWN_MARKER:
			stage->octave -= inc;
			break;

		case VELOCITY_UP_MARKER:
			stage->velocity += inc;
			break;

		case VELOCITY_DOWN_MARKER:
			stage->velocity -= inc;
			break;

		case EXTEND_MARKER:
			stage->extend += inc;
			break;

		case REPEAT_MARKER:
			stage->repeat += inc;
			break;

		case TIE_MARKER:
			stage->tie += inc;
			break;

		case LEGATO_MARKER:
			stage->legato += inc;
			break;

		case SKIP_MARKER:
			stage->skip += inc;
			break;

		case RANDOM_MARKER:
			stage->random += inc;
			break;
	}
}

void shuffle_stages() {
	for (int i = STAGE_COUNT - 1; i > 0; i--) {
		u8 r = rand() % (i + 1);
		u8 swap = stage_orders[c_pattern].order[r];
		stage_orders[c_pattern].order[r] = stage_orders[c_pattern].order[i];
		stage_orders[c_pattern].order[i] = swap;
//		stage_orders[c_pattern].order[i] = 3;
	}
//	for (int i = 0; i < STAGE_COUNT; i++) {
//		stage_orders[c_pattern].order[i] = STAGE_COUNT - 1 - i;
//	}
}

void unshuffle_stages() {
	for (int i = 0; i < 8; i++) {
		stage_orders[c_pattern].order[i] = i;
	}
}

void copy_pattern(u8 from_pattern, u8 to_pattern) {
	if (from_pattern == to_pattern) {
		return;
	}
	// copy the entire pattern
	memcpy(&memory.patterns[to_pattern], &memory.patterns[from_pattern], sizeof(memory.patterns[from_pattern]));

	// if shuffle is on, order the new pattern's stages in the shuffled order
	if (shuffle_on) {
		for (u8 s = 0; s < STAGE_COUNT; s++) {
			for (u8 row = 0; row < ROW_COUNT; row++) {
				u8 v = get_pattern_grid(from_pattern, row, s);
				set_pattern_grid(to_pattern, row, stage_index(s), v);
			}
		}

	}
}

/**
 * get_first_pattern returns the first pattern when patterns are combined (length > 1).
 * @return the index of the first pattern
 */
u8 get_first_pattern() {
	if (length == 2) {
		return c_pattern - c_pattern % 2;
	} else if (length == 3 && c_pattern % 4 < 3) {
		return c_pattern - c_pattern % 4;
	} else if (length == 4) {
		return c_pattern - c_pattern % 4;
	}
	return c_pattern;
}

/**
 * get_continue_pattern returns the next pattern
 * when patterns are combined (length > 1).
 * @return the index of the next pattern
 */
u8 get_continue_pattern() {
	u8 f = get_first_pattern();
	if (length == 2) {
		return f + (c_pattern + 1) % 2;
	} else if (length == 3 && c_pattern % 4 < 3) {
		return f + (c_pattern + 1) % 3;
	} else if (length == 4) {
		return f + (c_pattern +1) % 4;
	}
	else {
		return c_pattern;
	}
}

/**
 * get_next_pattern returns the next pattern when patterns
 * are being sequenced over multiple measures (flow = true).
 * @return the index of the next pattern
 */
u8 get_next_pattern() {
	if (flow_on) {
		flow_index = (flow_index + 1) % 8;
		if (c_pattern_group == 1) {
			return memory.flow2[flow_index] + 4;
		} else {
			return memory.flow1[flow_index];
		}
	}
	return c_pattern;
}


/***** save and load *****/

void clear_stages() {
	for (int s = 0; s < GRID_COLUMNS; s++) {
		stages[s] = (Stage) { 0, OUT_OF_RANGE, 0, 0, 0, 0, 0, 0, 0 };
	}
}

void clear() {
	memory.settings.version = APP_VERSION;
	memcpy(&memory.flow1, &flow1, sizeof(memory.flow1));
	memcpy(&memory.flow2, &flow2, sizeof(memory.flow2));
	clear_stages();
	for (int p = 0; p < PATTERN_COUNT; p++) {
		for (int row = 0; row < ROW_COUNT; row++) {
			for (int column = 0; column < GRID_COLUMNS; column++) {
				set_pattern_grid(p, row, column, OFF_MARKER);
				if (p == c_pattern) {
					draw_pad(row, column, OFF_MARKER);
				}
			}
		}

	}
}

void save() {
	memory.settings.version = APP_VERSION;
    hal_write_flash(0, (u8*)&memory, sizeof(memory));
}

void load_stages() {
	for (int row = 0; row < ROW_COUNT; row++) {
		for (int column = 0; column < GRID_COLUMNS; column++) {
			u8 value = get_pattern_grid(c_pattern, row, column);
			update_stage(&stages[column], row, column, value, true);
			if (!in_settings) {
				if (column == PATTERN_MOD_COLUMN) {
					draw_button(PATTERN_MOD_GROUP, row, value);
				} else {
					draw_pad(row, column, value);
				}
			}
		}
	}
}

void load() {
	clear();
	hal_read_flash(0, (u8*)&memory, sizeof(memory));
	load_stages();
	reset_stage_orders();
	draw();
}

void load_settings() {
	clear();
	hal_read_flash(0, (u8*)&memory.settings, sizeof(memory.settings));
	draw();
}

void change_pattern(u8 p_index) {
	c_pattern = p_index;
	c_pattern_group = c_pattern / 4;
	c_stage = c_extend = c_repeat = 0;
	shuffle_on = pattern_shuffles[p_index];
	clear_stages();
	load_stages();
	draw();
}


/***** handlers *****/

void on_settings(u8 index, u8 row, u8 column, u8 value) {
	if (value) {
		u8 c = memory.settings.midi_channel;

		if (row == SETTINGS_MIDI_ROW_1) {
			c = column;

		} else if (row == SETTINGS_MIDI_ROW_2) {
			c = column + 8;

		} else if (row == SETTINGS_PATTERN_ROW) {
			change_pattern(column);
			draw_settings();
			draw_patterns();

		} else if (row == SETTINGS_FLOW1_ROW) {
			memory.flow1[column] = (memory.flow1[column] + 1) % 4;
			draw_settings();

		} else if (row == SETTINGS_FLOW2_ROW) {
			memory.flow2[column] = (memory.flow2[column] + 1) % 4;
			draw_settings();

		} else if (row == SETTINGS_MISC_ROW && column == SETTINGS_AUTO_LOAD_COLUMN) {
			memory.settings.auto_load = !memory.settings.auto_load;
			draw_settings();

		}

		// if the MIDI channel changes, make sure there are no stuck notes
		if (c != memory.settings.midi_channel) {
			all_notes_off(memory.settings.midi_channel);
			memory.settings.midi_channel = c;
			draw_settings();
		}
	}
}

void on_pad(u8 index, u8 row, u8 column, u8 value) {

	if (in_settings) {
		on_settings(index, row, column, value);
		return;
	}

	if (jump_on) {
		c_stage = column;
		c_extend = c_repeat = 0;
		return;
	}

	if (value) {
		u8 previous = get_pattern_grid(c_pattern, row, stage_index(column));
		bool turn_on = (previous != current_marker);

		// remove the old marker that was at this row
		update_stage(&stages[stage_index(column)], row, column, previous, false);

		if (turn_on) {
			set_grid(row, stage_index(column), current_marker);
			draw_pad(row, column, current_marker);
			// add the new marker
			update_stage(&stages[stage_index(column)], row, column, current_marker, true);
		} else {
			set_grid(row, stage_index(column), OFF_MARKER);
			draw_pad(row, column, OFF_MARKER);
		}

	}
}

/**
 * on_button handles button events. when value=0, button was released.
 */
void on_button(u8 index, u8 group, u8 offset, u8 value) {
	if (index == PLAY_BUTTON) {
		if (value && (clock_source == INTERNAL || !is_running)) {
			is_running = !is_running;
			if (is_running) {
				clock_source = INTERNAL;
				c_measure = c_beat = c_tick = c_stage = 0;
			} else {
				note_off();
			}
		}
		draw_function_button(PLAY_BUTTON);

	} else if (index == PANIC_BUTTON) {
		if (value) {
			all_notes_off(memory.settings.midi_channel);
			draw_by_index(PANIC_BUTTON, PANIC_BUTTON_ON_COLOR);
		} else {
			draw_by_index(PANIC_BUTTON, PANIC_BUTTON_OFF_COLOR);
		}

	} else if (index == SETTINGS_BUTTON) {
		if (value) {
			in_settings = true;
			draw_by_index(SETTINGS_BUTTON, BUTTON_ON_COLOR);
			clear_pads();
			draw_settings();
		} else {
			in_settings = false;
			draw_by_index(SETTINGS_BUTTON, BUTTON_OFF_COLOR);
			draw_stages();
		}

	} else if (index == RESET_BUTTON) {
		if (value) {
			reset = (reset + 1) % 3;
			draw_function_button(RESET_BUTTON);
		}

	} else if (index == LENGTH_BUTTON) {
		if (value) {
			length = length % 4 + 1;
			draw_function_button(LENGTH_BUTTON);
		}

	} else if (index == LOAD_BUTTON) {
		if (value) {
			draw_by_index(LOAD_BUTTON, BUTTON_ON_COLOR);
			load();
		} else {
			draw_by_index(LOAD_BUTTON, BUTTON_OFF_COLOR);
		}

	} else if (index == CLEAR_BUTTON) {
		if (value) {
			draw_by_index(CLEAR_BUTTON, BUTTON_ON_COLOR);
			clear();
		} else {
			draw_by_index(CLEAR_BUTTON, BUTTON_OFF_COLOR);
		}

	} else if (index == FLOW_BUTTON) {
		if (value) {
			flow_on = !flow_on;
			if (flow_on) {
				flow_index = FLOW_LENGTH - 1;
				// don't change pattern yet; wait for the next reset
			}
			draw();
		}

	} else if (index == SHUFFLE_BUTTON) {
		if (value) {
			shuffle_on = !shuffle_on;
			pattern_shuffles[c_pattern] = shuffle_on;
			draw_function_button(SHUFFLE_BUTTON);
			if (shuffle_on) {
				shuffle_stages();
			} else {
				unshuffle_stages();
			}
			draw_stages();
		}

	} else if (index == JUMP_BUTTON) {
		if (value) {
			jump_on = !jump_on;
			draw_function_button(JUMP_BUTTON);
			draw_markers();
		}

	} else if (index == WATER_BUTTON) {
		if (value) {
			water_on = !water_on;
			if (water_on) {
				draw_water();
			} else {
				draw();
			}
		}

	} else if (group == PATTERN_MOD_GROUP) {
		// same logic as in on_pad() but for buttons
		if (value) {
			u8 previous = get_pattern_grid(c_pattern, offset, PATTERN_MOD_COLUMN);
			bool turn_on = (previous != current_marker);

			// remove the old marker that was at this row
			update_stage(&stages[PATTERN_MOD_COLUMN], offset, PATTERN_MOD_COLUMN, previous, false);

			if (turn_on) {
				set_grid(offset, PATTERN_MOD_COLUMN, current_marker);
				draw_button(PATTERN_MOD_GROUP, offset, current_marker);
				// add the new marker
				update_stage(&stages[PATTERN_MOD_COLUMN], offset, PATTERN_MOD_COLUMN, current_marker, true);
			} else {
				set_grid(offset, PATTERN_MOD_COLUMN, OFF_MARKER);
				draw_button(PATTERN_MOD_GROUP, offset, OFF_MARKER);
			}
		}


	} else if (group == PATTERNS_GROUP && offset >= PATTERNS_OFFSET_LO && offset <= PATTERNS_OFFSET_HI) {
		if (value) {
			pressed_button_index = index;
			pressed_button_time = app_clock;
			draw_button(group, offset, BUTTON_ON_COLOR);
		} else {
			u8 new_pattern = (3 - offset + PATTERNS_OFFSET_LO) + c_pattern_group * 4;
			if (index == pressed_button_index && app_clock - pressed_button_time >= LONG_PRESS_MILLIS) {
				copy_pattern(c_pattern, new_pattern);
			} else {
				// change pattern on release
				change_pattern(new_pattern);
			}
			draw_patterns();
		}

	} else if (group == MARKER_GROUP) {

		if (value && !jump_on) {
			u8 m = marker_map[offset];
			u8 n = OUT_OF_RANGE;

			u8 previous_marker_index = current_marker_index;

			// some markers require clever fooferaw
			switch (m) {
				case SHARP_MARKER:
					if (current_marker == SHARP_MARKER) {
						n = FLAT_MARKER;
					} else {
						n = SHARP_MARKER;
					}
					break;

				case OCTAVE_UP_MARKER:
					if (current_marker == OCTAVE_UP_MARKER) {
						n = OCTAVE_DOWN_MARKER;
					} else {
						n = OCTAVE_UP_MARKER;
					}
					break;

				case VELOCITY_UP_MARKER:
					if (current_marker == VELOCITY_UP_MARKER) {
						n = VELOCITY_DOWN_MARKER;
					} else {
						n = VELOCITY_UP_MARKER;
					}
					break;

				case EXTEND_MARKER:
					if (current_marker == EXTEND_MARKER) {
						n = REPEAT_MARKER;
					} else {
						n = EXTEND_MARKER;
					}
					break;

				case TIE_MARKER:
					if (current_marker == TIE_MARKER) {
						n = SKIP_MARKER;
					} else {
						n = TIE_MARKER;
					}
					break;

				case LEGATO_MARKER:
					if (current_marker == LEGATO_MARKER) {
						n = RANDOM_MARKER;
					} else {
						n = LEGATO_MARKER;
					}
					break;

				default:
					// neither MARKER_OFF nor MARKER_NOTE have alternate values
					n = m;
					break;
			}

			if (n != OUT_OF_RANGE) {
//				plot_led(TYPEPAD, DISPLAY_BUTTON, palette[n]);
				current_marker = n;
				current_marker_index = offset;
				draw_button(MARKER_GROUP, previous_marker_index, marker_map[previous_marker_index]);
			}
		}

	} else {

	}
}


/***** timing *****/

/**
 * tick does all the work for a clock tick.
 *
 * - step through the stages
 * - for each stage, step through its repeats
 * - for each repeat, step through its extensions
 *
 */
void tick() {


	if (!is_running) {
		return;
	}

	if (c_tick % TICKS_PER_16TH == 0) {

		// flash the status light in time
		if (c_beat == 0 && c_tick == 0) {
//			status_light(rainbow[c_measure % 8]);
			status_light(STATUS_LIGHT);
		} else if (c_tick == 0) {
			status_light(STATUS_LIGHT_DIM);
		} else {
			status_light(BLACK);
		}

		// update water if it's on
		if (water_on) {
			draw_water();
		}

		// if at beginning of pattern, we might change pattern (if length > 1)
		if (c_stage == 0) {
			u8 np = get_continue_pattern();
			if (np != c_pattern) {
				change_pattern(np);
			}
			c_stage = c_repeat = c_extend = 0;
		}

		// reset at the beginning of the measure (if reset is set)
		// reset=1: reset every measure; reset=2: every other measure
		if ((c_beat == 0 && c_tick == 0 && reset == 1) ||
				(c_beat == 0 && c_tick == 0 && reset == 2 && c_measure %2 == 0) ||
				(c_stage == 0 && reset == 0)) {
			u8 np = c_pattern;
			if (flow_on) {
				np = get_next_pattern();
			} else {
				np = get_first_pattern();
			}
			if (np != c_pattern) {
				change_pattern(np);
			}
			c_stage = c_repeat = c_extend = 0;
		}

		// copy the stage and apply randomness to it if needed.
		Stage stage = stages[stage_index(c_stage)];
		for (int i = 0; i < stage.random + stages[PATTERN_MOD_COLUMN].random; i++) {
			u8 r = rand() % RANDOM_MARKER_COUNT;
			update_stage(&stage, 0, stage_index(c_stage), random_markers[r], true);
		}

		// add in pattern mods (note & velocity mods computed later; random already computed)
		stage.extend += stages[PATTERN_MOD_COLUMN].extend;
		stage.repeat += stages[PATTERN_MOD_COLUMN].repeat;

		// if it's a tie or extension>0, do nothing
		// if it's legato, send previous note off after new note on
		// otherwise, send previous note off first
		// also highlight the current playing note on pads in PLAYING_NOTE_COLOR
		// and then turn it back to NOTE_MARKER when it stops
		if (stage.tie <= 0 && c_extend == 0) {
			if (stage.legato <= 0 && current_note != OUT_OF_RANGE) {
				note_off();
				if (!in_settings) {
//					draw_pad(current_note_row, current_note_column, NOTE_MARKER);
					draw_stage(current_note_column);
				}
			}

			u8 previous_note = current_note;
			u8 previous_note_column = current_note_column;
			if (stage.note_count > 0 && stage.note != OUT_OF_RANGE) {
				u8 n = get_note(stage);
				hal_send_midi(USBMIDI, NOTEON | memory.settings.midi_channel, n, get_velocity(stage));
				if (!in_settings) {
					draw_pad(stage.note, c_stage, PLAYING_NOTE_COLOR);
				}
				current_note = n;
				current_note_row = stage.note;
				current_note_column = c_stage;
			}

			if (stage.legato > 0 && previous_note != OUT_OF_RANGE) {
				hal_send_midi(USBMIDI, NOTEOFF | memory.settings.midi_channel, previous_note, 0);
				if (!in_settings) {
//					draw_pad(previous_note_row, previous_note_column, NOTE_MARKER);
					draw_stage(previous_note_column);
				}
			}
		}

#ifdef DEBUG
		// echo the current step activity on the debug buttons
		debug(1, stage.note_count > 0 ? NOTE_MARKER : OFF_MARKER);
		debug(1, stage.accidental > 0 ? SHARP_MARKER : OUT_OF_RANGE);
		debug(1, stage.accidental < 0 ? FLAT_MARKER : OUT_OF_RANGE);
		debug(1, stage.tie > 0 ? TIE_MARKER : OUT_OF_RANGE);
		debug(2, stage.octave > 0 ? OCTAVE_UP_MARKER : OFF_MARKER);
		debug(2, stage.octave < 0 ? OCTAVE_DOWN_MARKER : OUT_OF_RANGE);
		debug(3, stage.velocity > 0 ? VELOCITY_UP_MARKER : OFF_MARKER);
		debug(3, stage.velocity < 0 ? VELOCITY_DOWN_MARKER : OUT_OF_RANGE);
		debug(3, stage.legato > 0 ? LEGATO_MARKER : OFF_MARKER);
		debug(3, stage.legato > 0 ? LEGATO_MARKER : OFF_MARKER);
#endif

		// now increment current extension
		// if that would exceed the stage's extension count, increment the repeats count
		// if that would exceed the stage's repeat count, go to the next stage
		c_extend++;
		if (c_extend > stage.extend) {
			c_extend = 0;
			c_repeat++;
		}
		if (c_repeat > stage.repeat) {
			c_repeat = 0;
			u8 previous_stage = c_stage;
			c_stage = (c_stage + 1) % STAGE_COUNT;
			// if every stage is set to skip, it will replay the current stage
			while (stages[stage_index(c_stage)].skip > 0 && c_stage != previous_stage) {
				c_stage = (c_stage + 1) % STAGE_COUNT;
			}
//			if (c_stage == 0) {
//				u8 np = get_continue_pattern();
//				if (np != c_pattern) {
//					change_pattern(np);
//				}
//				c_stage = c_repeat = c_extend = 0;
//			}
		}


	}

	c_tick = (c_tick + 1) % TICKS_PER_BEAT;
	if (c_tick == 0) {
		c_beat = (c_beat + 1) % BEATS_PER_MEASURE;
	}
	if (c_beat == 0 && c_tick == 0) {
		c_measure++;
	}

}

void blink() {

	static u8 blink = 0;

	if (!jump_on) {
		if (blink == 0) {
			draw_button(MARKER_GROUP, current_marker_index, current_marker);
		} else {
			draw_button(MARKER_GROUP, current_marker_index, BLACK);
		}
	}

	blink = 1 - blink;
}


/***** callbacks for the HAL *****/

void app_surface_event(u8 type, u8 index, u8 value)
{

    switch (type)
    {
        case  TYPEPAD:
        {


			u8 row;
			u8 column;
			u8 check = index_to_row_column(index, &row, &column);
			if (check != OUT_OF_RANGE) {
				on_pad(index, row, column, value);
			} else {
				u8 group;
				u8 offset;
				check = index_to_group_offset(index, &group, &offset);
				if (check != OUT_OF_RANGE) {
					on_button(index, group, offset, value);
				}
			}

//			u8 row = index_to_row(index);
//			u8 column = index_to_column(index);
//			if (is_pad(row, column)) {
//				on_pad(index, row, column, value);
//			} else {
//				u8 group = index_to_group(index);
//				u8 offset = index_to_offset(index);
//				if (is_button(group, offset)) {
//					on_button(index, group, offset, value);
//				}
//			}



//            hal_send_midi(USBMIDI, NOTEON | 0, index, value);


//            // toggle it and store it off, so we can save to flash if we want to
//            if (value)
//            {
//                g_Buttons[index] = MAXLED * !g_Buttons[index];
//            }
//
//            // example - light / extinguish pad LEDs
//            hal_plot_led(TYPEPAD, index, 0, 0, g_Buttons[index]);
//
//            // example - send MIDI
//            hal_send_midi(USBMIDI, NOTEON | 0, index, value);
////            hal_send_midi(DINMIDI, NOTEON | 0, index, value);
            
        }
        break;
            
        case TYPESETUP:
        {
            if (value) {
            	save();
            }
        }
        break;
    }
}

//______________________________________________________________________________

void app_midi_event(u8 port, u8 status, u8 d1, u8 d2)
{

	switch (status) {

		case MIDISTART:
			is_running = true;
			clock_source = EXTERNAL;
			c_measure = c_beat = c_tick = c_stage = 0;
			draw_function_button(PLAY_BUTTON);
			break;

		case MIDISTOP:
			is_running = false;
			clock_source = INTERNAL;
			note_off();
			draw_function_button(PLAY_BUTTON);
			break;

		case MIDICONTINUE:
			is_running = true;
			clock_source = EXTERNAL;
			draw_function_button(PLAY_BUTTON);
			break;

		case MIDITIMINGCLOCK:
			if (is_running && clock_source == EXTERNAL) {
				tick();
			}
			break;

	}


//    // example - MIDI interface functionality for USB "MIDI" port -> DIN port
//    if (port == USBMIDI)
//    {
//        hal_send_midi(DINMIDI, status, d1, d2);
//    }
//
//    // // example -MIDI interface functionality for DIN -> USB "MIDI" port port
//    if (port == DINMIDI)
//    {
//        hal_send_midi(USBMIDI, status, d1, d2);
//    }
}

//______________________________________________________________________________

void app_sysex_event(u8 port, u8 * data, u16 count)
{
    // example - respond to UDI messages?
}

//______________________________________________________________________________

void app_aftertouch_event(u8 index, u8 value)
{
    // example - send poly aftertouch to MIDI ports
//    hal_send_midi(USBMIDI, POLYAFTERTOUCH | 0, index, value);
//    hal_send_midi(USBMIDI, CHANNELAFTERTOUCH | 0, value, 0);
    
    
}

//______________________________________________________________________________

void app_cable_event(u8 type, u8 value)
{
    // example - light the Setup LED to indicate cable connections
    if (type == MIDI_IN_CABLE)
    {
        hal_plot_led(TYPESETUP, 0, 0, value, 0); // green
    }
    else if (type == MIDI_OUT_CABLE)
    {
        hal_plot_led(TYPESETUP, 0, value, 0, 0); // red
    }
}

//______________________________________________________________________________


void app_timer_event()
{
	#define TICK_INTERVAL 20
	#define BLINK_INTERVAL 125

	app_clock++;

    static int tick_count = 0;
	static int tick_timer = TICK_INTERVAL;
	static int blink_timer = BLINK_INTERVAL;
    
    if (clock_source == INTERNAL && is_running && tick_timer >= TICK_INTERVAL) {
    	tick_timer = 0;
    	tick();
    } else {
    	tick_timer++;
    }
    
    if (blink_timer >= BLINK_INTERVAL) {
    	blink_timer = 0;
    	blink();
    } else {
    	blink_timer++;
    }

    tick_count++;

	// alternative example - show raw ADC data as LEDs
//	for (int i=0; i < PAD_COUNT; ++i)
//	{
//		// raw adc values are 12 bit, but LEDs are 6 bit.
//		// Let's saturate into r;g;b for a rainbow effect to show pressure
//		u16 r = 0;
//		u16 g = 0;
//		u16 b = 0;
//
//		u16 x = (3 * MAXLED * g_ADC[i]) >> 12;
//
//		if (x < MAXLED)
//		{
//			r = x;
//		}
//		else if (x >= MAXLED && x < (2*MAXLED))
//		{
//			r = 2*MAXLED - x;
//			g = x - MAXLED;
//		}
//		else
//		{
//			g = 3*MAXLED - x;
//			b = x - 2*MAXLED;
//		}
//
//		hal_plot_led(TYPEPAD, ADC_MAP[i], r, g, b);
//	}
}


/***** initialization *****/

//void colors_init() {
//
//	palette[BLACK] = (Color){0, 0, 0};
//	palette[DARK_GRAY] = (Color){C_LO, C_LO, C_LO};
//	palette[WHITE] = (Color){C_HI, C_HI, C_HI};
//	palette[GRAY] = (Color){C_MID, C_MID, C_MID};
//
//	palette[RED] = (Color){C_HI, 0, 0};
//	palette[ORANGE] = (Color){63, 20, 0};
//	palette[YELLOW] = (Color){C_HI, C_HI, 0};
//	palette[GREEN] = (Color){0, C_HI, 0};
//	palette[CYAN] = (Color){0, C_HI, C_HI};
//	palette[BLUE] = (Color){0, 0, C_HI};
//	palette[PURPLE] = (Color){10, 0, 63};
//	palette[MAGENTA] = (Color){C_HI, 0, C_HI};
//
//	palette[DIM_RED] = (Color){C_MID, 0, 0};
//	palette[DIM_ORANGE] = (Color){20, 8, 0};
//	palette[DIM_YELLOW] = (Color){C_MID, C_MID, 0};
//	palette[DIM_GREEN] = (Color){0, C_MID, 0};
//	palette[DIM_CYAN] = (Color){0, C_MID, C_MID};
//	palette[DIM_BLUE] = (Color){0, 0, C_MID};
//	palette[DIM_PURPLE] = (Color){4, 0, 20};
//	palette[DIM_MAGENTA] = (Color){C_MID, 0, C_MID};
//
//	palette[SKY_BLUE] = (Color){8, 18, 63};
//	palette[PINK] = (Color){32, 13, 22};
//	palette[DIM_PINK] = (Color){16, 7, 11};
//	palette[GRAY_GREEN] = (Color){18, 32, 22};
//	palette[DIM_GRAY_GREEN] = (Color){10, 22, 14};
//
//	rainbow[0] = WHITE;
//	rainbow[1] = RED;
//	rainbow[2] = ORANGE;
//	rainbow[3] = YELLOW;
//	rainbow[4] = GREEN;
//	rainbow[5] = CYAN;
//	rainbow[6] = BLUE;
//	rainbow[7] = PURPLE;
//
//}

void settings_init() {
	memory.settings.version = APP_VERSION;
	memory.settings.auto_load = false;
	memory.settings.tempo = 120;
	memory.settings.clock_divide = 6;
	memory.settings.midi_channel = 0;
	memory.settings.midi_ports = 3;
}

void app_init(const u16 *adc_raw)
{

	// initialize some things
	colors_init(palette);
	settings_init();
	warning(SKY_BLUE);

	// make sure stages are clear
	for (int s = 0; s < 8; s++) {
		stages[s] = (Stage) { 0, OUT_OF_RANGE, 0, 0, 0, 0, 0, 0, 0 };
	}
	reset_stage_orders();

	current_marker_index = 1;
	current_marker = marker_map[current_marker_index];

	// load just the settings, so we can check whether to load all patterns
	load_settings();
	if (memory.settings.auto_load) {
		load();
	}

	c_pattern = 0;
	draw();



//	time_t t;
//	srand((unsigned) time(&t));

    // example - load button states from flash
//    hal_read_flash(0, g_Buttons, BUTTON_COUNT);
    
    // example - light the LEDs to say hello!
//    for (int i=0; i < 10; ++i)
//    {
//        for (int j=0; j < 10; ++j)
//        {
////            u8 b = g_Buttons[j*10 + i];
////            hal_plot_led(TYPEPAD, j*10 + i, b, b, b);
//        	u8 index = j*10 + i;
//        	if (g_Buttons[index] < 0 || g_Buttons[index] >= PSIZE) {
//        		g_Buttons[index] = 0;
//        	}
//            plot_led(TYPEPAD, index, palette[g_Buttons[index]]);
//        }
//    }

	// store off the raw ADC frame pointer for later use
	g_ADC = adc_raw;
}
