# make odysseus, make island or make ithaca builds one executable.
# make all builds all three.
# make clean removes the objects and executables.

CFLAGS = -Wall -Wextra -g 

build/custom_dynamic.o: src/custom_dynamic.c src/custom_dynamic.h
	gcc $(CFLAGS) -c src/custom_dynamic.c -o build/custom_dynamic.o

build/odysseus_parser.o: src/odysseus_parser.c src/odysseus_parser.h src/custom_dynamic.h
	gcc $(CFLAGS) -c src/odysseus_parser.c -o build/odysseus_parser.o

build/odysseus_command.o: src/odysseus_command.c src/odysseus_command.h src/custom_dynamic.h
	gcc $(CFLAGS) -c src/odysseus_command.c -o build/odysseus_command.o

build/odysseus.o: src/odysseus_main.c src/odysseus_parser.h src/odysseus_command.h src/custom_dynamic.h
	gcc $(CFLAGS) -c src/odysseus_main.c -o build/odysseus.o

build/island_parser.o: src/island_parser.c src/island_parser.h
	gcc $(CFLAGS) -c src/island_parser.c -o build/island_parser.o

build/island.o: src/island_main.c src/SPHRAGIS\ Library-20260917/sphragis.h src/island_parser.h
	gcc $(CFLAGS) -c src/island_main.c -o build/island.o

build/ithaca_parser.o: src/ithaca_parser.c src/ithaca_parser.h src/custom_dynamic.h
	gcc $(CFLAGS) -c src/ithaca_parser.c -o build/ithaca_parser.o

build/ithaca.o: src/ithaca_main.c src/ithaca_parser.h
	gcc $(CFLAGS) -c src/ithaca_main.c -o build/ithaca.o

odysseus: build/odysseus.o build/odysseus_parser.o build/odysseus_command.o build/custom_dynamic.o
	gcc build/odysseus.o build/odysseus_parser.o build/odysseus_command.o build/custom_dynamic.o -g -o build/odysseus -lpthread

island: build/island.o build/island_parser.o src/SPHRAGIS\ Library-20260917/sphragis.o
	gcc build/island.o build/island_parser.o src/SPHRAGIS\ Library-20260917/sphragis.o -g -o build/island -lpthread

ithaca: build/ithaca.o build/ithaca_parser.o build/custom_dynamic.o
	gcc build/ithaca.o build/ithaca_parser.o build/custom_dynamic.o -g -o build/ithaca -lpthread

all: odysseus island ithaca

clean:
	rm -f build/*.o build/odysseus build/island build/ithaca