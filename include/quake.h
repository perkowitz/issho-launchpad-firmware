#ifndef QUAKE_H
#define QUAKE_H

#include <stdbool.h>
#include "issho.h"


/***** application constants *****/
#define QUAKE_APP_ID 1
#define APP_VERSION 1
#define PATTERN_COUNT 4
#define TRACK_COUNT 16
#define STEP_COUNT 16
#define PATTERN_COUNT 4
#define TRACK_BYTE_COUNT 8
#define MAX_VELOCITY 7    // max value stored
#define KICK_DRUM_TRACK 8
// top buttons
#define EARTH_BUTTON 94

// pads
#define TRACK_PLAY_ROW_1 7
#define TRACK_PLAY_ROW_2 6
#define TRACK_SELECT_ROW_1 5
#define TRACK_SELECT_ROW_2 4
#define STEPS_ROW_1 3
#define STEPS_ROW_2 2
#define CLOCK_ROW_1 1
#define CLOCK_ROW_2 0

// buttons
#define VELOCITY_GROUP RIGHT
#define VELOCITY_OFFSET 0
#define VELOCITY_BUTTON_COUNT 4
#define PATTERN_PLAY_GROUP LEFT
#define PATTERN_PLAY_OFFSET 4
#define PATTERN_SELECT_GROUP RIGHT
#define PATTERN_SELECT_OFFSET 4

// left buttons
#define JUMP_BUTTON 10
#define TRIGGER_BUTTON 20
#define AUTOFILL_BUTTON 30
#define QUAKE_BUTTON 40

// main palette colors
#define PRIMARY_COLOR GREEN
#define PRIMARY_DIM_COLOR DIM_GREEN
//#define SECONDARY_COLOR RED
//#define SECONDARY_DIM_COLOR DIM_RED
#define SECONDARY_COLOR BROWN
#define SECONDARY_DIM_COLOR BROWN
#define ACCENT_COLOR WHITE
#define OFF_COLOR DARK_GRAY
#define ON_COLOR YELLOW

// element colors
#define TRACK_PLAY_OFF_COLOR BLACK
#define TRACK_PLAY_ON_COLOR PRIMARY_DIM_COLOR
#define TRACK_PLAY_OFF_PLAY_COLOR OFF_COLOR
#define TRACK_PLAY_ON_PLAY_COLOR ON_COLOR
#define TRACK_SELECT_OFF_COLOR SECONDARY_DIM_COLOR
#define TRACK_SELECT_SELECT_COLOR ON_COLOR
#define STEPS_OFF_COLOR BLACK
#define STEPS_ON_COLOR GRAY
#define STATUS_LIGHT GREEN
#define STATUS_LIGHT_DIM BROWN
#define EARTH_BUTTON_COLOR DIM_GREEN
#define VELOCITY_OFF_COLOR BLACK
#define VELOCITY_ON_COLOR GRAY
#define PATTERN_PLAY_COLOR PRIMARY_DIM_COLOR
#define PATTERN_PLAY_ON_COLOR ON_COLOR
#define PATTERN_SELECT_COLOR SECONDARY_DIM_COLOR
#define PATTERN_SELECT_ON_COLOR ON_COLOR
#define QUAKE_BUTTON_COLOR DIM_RED
#define AUTOFILL_BUTTON_COLOR PINK
#define TRIGGER_BUTTON_COLOR SECONDARY_DIM_COLOR
#define JUMP_BUTTON_COLOR DIM_RED
#define KICK_DRUM_COLOR RED

// settings
#define SETTINGS_MIDI_ROW_1 1
#define SETTINGS_MIDI_ROW_2 0
#define SETTINGS_VERSION_ROW 3
#define SETTINGS_MISC_ROW 2
#define SETTINGS_AUTO_LOAD_COLUMN 0
#define SETTINGS_MIDI_DIN_COLUMN 1
#define SETTINGS_MIDI_USB_COLUMN 2
#define SETTINGS_MIDI_STANDALONE_COLUMN 3
#define SETTINGS_PATTERN_FOLLOW_COLUMN 7
#define SETTINGS_PATTERN_FOLLOW_COLOR PRIMARY_DIM_COLOR

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
	u8 pattern_follow;
	u8 reserved2;
	u8 reserved3;
	u8 reserved4;
} Settings;

typedef struct Memory {
	Settings settings;
	Pattern patterns[PATTERN_COUNT];
} Memory;



#endif