
#include "app.h"
#include "issho.h"
#include "quake.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>


#define TICK_INTERVAL 20
#define BLINK_INTERVAL 125

static const u16 *g_ADC = 0;   // ADC frame pointer
static u8 clock_source = INTERNAL;
static unsigned long app_clock = 0;

static bool earth_on = false;
static bool is_running = false;
static bool in_settings = false;
static bool quake_on = false;
static bool trigger_on = false;
static bool autofill_on = false;
static bool jump_on = false;

static u8 earth_row = 0;
static u8 earth_column = 0;
static u8 earth_size = 0;
static u8 earth_color = RED;


// mapping tracks to midi note numbers
const u8 note_map[TRACK_COUNT] = {
		49, 37, 39, 51, 42, 44, 46, 50,
		36, 38, 40, 41, 43, 45, 47, 48
};

static Memory memory;

// current location
static u8 c_pattern = 0;
static u8 c_step = 0;
static u8 sel_pattern = 0;
static u8 sel_track = 8;
static u8 sel_step;

// current timing
static int c_measure = 0;
static u8 c_beat = 0;
static u8 c_tick = 0;


/***** midi *****/

void note_off() {
}


/***** patterns, tracks, steps *****/

u8 byte_index(u8 step) {
	return step / 2;
}
u8 emask(u8 step) {
	return step %2 == 0 ? 0x80 : 0x08;
}
u8 vmask(u8 step) {
	return step %2 == 0 ? 0x70 : 0x07;
}
u8 vinvmask(u8 step) {
	return step %2 == 0 ? 0x8f : 0xf8;
}
u8 veloshift(u8 step, u8 velocity) {
	return step %2 == 0 ? (0x70 & velocity << 4) : (0x07 & velocity);
}
u8 velo2midi(u8 velocity) {
	u8 v = velocity * 18 + 1;
	return v > 127 ? 127 : v;
}

void get_step(u8 pattern, u8 track, u8 step, u8 *enabled, u8 *velocity) {
	u8 index = byte_index(step);
	*enabled = memory.patterns[pattern].tracks[track].bytes[index] & emask(step);
	if (*enabled > 15) {
		*enabled = *enabled >> 4;
	}
	*velocity = memory.patterns[pattern].tracks[track].bytes[index] & vmask(step);
	if (*velocity > 15) {
		*velocity = *velocity >> 4;
	}
}

void set_step_enabled(u8 pattern, u8 track, u8 step, u8 enabled) {
	u8 index = byte_index(step);
	u8 byte = memory.patterns[pattern].tracks[track].bytes[index];
	u8 new_byte = enabled ? byte | emask(step) : byte & emask(step);
	memory.patterns[pattern].tracks[track].bytes[index] = new_byte;
}

void set_velocity(u8 pattern, u8 track, u8 step, u8 velocity) {
	u8 index = byte_index(step);
	u8 byte = memory.patterns[pattern].tracks[track].bytes[index];
	u8 new_byte = byte & vinvmask(step);
	new_byte = new_byte | veloshift(step, velocity);
	memory.patterns[pattern].tracks[track].bytes[index] = new_byte;
}

void toggle_step_enabled(u8 pattern, u8 track, u8 step) {
	u8 index = byte_index(step);
	u8 byte = memory.patterns[pattern].tracks[track].bytes[index] ^ emask(step);;
	memory.patterns[pattern].tracks[track].bytes[index] = byte;
}

bool is_track_enabled(u8 pattern, u8 track) {
	u16 mask = 1 << track;
	return memory.patterns[pattern].enabled & mask;
}

void toggle_track_enabled(u8 pattern, u8 track) {
	u16 mask = 1 << track;
	memory.patterns[pattern].enabled = memory.patterns[pattern].enabled ^ mask;
}


/***** draw *****/

void draw_function_button(u8 button_index) {
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
		case EARTH_BUTTON:
			draw_by_index(button_index, EARTH_BUTTON_COLOR);
			break;
		case LOAD_BUTTON:
			draw_by_index(button_index, LOAD_BUTTON_COLOR);
			break;
		case CLEAR_BUTTON:
			draw_by_index(button_index, CLEAR_BUTTON_COLOR);
			break;
		case QUAKE_BUTTON:
			draw_by_index(button_index, quake_on ? ON_COLOR : QUAKE_BUTTON_COLOR);
			break;
		case AUTOFILL_BUTTON:
			draw_by_index(button_index, autofill_on ? ON_COLOR : AUTOFILL_BUTTON_COLOR);
			break;
		case TRIGGER_BUTTON:
			draw_by_index(button_index, trigger_on ? ON_COLOR : TRIGGER_BUTTON_COLOR);
			break;
		case JUMP_BUTTON:
			draw_by_index(button_index, jump_on ? ON_COLOR : JUMP_BUTTON_COLOR);
			break;

	}
}

void draw_function_buttons() {
	// top buttons
	draw_function_button(PLAY_BUTTON);
	draw_function_button(PANIC_BUTTON);
	draw_function_button(SETTINGS_BUTTON);
	draw_function_button(EARTH_BUTTON);
	draw_function_button(LOAD_BUTTON);
	draw_function_button(CLEAR_BUTTON);

	// left buttons
//	draw_function_button(QUAKE_BUTTON);
//	draw_function_button(AUTOFILL_BUTTON);
	draw_function_button(TRIGGER_BUTTON);
	draw_function_button(JUMP_BUTTON);
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
	draw_pad(SETTINGS_MISC_ROW, SETTINGS_AUTO_LOAD_COLUMN, memory.settings.auto_load ? SETTINGS_ON_COLOR : SETTINGS_AUTOLOAD_COLOR);

	// midi ports
	draw_pad(SETTINGS_MISC_ROW, SETTINGS_MIDI_USB_COLUMN, get_port(PORT_USB) ? SETTINGS_ON_COLOR : SETTINGS_MIDI_PORT_COLOR);
	draw_pad(SETTINGS_MISC_ROW, SETTINGS_MIDI_DIN_COLUMN, get_port(PORT_DIN) ? SETTINGS_ON_COLOR : SETTINGS_MIDI_PORT_COLOR);
	draw_pad(SETTINGS_MISC_ROW, SETTINGS_MIDI_STANDALONE_COLUMN, get_port(PORT_STANDALONE) ? SETTINGS_ON_COLOR : SETTINGS_MIDI_PORT_COLOR);

	// misc
	draw_pad(SETTINGS_MISC_ROW, SETTINGS_PATTERN_FOLLOW_COLUMN, memory.settings.pattern_follow ? SETTINGS_ON_COLOR : SETTINGS_PATTERN_FOLLOW_COLOR);

}

void draw_earth() {
	clear_buttons();
	for (u8 column = 0; column < COLUMN_COUNT; column++) {
		for (u8 row = 0; row < 4; row++) {
			plot_by_index(pad_index(row, column), rand_color(3, 8, 1, 4, 0, 1));
		}
		for (u8 row = 4; row < ROW_COUNT; row++) {
			plot_by_index(pad_index(row, column), rand_color(0, 4, 6, 16, 0, 2));
		}
	}
	plot_by_index(EARTH_BUTTON, rand_color(0, 4, 6, 16, 0, 2));

	if (c_step % 8 == 0) {
		earth_row = rand() % (ROW_COUNT - 2) + 1;
		earth_column = rand() % (COLUMN_COUNT - 2) + 1;
		earth_size = 0;
	}
	if (c_step % 2 == 0) {
		for (u8 row = earth_row - earth_size; row <= earth_row + earth_size; row++) {
			draw_pad(row, earth_column - earth_size, earth_color);
			draw_pad(row, earth_column + earth_size, earth_color);
		}
		for (u8 column = earth_column - earth_size; column <= earth_column + earth_size; column++) {
			draw_pad(earth_row - earth_size, column, earth_color);
			draw_pad(earth_row + earth_size, column, earth_color);
		}
		earth_size++;
	}
}

void draw_velocity(u8 velocity) {
	for (u8 offset = VELOCITY_OFFSET; offset < VELOCITY_BUTTON_COUNT; offset++) {
		draw_button(VELOCITY_GROUP, offset, offset <= velocity / 2 ? VELOCITY_ON_COLOR : VELOCITY_OFF_COLOR);
	}
}

void draw_step(u8 step, u8 enabled) {
	u8 row = step < 8 ? STEPS_ROW_1 : STEPS_ROW_2;
	u8 column = step % 8;
	u8 c = enabled ? STEPS_ON_COLOR : STEPS_OFF_COLOR;
	draw_pad(row, column, c);
}

// draws the steps for the provided pattern track
void draw_track(u8 pattern, u8 track) {
	for (u8 step = 0; step < STEP_COUNT; step++) {
		u8 enabled;
		u8 velocity;
		get_step(pattern, track, step, &enabled, &velocity);
		draw_step(step, enabled);
	}
}

// draws the currently selected pattern
void draw_playing_pattern(u8 pattern) {
	u16 bit = 1;
	for (u8 track = 0; track < TRACK_COUNT; track++) {
		// show enabled tracks
		// TODO this is about PLAYING pattern, rest is about SELECTED pattern
		u8 row = track < 8 ? TRACK_PLAY_ROW_1 : TRACK_PLAY_ROW_2;
		u8 column = track % 8;
		u8 c = is_track_enabled(pattern, track) ? TRACK_PLAY_ON_COLOR : TRACK_PLAY_OFF_COLOR;
//		u8 c = memory.patterns[pattern].enabled & bit ? TRACK_PLAY_ON_COLOR : TRACK_PLAY_OFF_COLOR;
		draw_pad(row, column, c);
		bit = bit << 1;
	}
}

// draws the currently selected pattern
void draw_selected_pattern(u8 pattern) {
	for (u8 track = 0; track < TRACK_COUNT; track++) {
		// show selected track
		u8 row = track < 8 ? TRACK_SELECT_ROW_1 : TRACK_SELECT_ROW_2;
		u8 column = track % 8;
		u8 c = track == sel_track ? TRACK_SELECT_SELECT_COLOR : TRACK_SELECT_OFF_COLOR;
		draw_pad(row, column, c);
	}
	draw_track(pattern, sel_track);
}

void draw_pattern_buttons() {
	for (u8 pattern = 0; pattern < PATTERN_COUNT; pattern++) {
		draw_button(PATTERN_PLAY_GROUP, pattern + PATTERN_PLAY_OFFSET,
				pattern == c_pattern ? PATTERN_PLAY_ON_COLOR : PATTERN_PLAY_COLOR);
		draw_button(PATTERN_SELECT_GROUP, pattern + PATTERN_SELECT_OFFSET,
				pattern == sel_pattern ? PATTERN_SELECT_ON_COLOR : PATTERN_SELECT_COLOR);
	}
}

void draw_clock() {
	for (u8 step = 0; step < STEP_COUNT; step++) {
		u8 row = step < 8 ? CLOCK_ROW_1 : CLOCK_ROW_2;
		u8 column = step % 8;
		u8 c = step == c_step ? ON_COLOR : BLACK;
		draw_pad(row, column, c);
	}

}

void draw() {
	if (!earth_on) {
		clear_pads();
		clear_buttons();
		draw_function_buttons();
		draw_selected_pattern(sel_pattern);
		draw_playing_pattern(c_pattern);
		draw_pattern_buttons();
		draw_clock();
	}
}


/***** save and load *****/

void clear_pattern(u8 index) {
	for (u8 track = 0; track < TRACK_COUNT; track++) {
		for (u8 byte = 0; byte < TRACK_BYTE_COUNT; byte++) {
			memory.patterns[index].tracks[track].bytes[byte] = 0;
		}
	}
	memory.patterns[index].enabled = -1;  // all tracks enabled
}

void clear() {
	memory.settings.version = APP_VERSION;
	for (u8 pattern = 0; pattern < PATTERN_COUNT; pattern++) {
		clear_pattern(pattern);
	}
}

void save() {
	memory.settings.version = APP_VERSION;
	memory.settings.midi_ports = (get_port(PORT_DIN) << 2) + (get_port(PORT_USB) << 1) + get_port(PORT_STANDALONE);
    hal_write_flash(0, (u8*)&memory, sizeof(memory));
}

void load() {
	if (read_app_id() != QUAKE_APP_ID) {
		draw_by_index(LOAD_BUTTON, RED);
		return;
	}
	clear();
	hal_read_flash(0, (u8*)&memory, sizeof(memory));
	set_port(PORT_DIN, memory.settings.midi_ports & 4);
	set_port(PORT_USB, memory.settings.midi_ports & 2);
	set_port(PORT_STANDALONE, memory.settings.midi_ports & 1);
	draw();
}

void load_settings() {
	if (read_app_id() != QUAKE_APP_ID) {
		draw_by_index(LOAD_BUTTON, RED);
		return;
	}
	clear();
	hal_read_flash(0, (u8*)&memory.settings, sizeof(memory.settings));
	set_port(PORT_DIN, memory.settings.midi_ports & 4);
	set_port(PORT_USB, memory.settings.midi_ports & 2);
	set_port(PORT_STANDALONE, memory.settings.midi_ports & 1);
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

		} else if (row == SETTINGS_MISC_ROW && column == SETTINGS_AUTO_LOAD_COLUMN) {
			memory.settings.auto_load = !memory.settings.auto_load;
			draw_settings();

		} else if (row == SETTINGS_MISC_ROW && column == SETTINGS_MIDI_USB_COLUMN) {
			all_notes_off(memory.settings.midi_channel);
			flip_port(PORT_USB);
			draw_settings();

		} else if (row == SETTINGS_MISC_ROW && column == SETTINGS_MIDI_DIN_COLUMN) {
			all_notes_off(memory.settings.midi_channel);
			flip_port(PORT_DIN);
			draw_settings();

		} else if (row == SETTINGS_MISC_ROW && column == SETTINGS_MIDI_STANDALONE_COLUMN) {
			all_notes_off(memory.settings.midi_channel);
			flip_port(PORT_STANDALONE);
			draw_settings();

		} else if (row == SETTINGS_MISC_ROW && column == SETTINGS_PATTERN_FOLLOW_COLUMN) {
			memory.settings.pattern_follow = !memory.settings.pattern_follow;
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

	if (row == TRACK_PLAY_ROW_1 || row == TRACK_PLAY_ROW_2) {
		if (value) {
			u8 track = column;
			if (row == TRACK_PLAY_ROW_2) {
				track += 8;
			}
			toggle_track_enabled(c_pattern, track);
			draw_playing_pattern(c_pattern);
		}

	} else if (row == TRACK_SELECT_ROW_1 || row == TRACK_SELECT_ROW_2) {
			if (value) {
				sel_track = column;
				if (row == TRACK_SELECT_ROW_2) {
					sel_track += 8;
				}
				draw_selected_pattern(sel_pattern);
			}

	} else if (row == STEPS_ROW_1 || row == STEPS_ROW_2) {
			if (value) {
				sel_step = column + (row == STEPS_ROW_2 ? 8 : 0);
				toggle_step_enabled(sel_pattern, sel_track, sel_step);
				u8 enabled;
				u8 velocity;
				get_step(sel_pattern, sel_track, sel_step, &enabled, &velocity);
				if (velocity == 0) {
					velocity = 5;
					set_velocity(sel_pattern, sel_track, sel_step, velocity);

				}
				draw_velocity(velocity);
				draw_selected_pattern(sel_pattern);
			}

	} else if (trigger_on && (row == CLOCK_ROW_1 || row == CLOCK_ROW_2)) {
		u8 track = column + (row == CLOCK_ROW_2 ? 8 : 0);
		if (value) {
			midi_note(memory.settings.midi_channel, note_map[track], value);
			draw_by_index(index, ON_COLOR);
		} else {
			midi_note(memory.settings.midi_channel, note_map[track], 0);
			draw_by_index(index, BLACK);
		}

	} else if (jump_on && (row == CLOCK_ROW_1 || row == CLOCK_ROW_2)) {
		if (value) {
			c_step = column + (row == CLOCK_ROW_2 ? 8 : 0);
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
				c_measure = c_beat = c_tick = 0;
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
			draw();
		}

	} else if (index == EARTH_BUTTON) {
		if (value) {
			earth_on = !earth_on;
			if (earth_on) {
				draw_earth();
			} else {
				draw();
			}
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

	} else if (index == TRIGGER_BUTTON) {
		if (value) {
			trigger_on = !trigger_on;
			if (trigger_on) {
				jump_on = false;
			}
			draw();
		}

	} else if (index == JUMP_BUTTON) {
		if (value) {
			jump_on = !jump_on;
			if (jump_on) {
				trigger_on = false;
			}
			draw();
		}

	} else if (group == VELOCITY_GROUP && offset >= VELOCITY_OFFSET && offset < VELOCITY_OFFSET + VELOCITY_BUTTON_COUNT) {
		if (value && sel_step != OUT_OF_RANGE) {
			u8 v = (offset - VELOCITY_OFFSET) * 2 + 1;
			set_velocity(sel_pattern, sel_track, sel_step, v);
			draw_velocity(v);
		}

	} else if (group == PATTERN_PLAY_GROUP && offset >= PATTERN_PLAY_OFFSET && offset < PATTERN_PLAY_OFFSET + PATTERN_COUNT) {
		c_pattern = offset - PATTERN_PLAY_OFFSET;
		if (memory.settings.pattern_follow != 0) {
			sel_pattern = offset - PATTERN_SELECT_OFFSET;
		}
		draw();

	} else if (group == PATTERN_SELECT_GROUP && offset >= PATTERN_SELECT_OFFSET && offset < PATTERN_SELECT_OFFSET + PATTERN_COUNT) {
		sel_pattern = offset - PATTERN_SELECT_OFFSET;
		draw();

	}

}


/***** timing *****/

void tick() {


	if (!is_running) {
		return;
	}

	if (c_tick % TICKS_PER_16TH == 0) {

		// flash the status light in time
		if (c_beat == 0 && c_tick == 0) {
			status(STATUS_LIGHT);
			c_step = 0;
		} else if (c_tick == 0) {
			status(STATUS_LIGHT_DIM);
		} else {
			status(BLACK);
		}


		// update earth if it's on
		if (earth_on) {
			draw_earth();
		} else {
			draw_selected_pattern(sel_pattern);
			draw_playing_pattern(c_pattern);
			draw_clock();
		}

		u8 enabled;
		u8 velocity;
		for (u8 track = 0; track < TRACK_COUNT; track++) {
			get_step(c_pattern, track, c_step, &enabled, &velocity);
			if (enabled) {
				u8 row = track < 8 ? TRACK_PLAY_ROW_1 : TRACK_PLAY_ROW_2;
				u8 column = track % 8;
				u8 track_enabled = is_track_enabled(c_pattern, track);
				u8 c = track_enabled ? TRACK_PLAY_ON_PLAY_COLOR : TRACK_PLAY_OFF_PLAY_COLOR;
				draw_pad(row, column, c);
				if (track_enabled) {
					midi_note(memory.settings.midi_channel, note_map[track], velo2midi(velocity));
					if (track == KICK_DRUM_TRACK) {
						draw_pad(row, column, KICK_DRUM_COLOR);
						// in earth mode, make an extra boom for the kick drum
						if (earth_on) {
							u8 c = rand() % 7;
							draw_pad(0, c, KICK_DRUM_COLOR);
							draw_pad(0, c + 1, KICK_DRUM_COLOR);
							draw_pad(1, c, KICK_DRUM_COLOR);
							draw_pad(1, c + 1, KICK_DRUM_COLOR);
						}
					}
				}
			}
		}


		c_step = (c_step + 1) % STEP_COUNT;
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

	if (blink == 0) {
	} else {
	}

	blink = 1 - blink;
}


/***** callbacks for the HAL *****/

void app_surface_event(u8 type, u8 index, u8 value) {

    switch (type) {
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


void app_midi_event(u8 port, u8 status, u8 d1, u8 d2) {

	switch (status) {
		case MIDISTART:
			is_running = true;
			clock_source = EXTERNAL;
			c_step = c_measure = c_beat = c_tick = 0;
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

}


void app_sysex_event(u8 port, u8 * data, u16 count) { }


void app_aftertouch_event(u8 index, u8 value) { }


void app_cable_event(u8 type, u8 value) {
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


void app_timer_event() {

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

}


/***** initialization *****/

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
	colors_init();
	settings_init();
	status(PRIMARY_COLOR);

	// load just the settings, so we can check whether to load all patterns
	load_settings();
	if (memory.settings.auto_load) {
		load();
	}

	c_pattern = 0;
	draw();

	// store off the raw ADC frame pointer for later use
	g_ADC = adc_raw;
}
