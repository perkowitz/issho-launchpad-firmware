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
#include "game.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>


//#define DEBUG 1

/***** global variables *****/
static const u16 *g_ADC = 0;   // ADC frame pointer
static u8 hw_buttons[BUTTON_COUNT] = {0};
static Color palette[PSIZE];
static u8 rainbow[8];

static int interval_millis = 1000;
static int tick_count = 0;
static int tick_timer = 0;
static int blink_timer = BLINK_INTERVAL;

static u8 warning_level = 0;
static u8 warning_blink = 0;

static bool in_settings = false;
u8 grid[ROW_COUNT][COLUMN_COUNT];

static u8 p1_color = SKY_BLUE;
static u8 p1_color2 = BLUE;
static u8 p2_color = GREEN;
static u8 p2_color2 = DIM_GREEN;
static u8 p1_score = 0;
static u8 p2_score = 0;
static u8 p1_row = OUT_OF_RANGE;
static u8 p1_col = OUT_OF_RANGE;
static u8 p2_row = OUT_OF_RANGE;
static u8 p2_col = OUT_OF_RANGE;

static u8 stage = START;
static u8 stage_clock = 0;
static u8 mode = TWO_PLAYER_ALL;


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

void draw_function_button(u8 button_index) {

	switch (button_index) {
		case PLAY_BUTTON:
//			draw_by_index(button_index, WHITE);
			break;
	}
}

void draw_function_buttons() {
	draw_function_button(PLAY_BUTTON);
}

void draw_scores() {
	draw_button(LEFT, (p1_score - 1) % 8, p1_color);
	draw_button(RIGHT, (p2_score - 1) % 8, p2_color);
}

void draw_win(u8 color) {
	color_pads(color);
	p1_score = p2_score = 0;
	interval_millis *= 0.9;
	stage_clock = 0;
	stage = START;
}

void draw_pads() {
	for (int row = 0; row < ROW_COUNT; row++) {
		for (int column = 0; column < COLUMN_COUNT; column++) {
			draw_pad(row, column, DARK_GRAY);
		}
	}
}

void draw_settings() {
}

void draw() {
	draw_function_buttons();
	draw_pads();
}

void random_pads() {
	switch (mode) {
	case TWO_PLAYER_ALL:
		p1_row = rand() % ROW_COUNT;
		p1_col = rand() % COLUMN_COUNT;
		p2_row = rand() % ROW_COUNT;
		p2_col = rand() % COLUMN_COUNT;
		while (p1_row == p2_row && p1_col == p2_col) {
			p2_col = rand() % COLUMN_COUNT;
		}
		break;

	case TWO_PLAYER_HALF:
		p1_row = rand() % 4;
		p1_col = rand() % COLUMN_COUNT;
		p2_row = rand() % 4 + 4;
		p2_col = rand() % COLUMN_COUNT;
		break;
	}

}


/***** handlers *****/

void on_settings(u8 index, u8 row, u8 column, u8 value) {
	if (value) {
	}
}

void on_pad(u8 index, u8 row, u8 column, u8 value) {

	if (in_settings) {
		on_settings(index, row, column, value);
		return;
	}

	if (value) {
		if (row == p1_row && column == p1_col) {
			set_grid(row, column, p1_color2, true);
			p1_score++;
			draw_scores();
			if (p1_score >= 8) {
				draw_win(p1_color);
			}
		} else if (row == p2_row && column == p2_col) {
			set_grid(row, column, p2_color2, true);
			p2_score++;
			draw_scores();
			if (p2_score >= 8) {
				draw_win(p2_color);
			}
		}


	}
}

/**
 * on_button handles button events. when value=0, button was released.
 */
void on_button(u8 index, u8 group, u8 offset, u8 value) {
	if (index == PLAY_BUTTON) {

	} else {

	}
}


/***** timing *****/

/**
 * tick does all the work for a clock tick.
 */
void tick() {

	switch (stage) {

	case START:
		if (stage_clock >= 4) {
			stage_clock = 0;
			clear_pads();
			draw_buttons(LEFT, BLACK);
			draw_buttons(RIGHT, BLACK);
			stage = PLAYING;
		} else {
			stage_clock++;
		}
		break;

	case PLAYING:
		if (get_grid(p1_row, p1_col) != p1_color2) {
			set_grid(p1_row, p1_col, BLACK, true);
		}
		if (get_grid(p2_row, p2_col) != p2_color2) {
			set_grid(p2_row, p2_col, BLACK, true);
		}
		p1_row = p1_col = p2_row = p2_col = OUT_OF_RANGE;

		random_pads();

		set_grid(p1_row, p1_col, p1_color, true);
		set_grid(p2_row, p2_col, p2_color, true);
		break;

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

//______________________________________________________________________________

void app_midi_event(u8 port, u8 status, u8 d1, u8 d2)
{

	switch (status) {

		case MIDISTART:
			break;

		case MIDISTOP:
			break;

		case MIDICONTINUE:
			break;

		case MIDITIMINGCLOCK:
			break;
	}

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
    if (type == MIDI_IN_CABLE) {
    }
    else if (type == MIDI_OUT_CABLE) {
    }
}

//______________________________________________________________________________


void app_timer_event()
{
    
    if (tick_timer >= interval_millis) {
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
	warning(SKY_BLUE);

	tick_timer = interval_millis;
	stage = START;
	stage_clock = 0;
	mode = TWO_PLAYER_HALF;
	draw();

	g_ADC = adc_raw;
}
