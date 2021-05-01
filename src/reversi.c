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
#include "reversi.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>


//#define DEBUG 1

/***** global variables *****/
static const u16 *g_ADC = 0;   // ADC frame pointer
//static u8 hw_buttons[BUTTON_COUNT] = {0};
static Color palette[PSIZE];
static u8 rainbow[8];

u8 grid[ROW_COUNT][COLUMN_COUNT];

static u8 p1_color = WHITE;
static u8 p2_color = BLUE;
static u8 current_player;
static u8 other_player;


/***** helper functions *****/

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


/***** grid *****/

void clear_grid(bool andDraw) {
	for (int row = 0; row < ROW_COUNT; row++) {
		for (int column = 0; column < COLUMN_COUNT; column++) {
			grid[row][column] = BLACK;
			if (andDraw) {
				draw_pad(row, column, BLACK);
			}
		}
	}
}

void set_grid(u8 row, u8 col, u8 color, bool andDraw) {
	if (row >= 0 && row < ROW_COUNT && col >= 0 && col < COLUMN_COUNT) {
		grid[row][col] = color;
		if (andDraw) {
			draw_pad(row, col, color);
		}
	}
}

u8 get_grid(u8 row, u8 col) {
	if (row >= 0 && row < ROW_COUNT && col >= 0 && col < COLUMN_COUNT) {
		return grid[row][col];
	}
	return OUT_OF_RANGE;
}


/***** draw *****/

void color_pads(u8 color) {
	for (int row = 0; row < ROW_COUNT; row++) {
		for (int column = 0; column < COLUMN_COUNT; column++) {
			draw_pad(row, column, color);
		}
	}
}

void clear_pads() {
	color_pads(BLACK);
}

void draw_buttons(u8 group, u8 color) {
	for (u8 offset = 0; offset < 8; offset++) {
		draw_button(group, offset, color);
	}
}

void draw_win(u8 color) {
	color_pads(color);
}

void draw_pads() {
	for (int row = 0; row < ROW_COUNT; row++) {
		for (int column = 0; column < COLUMN_COUNT; column++) {
			u8 c = get_grid(row, column);
			if (c != OUT_OF_RANGE) {
				draw_pad(row, column, c);
			}
		}
	}
}

void draw() {
	draw_pads();
}

/***** game logic *****/

bool check_direction(u8 row, u8 column, u8 d_row, u8 d_column) {
	return true;
}

bool check_move(u8 row, u8 column) {

	draw_button(TOP, column, WHITE);
	draw_button(row, LEFT, WHITE);
	bool legal = false;
	u8 offset = 0;
	for (u8 d_row = -1; d_row <= 1; d_row++) {
		for (u8 d_column = -1; d_column <= 1; d_column++) {
			if (d_row != 0 || d_column != 0) {
				draw_button(BOTTOM, offset, DIM_RED);
				u8 c = get_grid(row + d_row, column + d_column);
				draw_button(RIGHT, offset, c);
				if (c == other_player) {
					//				legal = (legal || check_direction(row, column, d_row, d_column));
					legal = true;
					draw_button(BOTTOM, offset, DIM_GREEN);
				}
				offset++;
			}
		}
	}

	return legal;
}

void next_player() {
	u8 p = other_player;
	other_player = current_player;
	current_player = p;
}


/***** handlers *****/

void on_pad(u8 index, u8 row, u8 column, u8 value) {
	if (value) {
		u8 v = get_grid(row, column);
		draw_buttons(TOP, BLACK);
		draw_buttons(BOTTOM, BLACK);
		draw_buttons(LEFT, BLACK);
		draw_buttons(RIGHT, BLACK);
		if (v != p1_color && v != p2_color) {
			bool legal = check_move(row, column);
			if (legal) {
				set_grid(row, column, current_player, true);
//				next_player();
			} else {
				draw_pad(row, column, DIM_RED);
			}
		}
	}
}

/**
 * on_button handles button events. when value=0, button was released.
 */
void on_button(u8 index, u8 group, u8 offset, u8 value) {
}


/***** callbacks for the HAL *****/

void app_surface_event(u8 type, u8 index, u8 value)
{

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
            }
        }
        break;
    }
}


void app_midi_event(u8 port, u8 status, u8 d1, u8 d2) {}

void app_sysex_event(u8 port, u8 * data, u16 count) {}

void app_aftertouch_event(u8 index, u8 value) {}

void app_cable_event(u8 type, u8 value) {}

void app_timer_event() {}
    

/***** initialization *****/

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

	rainbow[0] = WHITE;
	rainbow[1] = RED;
	rainbow[2] = ORANGE;
	rainbow[3] = YELLOW;
	rainbow[4] = GREEN;
	rainbow[5] = CYAN;
	rainbow[6] = BLUE;
	rainbow[7] = PURPLE;

}

void app_init(const u16 *adc_raw)
{

	// initialize some things
	colors_init();
	status_light(WHITE);

	current_player = p1_color;
	other_player = p2_color;
	set_grid(3, 3, p1_color, true);
	set_grid(4, 4, p1_color, true);
	set_grid(4, 3, p2_color, true);
	set_grid(3, 4, p2_color, true);

	g_ADC = adc_raw;
}
