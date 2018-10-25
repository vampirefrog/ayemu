all: test

test: test.o tools.o ay_file.o ay_player.o emu2149.o vgm.o z80emu/z80emu.o
	gcc $^ -o $@ -lao

%.o: %.c
	gcc -g -Wall -Iz80emu -c $< -o $@

clean:
	rm -f *.o test
