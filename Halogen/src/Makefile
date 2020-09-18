CC = g++
EXE = Halogen
SRC = *.cpp
OUTPUT = ..\..\..\$(EXE)$(VER)

WFLAGS = -Wall -Wextra -Wshadow -std=c++14
CFLAGS = -O3 $(WFLAGS) -DNDEBUG -flto -march=native

POPCNTFLAGS = -DGCC_USE_POPCNT

default:
	$(CC) $(CFLAGS) $(SRC) $(POPCNTFLAGS) -o $(EXE)