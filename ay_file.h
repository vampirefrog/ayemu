#pragma once

#include <stdint.h>
#include <stdlib.h>

struct __attribute__((__packed__)) ay_file_song_structure {
	uint16_t p_song_name, p_song_data;
};

struct __attribute__((__packed__)) ay_file_song_data {
	uint8_t a_chan, b_chan, c_chan, noise;
	uint16_t song_length;
	uint16_t fade_length;
	uint8_t hi_reg, lo_reg;
	uint16_t p_points, p_addresses;
};

struct __attribute__((__packed__)) ay_file_point {
	uint16_t stack, init, interrupt;
};

struct ay_file {
	uint8_t *data;
	size_t data_size;

	uint8_t file_version, player_version;
	uint8_t *special_player, *author, *misc;
	uint8_t num_songs, first_song;
	struct ay_file_song_structure *song_structures;

	uint8_t cur_song;
};

int ay_file_load(struct ay_file *, uint8_t *data, size_t size);
int ay_file_free(struct ay_file *);

void ay_file_dump(struct ay_file *);

char *ay_file_get_author(struct ay_file *f);
char *ay_file_get_misc(struct ay_file *f);
struct ay_file_song_data *ay_file_get_song_data(struct ay_file *f, int s);
char *ay_file_get_song_name(struct ay_file *f, int s);
struct ay_file_point *ay_file_get_song_point(struct ay_file *f, struct ay_file_song_data *sd);
uint16_t *ay_file_get_song_addresses(struct ay_file *f, struct ay_file_song_data *sd);

#ifndef __EMSCRIPTEN__
void ay_file_dump(struct ay_file *f);
#endif
