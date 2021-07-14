CC 		= g++
SRC 	= *.cpp Pyrrhic/tbprobe.cpp
OUTPUT 	= ../../../$(EXE)$(VER)
EXE 	= Halogen
LIBS    = -Wl,--whole-archive -lpthread -Wl,--no-whole-archive -lm

WFLAGS = -Wall -Wextra -Wshadow -std=c++17
CFLAGS = -O3 $(WFLAGS) -DNDEBUG -flto -march=native 
RFLAGS = -O3 $(WFLAGS) -DNDEBUG -flto -static -fno-devirtualize -fno-use-linker-plugin
PGOFLAGS = -O3 $(WFLAGS) -DNDEBUG -flto -march=native

POPCNTFLAGS = -DUSE_POPCNT -msse3 -mpopcnt
PEXTFLAGS   = $(POPCNTFLAGS) -DUSE_PEXT -mbmi2
AVX2FLAGS   = -DUSE_AVX2 -msse -msse3 -mpopcnt -mavx2 -msse4.1 -mssse3 -msse2

default:
	$(CC) $(CFLAGS) $(SRC) $(LIBS) $(POPCNTFLAGS) -o $(EXE)

pgo:
	rm -f *.gcda
	$(CC) -fprofile-generate $(PGOFLAGS) $(SRC) $(LIBS) $(POPCNTFLAGS) -o $(EXE)
	./$(EXE) bench 12
	$(CC) -fprofile-use $(PGOFLAGS) $(SRC) $(LIBS) $(POPCNTFLAGS) -o $(EXE)
	rm -f *.gcda
	
release:
	$(CC) $(RFLAGS) $(SRC) $(LIBS) -o $(EXE)$(VER)-x64-nopopcnt.exe
	$(CC) $(RFLAGS) $(SRC) $(LIBS) $(POPCNTFLAGS) -o $(EXE)$(VER)-x64-popcnt.exe
	$(CC) $(RFLAGS) $(SRC) $(LIBS) $(PEXTFLAGS) -o $(EXE)$(VER)-x64-pext.exe
	$(CC) $(RFLAGS) $(SRC) $(LIBS) $(AVX2FLAGS) $(POPCNTFLAGS) -o $(EXE)$(VER)-x64-popcnt-avx2.exe
	$(CC) $(RFLAGS) $(SRC) $(LIBS) $(AVX2FLAGS) $(PEXTFLAGS) -o $(EXE)$(VER)-x64-pext-avx2.exe
	
sanitize:
	$(CC) -O0 -fsanitize=undefined $(WFLAGS) $(SRC) $(LIBS) -o $(EXE)_undefined.exe
	$(CC) -O0 -fsanitize=address   $(WFLAGS) $(SRC) $(LIBS) -o $(EXE)_address.exe

debug:
	$(CC) -O0 $(WFLAGS) $(SRC) $(LIBS) -o $(EXE)