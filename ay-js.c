#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "tools.h"
#include "ay_player.h"

int opt_channel_mask = 0xffff;
int opt_loops = 1;

struct ay_player player;
struct ay_file f;

int bufSize, sampleRate;
int32_t *bufL, *bufR;

void init_player(int32_t *pBufL, int32_t *pBufR, int buf_size, int sample_rate) {
	memset(&f, 0, sizeof(f));
	bufSize = buf_size;
	sampleRate = sample_rate;
	bufL = pBufL;
	bufR = pBufR;
	ay_player_init(&player, sample_rate);
}

int play_file(uint8_t *ay_data, size_t ay_data_len, int track) {
	int er = ay_file_load(&f, ay_data, ay_data_len);
	if(er) {
		printf("Error loading AY file\n");
		return 2;
	}

	ay_player_play_file(&player, &f, track);

	return 0;
}

int render_samples() {
	ay_player_fill_buffer(&player, bufL, bufR, bufSize);
	return player.ended;
}

int get_num_songs() {
	return f.num_songs;
}

char *get_song_name(int n) {
	return ay_file_get_song_name(&f, n);
}
