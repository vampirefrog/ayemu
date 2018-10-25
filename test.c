	#include <stdio.h>
#include <string.h>
#include <ao/ao.h>
#include <signal.h>

#include "ay_file.h"
#include "ay_player.h"
#include "tools.h"
#include "vgm.h"

#define BUF_SIZE 4096
#define SAMPLE_RATE 44100

int running = 1;

void sighandler(int signum) {
	signal(signum, NULL);
	printf("Caught signal %d, coming out...\n", signum);
	running = 0;
}

int main(int argc, char **argv) {
	size_t s = 0;
	uint8_t *ay_data = load_file(argv[1], &s);
	printf("Loaded %s: %luB\n", argv[1], s);

	struct ay_file f;
	int r = ay_file_load(&f, ay_data, s);
	if(r < 0) {
		fprintf(stderr, "Error loading %s\n", argv[1]);
		return 1;
	}
	ay_file_dump(&f);

	struct ay_player p;
	ay_player_init(&p, SAMPLE_RATE);
	struct vgm_logger l;
	vgm_logger_begin(&l, "test.vgm");
	p.logger = &l;
	ay_player_play_file(&p, &f, argc >= 3 ? atoi(argv[2]) : f.first_song);

	ao_device *device;
	ao_sample_format format;
	int default_driver;

	/* -- Initialize -- */

	ao_initialize();

	/* -- Setup for default driver -- */

	default_driver = ao_default_driver_id();

	memset(&format, 0, sizeof(format));
	format.bits = 16;
	format.channels = 2;
	format.rate = SAMPLE_RATE;
	format.byte_format = AO_FMT_LITTLE;

	/* -- Open driver -- */
	device = ao_open_live(default_driver, &format, NULL /* no options */);
	if (device == NULL) {
		fprintf(stderr, "Error opening device.\n");
		return 1;
	}

	signal(SIGINT, sighandler);

	/* -- Play some stuff -- */
	while(running) {
		int32_t bufL[BUF_SIZE], bufR[BUF_SIZE];
		int16_t buf[BUF_SIZE * 2];
		ay_player_fill_buffer(&p, bufL, bufR, BUF_SIZE);
		for(int i = 0; i < BUF_SIZE; i++) {
			buf[i * 2] = bufL[i];
			buf[i * 2 + 1] = bufR[i];
		}
		ao_play(device, (char *)buf, BUF_SIZE * format.channels * format.bits / 8);
	}

	vgm_logger_end(&l);

	/* -- Close and shutdown -- */
	ao_close(device);

	ao_shutdown();

	return 0;
}
