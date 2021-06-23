#ifndef QUAKE_H
#define QUAKE_H

#include <stdbool.h>
#include "issho.h"


/***** application constants *****/
#define QUAKE_APP_ID = 1
#define APP_VERSION 1
#define PATTERN_COUNT 4
#define TRACK_COUNT 16
#define STEP_COUNT 16
#define PATTERN_COUNT 4
#define TRACK_BYTE_COUNT 8

// top buttons
#define EARTH_BUTTON 94

// pads
#define TRACK_PLAY_ROW_1 5
#define TRACK_PLAY_ROW_2 4
#define TRACK_SELECT_ROW_1 3
#define TRACK_SELECT_ROW_2 2
#define STEPS_ROW_1 1
#define STEPS_ROW_2 0
#define CLOCK_ROW_1 7
#define CLOCK_ROW_2 6

// settings
#define SETTINGS_MIDI_ROW_1 1
#define SETTINGS_MIDI_ROW_2 0
#define SETTINGS_VERSION_ROW 3
#define SETTINGS_MISC_ROW 2
#define SETTINGS_AUTO_LOAD_COLUMN 0
#define SETTINGS_MIDI_DIN_COLUMN 1
#define SETTINGS_MIDI_USB_COLUMN 2
#define SETTINGS_MIDI_STANDALONE_COLUMN 3

// main palette colors
#define PRIMARY_COLOR GREEN
#define PRIMARY_DIM_COLOR DIM_GREEN
//#define SECONDARY_COLOR RED
//#define SECONDARY_DIM_COLOR DIM_RED
#define SECONDARY_COLOR BROWN
#define SECONDARY_DIM_COLOR GRAY_BROWN
#define ACCENT_COLOR YELLOW
#define OFF_COLOR DARK_GRAY
#define ON_COLOR YELLOW

// element colors
#define TRACK_PLAY_OFF_COLOR BLACK
#define TRACK_PLAY_ON_COLOR PRIMARY_DIM_COLOR
#define TRACK_PLAY_OFF_PLAY_COLOR OFF_COLOR
#define TRACK_PLAY_ON_PLAY_COLOR ON_COLOR
#define TRACK_SELECT_OFF_COLOR SECONDARY_DIM_COLOR
#define TRACK_SELECT_SELECT_COLOR ON_COLOR
#define TRACK_SELECT_ACCENT_COLOR ON_COLOR
#define STEPS_OFF_COLOR BLACK
#define STEPS_ON_COLOR PRIMARY_DIM_COLOR
#define STEPS_ACCENT_COLOR ON_COLOR
#define STATUS_LIGHT GREEN
#define STATUS_LIGHT_DIM DIM_GREEN
#define EARTH_BUTTON_COLOR DIM_GREEN

// represents one track (4 bits per step, 16 steps = 64 bits = 8 bytes)
typedef struct Track {
	u8 bytes[TRACK_BYTE_COUNT];
} Track;

typedef struct Pattern {
	u16 enabled;
	Track tracks[TRACK_COUNT];
} Pattern;

// app-specific settings to be stored in flash memory
// all apps should have "u8 app_id" first so they can verify that the saved
// memory is for the right app before loading into memory
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
	Pattern patterns[PATTERN_COUNT];
} Memory;



#endif
