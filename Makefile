all: test ay.js

test: test.o tools.o ay_file.o ay_player.o emu2149.o vgm.o z80emu/z80emu.o
	gcc $^ -o $@ -lao

%.o: %.c
	gcc -g -Wall -Iz80emu -c $< -o $@

ay.js: ay_file.c ay_player.c emu2149.c z80emu/z80emu.c ay-js.c
	emcc $^ -Wall -O2 -s EXPORTED_FUNCTIONS='["_init_player", "_play_file", "_render_samples", "_get_num_songs", "_get_song_name"]' -s EXTRA_EXPORTED_RUNTIME_METHODS='["ccall", "cwrap"]' -o $@

clean:
	rm -f *.o ay.js *.wasm *.map test
