# make odysseus, make island or make ithaca builds one executable.
# make all builds all three.
# make clean removes the objects and executables.

CFLAGS = -Wall -Wextra -g -Isrc -Isrc/island
SPHRAGIS = src/SPHRAGIS\ Library-20260917

all: build_dir odysseus island ithaca

# Ensures the build directory exists before compiling
build_dir:
	mkdir -p build

# ---------- shared modules ----------
build/custom_dynamic.o: src/custom_dynamic.c src/custom_dynamic.h
	gcc $(CFLAGS) -c src/custom_dynamic.c -o build/custom_dynamic.o

build/protocol.o: src/protocol.c src/protocol.h
	gcc $(CFLAGS) -c src/protocol.c -o build/protocol.o

build/socket_utils.o: src/socket_utils.c src/socket_utils.h
	gcc $(CFLAGS) -c src/socket_utils.c -o build/socket_utils.o

# ---------- odysseus ----------
build/ship_config.o: src/ship_config.c src/ship_config.h src/custom_dynamic.h
	gcc $(CFLAGS) -c src/ship_config.c -o build/ship_config.o

build/ship_actions.o: src/ship_actions.c src/ship_actions.h src/ship_config.h src/protocol.h src/socket_utils.h src/custom_dynamic.h
	gcc $(CFLAGS) -c src/ship_actions.c -o build/ship_actions.o

build/commands.o: src/commands.c src/commands.h src/ship_actions.h src/ship_config.h
	gcc $(CFLAGS) -c src/commands.c -o build/commands.o

build/odysseus.o: src/odysseus.c src/ship_config.h src/ship_actions.h src/commands.h src/custom_dynamic.h
	gcc $(CFLAGS) -c src/odysseus.c -o build/odysseus.o

# ---------- ithaca ----------
build/voyage_config.o: src/voyage_config.c src/voyage_config.h src/custom_dynamic.h
	gcc $(CFLAGS) -c src/voyage_config.c -o build/voyage_config.o

build/voyage_service.o: src/voyage_service.c src/voyage_service.h src/voyage_config.h src/protocol.h src/socket_utils.h
	gcc $(CFLAGS) -c src/voyage_service.c -o build/voyage_service.o

build/ithaca.o: src/ithaca.c src/voyage_config.h src/voyage_service.h src/socket_utils.h src/custom_dynamic.h
	gcc $(CFLAGS) -c src/ithaca.c -o build/ithaca.o

# ---------- island ----------
build/port_config.o: src/port_config.c src/port_config.h src/custom_dynamic.h $(SPHRAGIS)/sphragis.h
	gcc $(CFLAGS) -c src/port_config.c -o build/port_config.o

# the only file that includes the course semaphore library
build/port_control.o: src/island/port_control.c src/island/port_control.h src/semaphore_v2.h
	gcc $(CFLAGS) -c src/island/port_control.c -o build/port_control.o

build/island_stock.o: src/island/island_stock.c src/island/island_stock.h src/port_config.h src/custom_dynamic.h
	gcc $(CFLAGS) -c src/island/island_stock.c -o build/island_stock.o

build/island_service.o: src/island/island_service.c src/island/island_service.h src/island/port_control.h src/island/island_stock.h src/port_config.h src/protocol.h src/socket_utils.h src/custom_dynamic.h
	gcc $(CFLAGS) -c src/island/island_service.c -o build/island_service.o

build/island.o: src/island.c src/port_config.h src/island/port_control.h src/island/island_stock.h src/island/island_service.h src/socket_utils.h $(SPHRAGIS)/sphragis.h
	gcc $(CFLAGS) -c src/island.c -o build/island.o

# ---------- executables ----------
odysseus: build/odysseus.o build/ship_config.o build/commands.o build/ship_actions.o build/protocol.o build/socket_utils.o build/custom_dynamic.o
	gcc build/odysseus.o build/ship_config.o build/commands.o build/ship_actions.o build/protocol.o build/socket_utils.o build/custom_dynamic.o $(CFLAGS) -o build/odysseus -lpthread

island: build/island.o build/port_config.o build/port_control.o build/island_stock.o build/island_service.o build/protocol.o build/socket_utils.o build/custom_dynamic.o $(SPHRAGIS)/sphragis.o
	gcc build/island.o build/port_config.o build/port_control.o build/island_stock.o build/island_service.o build/protocol.o build/socket_utils.o build/custom_dynamic.o $(SPHRAGIS)/sphragis.o $(CFLAGS) -o build/island -lpthread

ithaca: build/ithaca.o build/voyage_config.o build/voyage_service.o build/protocol.o build/custom_dynamic.o build/socket_utils.o
	gcc build/ithaca.o build/voyage_config.o build/voyage_service.o build/protocol.o build/custom_dynamic.o build/socket_utils.o $(CFLAGS) -o build/ithaca -lpthread

clean:
	rm -f build/*.o build/odysseus build/island build/ithaca

.PHONY: all build_dir odysseus island ithaca clean