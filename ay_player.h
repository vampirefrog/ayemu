#pragma once

#include <stdint.h>

#include "z80emu/z80emu.h"
#include "ay_file.h"
#include "emu2149.h"
#include "vgm.h"

struct ay_player {
	struct ay_file *file;
	Z80_STATE z80;
	PSG psg;
	int sample_rate;
	uint8_t memory[65536];
	struct vgm_logger *logger;

	int cpu_clock;
	int tstate_remainder;
	int buf_remaining;
	int32_t *bufL, *bufR;
	int int_tstates, int_remainder;
	int samples_remainder;
	int64_t prev_tstates; /* for computing tstates elapsed since last OUT cb */
	uint8_t addr_latch;
	uint8_t beeper;
};

int ay_player_init(struct ay_player *, int sample_rate);
int ay_player_play_file(struct ay_player *, struct ay_file *, int track);
int ay_player_fill_buffer(struct ay_player *, int32_t *bufL, int32_t *bufR, int num_samples);
void ay_player_out(struct ay_player *, uint16_t port, uint8_t data, int cur_tstates);
uint8_t ay_player_in(struct ay_player *, uint16_t port, int cur_tstates);
