#include "app.h"
#include "issho.h"
#include <stdlib.h>
#include <stdbool.h>

// vars
static bool midi_ports[3] = { true, true, false };
static Color palette[PSIZE];
static u8 hw_buttons[BUTTON_COUNT] = {0};




void colors_init() {

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
	palette[BROWN] = (Color){6, 2, 0};
	palette[GRAY_BROWN] = (Color){6, 4, 2};

}

Color rand_color(u8 r_lo, u8 r_hi, u8 g_lo, u8 g_hi, u8 b_lo, u8 b_hi) {
	u8 r = rand() % (r_hi - r_lo) + r_lo;
	u8 g = rand() % (g_hi - g_lo) + g_lo;
	u8 b = rand() % (b_hi - b_lo) + b_lo;
	return (Color){r, g, b};

}


/***** lighting up stuff *****/
void plot_led(u8 type, u8 index, Color color) {
	if (index >= 0 && index < BUTTON_COUNT) {
		hal_plot_led(type, index, color.red, color.green, color.blue);
	}
}

void plot_by_index(u8 index, Color color) {
	plot_led(TYPEPAD, index, color);
}

void status_light(Color color) {
	plot_led(TYPESETUP, 0, color);
}

void draw_by_index(u8 index, u8 palette_index) {
	if (palette_index >= 0 && palette_index < PSIZE) {
		plot_by_index(index, palette[palette_index]);
	}
}

void status(u8 palette_index) {
	if (palette_index >= 0 && palette_index < PSIZE) {
		status_light(palette[palette_index]);
	}
}



/***** midi *****/

bool get_port(u8 index) {
	return midi_ports[index];
}

void set_port(u8 index, bool enabled) {
	midi_ports[index] = enabled;
}

void flip_port(u8 index) {
	midi_ports[index] = !midi_ports[index];
}

void send_midi(u8 status, u8 data1, u8 data2) {
//	if (get_port(PORT_DIN)) {
//		hal_send_midi(DINMIDI, status, data1, data2);
//	}
//	if (get_port(PORT_USB)) {
//		hal_send_midi(USBMIDI, status, data1, data2);
//	}
//	if (get_port(PORT_STANDALONE)) {
//		hal_send_midi(USBSTANDALONE, status, data1, data2);
//	}
	hal_send_midi(USBMIDI, status, data1, data2);
}

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

