#include <string.h>
#include <stdio.h>

#include "ay_player.h"
#include "z80emu/z80emu.h"

int ay_player_init(struct ay_player *p, int rate) {
	p->sample_rate = rate;
	p->cpu_clock = 3546900;
	p->int_tstates = 70908; // t-states between two frames

	return 0;
}

// stolen from aylet
static const unsigned char intz[] = {
	0xf3,		/* di */
	0xcd,0,0,	/* call init */
	0xed,0x5e,	/* loop: im 2 */
	0xfb,		/* ei */
	0x76,		/* halt */
	0x18,0xfa	/* jr loop */
};
static const unsigned char intnz[] = {
	0xf3,		/* di */
	0xcd,0,0,	/* call init */
	0xed,0x56,	/* loop: im 1 */
	0xfb,		/* ei */
	0x76,		/* halt */
	0xcd,0,0,	/* call interrupt */
	0x18,0xf7	/* jr loop */
};
#define ENDIAN_SWAP(x) (((x) >> 8) | (((x) << 8) & 0xffff))
int ay_player_play_file(struct ay_player *p, struct ay_file *f, int track) {
	p->file = f;

	p->int_remainder = 0;
	p->tstate_remainder = 0;
	p->samples_remainder = 0;
	p->bufL = p->bufR = 0;

	p->addr_latch = 0;

	memset(p->memory + 0x0000, 0xc9, 0x0100);
	memset(p->memory + 0x0100, 0xff, 0x3f00);
	memset(p->memory + 0x4000, 0x00, 0xc000);
	p->memory[0x38] = 0xfb; /* ei */

	struct ay_file_song_data *sd = ay_file_get_song_data(f, track);
	struct ay_file_point *pt = ay_file_get_song_point(f, sd);
	uint16_t *a = ay_file_get_song_addresses(f, sd);

	int16_t ourinit = pt->init ? pt->init : *a;

	if(!pt->interrupt) {
		memcpy(p->memory, intz, sizeof(intz));
	} else {
  		memcpy(p->memory, intnz, sizeof(intnz));
  		p->memory[ 9] = pt->interrupt >> 8;
  		p->memory[10] = pt->interrupt & 0xff;
	}

	p->memory[2] = ourinit >> 8;
	p->memory[3] = ourinit & 0xff;

	/* now put the memory blocks in place */
	while(*a) {
		int addr = ENDIAN_SWAP(*a); a++;
		int len  = ENDIAN_SWAP(*a); a++;
		int ofs  = ENDIAN_SWAP(*a);
		if(ofs >= 0x8000) ofs -= 0x10000;
		uint8_t *ptr = (uint8_t *)a + ofs;
		a++;

		memcpy(p->memory + addr, ptr, len);
	}

	memset(&p->z80, 0, sizeof(p->z80));
	Z80Reset(&p->z80);

	p->z80.registers.byte[Z80_B] = p->z80.registers.byte[Z80_D] = p->z80.registers.byte[Z80_H] = p->z80.registers.byte[Z80_A] = sd->hi_reg;
	p->z80.registers.byte[Z80_C] = p->z80.registers.byte[Z80_E] = p->z80.registers.byte[Z80_L] = p->z80.registers.byte[Z80_F] = sd->lo_reg;
	p->z80.registers.word[Z80_SP] = ENDIAN_SWAP(pt->stack);
	p->z80.i = 3;
	p->z80.pc = 0;

	PSG_init(&p->psg, p->cpu_clock / 2, p->sample_rate);
	PSG_reset(&p->psg);
	PSG_setVolumeMode(&p->psg, 2);
	PSG_setMask(&p->psg, 0);
	PSG_setFlags(&p->psg, EMU2149_ZX_STEREO);

	return 0;
}

static int ay_player_render_tstates(struct ay_player *p, int tstates) {
	int64_t x = tstates * (int64_t)p->sample_rate + p->samples_remainder;
	int64_t samples = x / p->cpu_clock;
	if(samples > p->buf_remaining)
		samples = p->buf_remaining;
	p->samples_remainder = x - samples * p->cpu_clock;

	if(samples > 0) {
		int32_t *buf[2] = { p->bufL, p->bufR };
		PSG_calc_stereo(&p->psg, buf, samples);
#ifndef __EMSCRIPTEN__
		if(p->logger)
			vgm_logger_wait(p->logger, samples);
#endif

		for(int i = 0; i < samples; i++) {
			int x;

			x = p->bufL[i];
			x += p->beeper ? 10000 : 0;
			if(x > 32767) x = 32767;
			p->bufL[i] = x;

			x = p->bufR[i];
			x += p->beeper ? 10000 : 0;
			if(x > 32767) x = 32767;
			p->bufR[i] = x;
		}

		p->bufL += samples;
		p->bufR += samples;
		p->buf_remaining -= samples;
	}

	return samples;
}

static int ay_player_emulate_tstates(struct ay_player *p, int tstates) {
	p->prev_tstates = 0;
	int cycles = Z80Emulate(&p->z80, tstates, p);
	ay_player_render_tstates(p, cycles - p->prev_tstates);
	return cycles;
}

int ay_player_fill_buffer(struct ay_player *p, int32_t *bufL, int32_t *bufR, int num_samples) {
	p->buf_remaining = num_samples;
	p->bufL = bufL;
	p->bufR = bufR;

	int64_t x = (int64_t)num_samples * p->cpu_clock + p->tstate_remainder;
	int tstates = x / p->sample_rate;
	p->tstate_remainder = x - tstates * p->sample_rate;

	int prev = 0, i;
	for(i = p->int_remainder; i < tstates; i += p->int_tstates) {
		prev += ay_player_emulate_tstates(p, i - prev);
		Z80Interrupt(&p->z80, 0, p);
	}

	prev += ay_player_emulate_tstates(p, tstates - prev);
	p->int_remainder = i - prev;

	return 0;
}

void ay_player_write_addr(struct ay_player *p, uint8_t addr) {
	p->addr_latch = addr;
}

void ay_player_out(struct ay_player *p, uint16_t port, uint8_t data, int cur_tstates) {
	int tstates = cur_tstates - p->prev_tstates;
	p->prev_tstates = cur_tstates;

	ay_player_render_tstates(p, tstates);

	if(port == 0xfffd) {
		p->addr_latch = data;
	} else if(port == 0xbffd) {
		PSG_writeReg(&p->psg, p->addr_latch, data);
#ifndef __EMSCRIPTEN__
		if(p->logger)
			vgm_logger_write_ay(p->logger, p->addr_latch, data);
#endif
	} else if((port & 0xff) == 0xfe) {
		p->beeper = data & 0x10;
	} else {
		// printf("Unknown port write 0x%04x = 0x%02x\n", port, data);
	}
}

uint8_t ay_player_in(struct ay_player *p, uint16_t port, int elapsed_cycles) {
	if(port == 0xbffd)
		return PSG_readReg(&p->psg, p->addr_latch);
	if((port & 0xff) == 0xfe)
		return p->beeper;

	return 0xff;
}
