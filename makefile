# make Odysseus, make Island or make Itchaca builds one executable.
# make all builds all three.
# make clean removes the objects and executables.

build/custom_dynamic.o: src/custom_dynamic.c src/custom_dynamic.h
	gcc src/custom_dynamic.c -Wall -Wextra -g -c -o build/custom_dynamic.o

build/odysseus_parser.o: src/odysseus_parser.c src/odysseus_parser.h
	gcc src/odysseus_parser.c -Wall -Wextra -g -c -o build/odysseus_parser.o

build/odysseus_command.o: src/odysseus_command.c src/odysseus_command.h
	gcc src/odysseus_command.c -Wall -Wextra -g -c -o build/odysseus_command.o

build/Odysseus.o: src/odysseus_main.c src/odysseus_parser.h src/odysseus_command.h src/custom_dynamic.h
	gcc src/odysseus_main.c -Wall -Wextra -g -c -o build/Odysseus.o

build/island_parser.o: src/island_parser.c src/island_parser.h
	gcc src/island_parser.c -Wall -Wextra -g -c -o build/island_parser.o

build/Island.o: src/island_main.c src/SPHRAGIS\ Library-20260917/sphragis.h src/island_parser.h
	gcc src/island_main.c -Wall -Wextra -g -c -o build/Island.o

build/itchaca_parser.o: src/itchaca_parser.c src/itchaca_parser.h
	gcc src/itchaca_parser.c -Wall -Wextra -g -c -o build/itchaca_parser.o

build/Itchaca.o: src/itchaca_main.c src/itchaca_parser.h
	gcc src/itchaca_main.c -Wall -Wextra -g -c -o build/Itchaca.o

Odysseus: build/Odysseus.o build/odysseus_parser.o build/odysseus_command.o build/custom_dynamic.o
	gcc build/Odysseus.o build/odysseus_parser.o build/odysseus_command.o build/custom_dynamic.o -g -o build/Odysseus -lpthread

Island: build/Island.o build/island_parser.o src/SPHRAGIS\ Library-20260917/sphragis.o
	gcc build/Island.o build/island_parser.o src/SPHRAGIS\ Library-20260917/sphragis.o -g -o build/Island -lpthread

Itchaca: build/Itchaca.o build/itchaca_parser.o
	gcc build/Itchaca.o build/itchaca_parser.o -g -o build/Itchaca -lpthread

all: Odysseus Island Itchaca

clean:
	rm -f build/*.o build/Odysseus build/Island build/Itchaca