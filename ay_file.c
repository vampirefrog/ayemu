#include <stdio.h>
#include <string.h>

#include "ay_file.h"

int ay_file_load(struct ay_file *f, uint8_t *data, size_t size) {
	if(size < 8) return -1;
	if(strncmp((char *)data, "ZXAYEMUL", 8)) return -1;

	f->data = data;
	f->data_size = size;
	f->file_version = data[8];
	f->player_version = data[9];
#define READ_PTR(field, typ, n) field = (typ)(data[n] || data[n+1] ? &data[n + (data[n] << 8) + data[n+1]] : 0)
	READ_PTR(f->special_player, void *, 10);
	READ_PTR(f->author, uint8_t *, 12);
	READ_PTR(f->misc, uint8_t *, 14);
	f->num_songs = data[16] + 1;
	f->first_song = data[17];
	READ_PTR(f->song_structures, struct ay_file_song_structure *, 18);
#undef READ_PTR

	return 0;
}

char *ay_file_get_author(struct ay_file *f) {
	return (char *)f->author;
}

char *ay_file_get_misc(struct ay_file *f) {
	return (char *)f->misc;
}

#define ENDIAN_SWAP(x) (((x) >> 8) | (((x) << 8) & 0xffff))
#define RELATIVE_PTR(x) ((uint8_t *)x + ENDIAN_SWAP(*x))
struct ay_file_song_data *ay_file_get_song_data(struct ay_file *f, int s) {
	if(s >= f->num_songs) return 0;

	uint8_t *ptr = (uint8_t *)&f->song_structures[s].p_song_data;
	int ofs = ENDIAN_SWAP(f->song_structures[s].p_song_data);
	if(ofs >= 0x8000) ofs -= 0x10000;

	return ofs ? (struct ay_file_song_data *)(ptr + ofs) : 0;
}

char *ay_file_get_song_name(struct ay_file *f, int s) {
	if(s >= f->num_songs) return 0;

	uint8_t *ptr = (uint8_t *)&f->song_structures[s].p_song_name;
	int ofs = ENDIAN_SWAP(f->song_structures[s].p_song_name);
	if(ofs >= 0x8000) ofs -= 0x10000;

	return ofs ? (char *)(ptr + ofs) : 0;
}

struct ay_file_point *ay_file_get_song_point(struct ay_file *f, struct ay_file_song_data *sd) {
	uint8_t *ptr = (uint8_t *)&sd->p_points;
	int ofs = ENDIAN_SWAP(sd->p_points);
	if(ofs >= 0x8000) ofs -= 0x10000;
	return ofs ? (struct ay_file_point *)(ptr + ofs) : 0;
}

uint16_t *ay_file_get_song_addresses(struct ay_file *f, struct ay_file_song_data *sd) {
	uint8_t *ptr = (uint8_t *)&sd->p_addresses;
	int ofs = ENDIAN_SWAP(sd->p_addresses);
	if(ofs >= 0x8000) ofs -= 0x10000;
	return ofs ? (uint16_t *)(ptr + ofs) : 0;
}

#ifndef __EMSCRIPTEN__
void ay_file_dump(struct ay_file *f) {
	printf("file_version=%d\n", f->file_version);
	printf("player_version=%d\n", f->player_version);
	printf("p_special_player=%p\n", f->special_player);
	printf("p_author=%p\n", f->author);
	printf("Author: %s\n", ay_file_get_author(f));
	printf("p_misc=%p\n", f->misc);
	printf("Misc: %s\n", ay_file_get_misc(f));
	printf("num_songs=%d\n", f->num_songs);
	printf("first_song=%d\n", f->first_song);
	printf("p_song_structures=%p\n", f->song_structures);

	for(int i = 0; i < f->num_songs; i++) {
		struct ay_file_song_data *sd = ay_file_get_song_data(f, i);
		char *song_name = ay_file_get_song_name(f, i);
		printf("Song %d: %s\n", i, song_name);
		printf("\ta_chan=%d b_chan=%d c_chan=%d noise=%d\n", sd->a_chan, sd->b_chan, sd->c_chan, sd->noise);
		printf("\tsong_length=%d (%.2fs)\n", ENDIAN_SWAP(sd->song_length), (float)ENDIAN_SWAP(sd->song_length) / 50);
		printf("\tfade_length=%d (%.2fs)\n", ENDIAN_SWAP(sd->fade_length), (float)ENDIAN_SWAP(sd->fade_length) / 50);
		printf("\thi_reg=0x%02x lo_reg=0x%02x\n", sd->hi_reg, sd->lo_reg);
		printf("\tp_points=0x%04x p_addresses=0x%04x\n", ENDIAN_SWAP(sd->p_points), ENDIAN_SWAP(sd->p_addresses));

		struct ay_file_point *p = ay_file_get_song_point(f, sd);
		printf("\tPoint stack=%04x init=%04x inter=%04x\n", ENDIAN_SWAP(p->stack), ENDIAN_SWAP(p->init), ENDIAN_SWAP(p->interrupt));

		uint16_t *a = ay_file_get_song_addresses(f, sd);
		printf("a=%p *a=%04x\n", a, *a);
		while(*a) {
			printf("\t%04x, ", ENDIAN_SWAP(*a)); a++;
			printf("%04x, ",   ENDIAN_SWAP(*a)); a++;
			printf("%04lx (%d)\n",   RELATIVE_PTR(a) - f->data, ENDIAN_SWAP(*a)); a++;
		}
	}
}
#endif