# make odysseus, make island or make ithaca builds one executable (in the project root).
# make all builds all three.
# make clean removes the objects and executables.

#so includes dont need folder names
UTIL     = src/util
ODY      = src/odysseus
ITH      = src/ithaca
ISL      = src/island
SPHRAGIS = src/SPHRAGIS\ Library-20260917

CFLAGS = -Wall -Wextra -g -Isrc -I$(UTIL) -I$(ODY) -I$(ITH) -I$(ISL)

all: build_dir odysseus island ithaca

# Ensures the build directory exists before compiling
build_dir:
	mkdir -p build

# ---------- util  ----------
build/custom_io.o: $(UTIL)/custom_io.c $(UTIL)/custom_io.h
	gcc $(CFLAGS) -c $(UTIL)/custom_io.c -o build/custom_io.o

build/protocol.o: $(UTIL)/protocol.c $(UTIL)/protocol.h
	gcc $(CFLAGS) -c $(UTIL)/protocol.c -o build/protocol.o

build/socket_utils.o: $(UTIL)/socket_utils.c $(UTIL)/socket_utils.h
	gcc $(CFLAGS) -c $(UTIL)/socket_utils.c -o build/socket_utils.o

# ---------- odysseus ----------
build/ship_config.o: $(ODY)/ship_config.c $(ODY)/ship_config.h $(UTIL)/custom_io.h
	gcc $(CFLAGS) -c $(ODY)/ship_config.c -o build/ship_config.o

build/ship_actions.o: $(ODY)/ship_actions.c $(ODY)/ship_actions.h $(ODY)/ship_config.h $(UTIL)/protocol.h $(UTIL)/socket_utils.h $(UTIL)/custom_io.h
	gcc $(CFLAGS) -c $(ODY)/ship_actions.c -o build/ship_actions.o

build/commands.o: $(ODY)/commands.c $(ODY)/commands.h $(ODY)/ship_actions.h $(ODY)/ship_config.h
	gcc $(CFLAGS) -c $(ODY)/commands.c -o build/commands.o

build/odysseus.o: src/odysseus.c $(ODY)/ship_config.h $(ODY)/ship_actions.h $(ODY)/commands.h $(UTIL)/custom_io.h
	gcc $(CFLAGS) -c src/odysseus.c -o build/odysseus.o

# ---------- ithaca ----------
build/voyage_config.o: $(ITH)/voyage_config.c $(ITH)/voyage_config.h $(UTIL)/custom_io.h
	gcc $(CFLAGS) -c $(ITH)/voyage_config.c -o build/voyage_config.o

build/voyage_service.o: $(ITH)/voyage_service.c $(ITH)/voyage_service.h $(ITH)/voyage_config.h $(UTIL)/protocol.h $(UTIL)/socket_utils.h
	gcc $(CFLAGS) -c $(ITH)/voyage_service.c -o build/voyage_service.o

build/ithaca.o: src/ithaca.c $(ITH)/voyage_config.h $(ITH)/voyage_service.h $(UTIL)/socket_utils.h $(UTIL)/custom_io.h
	gcc $(CFLAGS) -c src/ithaca.c -o build/ithaca.o

# ---------- island ----------
build/port_config.o: $(ISL)/port_config.c $(ISL)/port_config.h $(UTIL)/custom_io.h $(SPHRAGIS)/sphragis.h
	gcc $(CFLAGS) -c $(ISL)/port_config.c -o build/port_config.o

build/port_control.o: $(ISL)/port_control.c $(ISL)/port_control.h $(UTIL)/semaphore_v2.h
	gcc $(CFLAGS) -c $(ISL)/port_control.c -o build/port_control.o

build/island_stock.o: $(ISL)/island_stock.c $(ISL)/island_stock.h $(ISL)/port_config.h $(UTIL)/custom_io.h
	gcc $(CFLAGS) -c $(ISL)/island_stock.c -o build/island_stock.o

build/island_service.o: $(ISL)/island_service.c $(ISL)/island_service.h $(ISL)/port_control.h $(ISL)/island_stock.h $(ISL)/port_config.h $(UTIL)/protocol.h $(UTIL)/socket_utils.h $(UTIL)/custom_io.h
	gcc $(CFLAGS) -c $(ISL)/island_service.c -o build/island_service.o

build/island.o: src/island.c $(ISL)/port_config.h $(ISL)/port_control.h $(ISL)/island_stock.h $(ISL)/island_service.h $(UTIL)/socket_utils.h $(SPHRAGIS)/sphragis.h
	gcc $(CFLAGS) -c src/island.c -o build/island.o

# ---------- executables  ----------
odysseus: build/odysseus.o build/ship_config.o build/commands.o build/ship_actions.o build/protocol.o build/socket_utils.o build/custom_io.o $(SPHRAGIS)/sphragis.o
	gcc build/odysseus.o build/ship_config.o build/commands.o build/ship_actions.o build/protocol.o build/socket_utils.o build/custom_io.o $(SPHRAGIS)/sphragis.o $(CFLAGS) -o odysseus -lpthread

island: build/island.o build/port_config.o build/port_control.o build/island_stock.o build/island_service.o build/protocol.o build/socket_utils.o build/custom_io.o $(SPHRAGIS)/sphragis.o
	gcc build/island.o build/port_config.o build/port_control.o build/island_stock.o build/island_service.o build/protocol.o build/socket_utils.o build/custom_io.o $(SPHRAGIS)/sphragis.o $(CFLAGS) -o island -lpthread

ithaca: build/ithaca.o build/voyage_config.o build/voyage_service.o build/protocol.o build/custom_io.o build/socket_utils.o
	gcc build/ithaca.o build/voyage_config.o build/voyage_service.o build/protocol.o build/custom_io.o build/socket_utils.o $(CFLAGS) -o ithaca -lpthread

clean:
	rm -f build/*.o odysseus island ithaca

.PHONY: all build_dir odysseus island ithaca clean