NOSTOS PROJECT - OS 2026/2027
authors:
    names: Daiki Fabius Kudo, Elvar Nói Leistner
    logins: daikifabius.kudo, elvarnoi.Leistner

Usage:

make odysseus, make island, make ithaca     : builds one executable (in the project root).
make all                                    : builds all three.
make clean                                  : removes the objects and executables.

Execution examples:
Assuming there exists the directories dat/ and 'Island Stock'/ for the config files
Normal:

./ithaca dat/t_ithaca.dat dat/voyages.dat 
./odysseus dat/t_odysseus.dat
./island dat/t_island.dat Island\ Stock/Aeaea.db 


With valgrind (full):

valgrind --leak-check=full --show-leak-kinds=all ./ithaca dat/t_ithaca.dat dat/voyages.dat 
valgrind --leak-check=full --show-leak-kinds=all ./odysseus dat/t_odysseus.dat
valgrind --leak-check=full --show-leak-kinds=all ./island dat/t_island.dat 'Island Stock'/Aeaea.db 
