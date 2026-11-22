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

montserrat:
./ithaca dat/ithaca.dat dat/voyages.dat 
./odysseus dat/polyphemus.dat
./odysseus dat/agamemnon.dat
./odysseus dat/telemachus.dat

matagalls:
./island dat/aeaea.dat Island\ Stock/Aeaea.db
./island dat/scheria.dat Island\ Stock/Scheria.db 
./island dat/aeolia.dat Island\ Stock/Aeolia.db 
./island dat/ogygia.dat Island\ Stock/Ogygia.db 
./island dat/thrinacia.dat Island\ Stock/Thrinacia.db 


With valgrind (full):

valgrind --leak-check=full --show-leak-kinds=all ./ithaca dat/t_ithaca.dat dat/voyages.dat 
valgrind --leak-check=full --show-leak-kinds=all ./odysseus dat/t_odysseus.dat
valgrind --leak-check=full --show-leak-kinds=all ./island dat/t_island.dat 'Island Stock'/Aeaea.db 
