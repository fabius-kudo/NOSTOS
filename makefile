# make Odysseus, make Island or make Itchaca builds one executable.
# make all builds all three.
# make clean removes the objects and executables.

build/custom_string.o: src/custom_string.c src/custom_string.h
	gcc src/custom_string.c -Wall -Wextra -g -c -o build/custom_string.o

build/Odysseus.o: src/Odysseus.c src/Odysseus.h src/custom_string.h
	gcc src/Odysseus.c -Wall -Wextra -g -c -o build/Odysseus.o

build/Island.o: src/Island.c src/SPHRAGIS Library-20260917/sphragis.h
	gcc src/Island.c -Wall -Wextra -g -c -o build/Island.o

build/Itchaca.o: src/Itchaca.c
	gcc src/Itchaca.c -Wall -Wextra -g -c -o build/Itchaca.o

Odysseus: build/Odysseus.o build/custom_string.o
	gcc build/Odysseus.o build/custom_string.o -g -o build/Odysseus -lpthread

Island: build/Island.o
	gcc build/Island.o -g -o build/Island -lpthread

Itchaca: build/Itchaca.o
	gcc build/Itchaca.o -g -o build/Itchaca -lpthread

all: Odysseus Island Itchaca

clean:
	rm -f build/*.o build/Odysseus build/Island build/Itchaca