#pragma once

#include "emutypes.h"

#define EMU2149_API

#define EMU2149_VOL_DEFAULT 1
#define EMU2149_VOL_YM2149 0
#define EMU2149_VOL_AY_3_8910 1

#define EMU2149_ZX_STEREO			0x80

#define PSG_MASK_CH(x) (1<<(x))

typedef struct {
	/* Volume Table */
	e_uint32 *voltbl;

	e_uint8 reg[0x20];
	e_int32 out;
	e_int32 cout[3];

	e_uint32 clk, rate, base_incr, quality;

	e_uint32 count[3];
	e_uint32 volume[3];
	e_uint32 freq[3];
	e_uint32 edge[3];
	e_uint32 tmask[3];
	e_uint32 nmask[3];
	e_uint32 mask;
	e_uint32 stereo_mask[3];

	e_uint32 base_count;

	e_uint32 env_volume;
	e_uint32 env_ptr;
	e_uint32 env_face;

	e_uint32 env_continue;
	e_uint32 env_attack;
	e_uint32 env_alternate;
	e_uint32 env_hold;
	e_uint32 env_pause;
	e_uint32 env_reset;

	e_uint32 env_freq;
	e_uint32 env_count;

	e_uint32 noise_seed;
	e_uint32 noise_count;
	e_uint32 noise_freq;

	/* rate converter */
	e_uint32 realstep;
	e_uint32 psgtime;
	e_uint32 psgstep;
	e_int32 prev, next;
	e_int32 sprev[2], snext[2];

	/* I/O Ctrl */
	e_uint32 adr;
} PSG;

void PSG_set_quality(PSG * psg, e_uint32 q);
void PSG_set_clock(PSG * psg, e_uint32 c);
void PSG_set_rate(PSG * psg, e_uint32 r);
void PSG_init(PSG *, e_uint32 clk, e_uint32 rate);
PSG *PSG_new(e_uint32 clk, e_uint32 rate);
void PSG_reset(PSG *);
void PSG_delete(PSG *);
void PSG_writeReg(PSG *, e_uint32 reg, e_uint32 val);
void PSG_writeIO(PSG * psg, e_uint32 adr, e_uint32 val);
e_uint8 PSG_readReg(PSG * psg, e_uint32 reg);
e_uint8 PSG_readIO(PSG * psg);
e_int16 PSG_calc(PSG *);
void PSG_calc_stereo(PSG * psg, e_int32 **out, e_int32 samples);
void PSG_setFlags(PSG * psg, e_uint8 flags);
void PSG_setVolumeMode(PSG * psg, int type);
e_uint32 PSG_setMask(PSG *, e_uint32 mask);
e_uint32 PSG_toggleMask(PSG *, e_uint32 mask);
