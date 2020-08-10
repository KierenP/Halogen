
#Thanks to Andrew Grant (ETHEREAL) 
#I used his makefile as a model for my own

CC = g++
EXE = Halogen
SRC = *.cpp
OUTPUT = ..\..\..\$(EXE)$(VER)

WFLAGS = -Wall -Wextra -Wshadow -std=c++14
RFLAGS = -O3 $(WFLAGS) -DNDEBUG -flto
CFLAGS = -O3 $(WFLAGS) -DNDEBUG -flto -march=native

POPCNTFLAGS = -DUSE_POPCNT

default:
	$(CC) $(CFLAGS) $(SRC) -o $(EXE)

release: 
	mkdir $(OUTPUT)
	$(CC) $(RFLAGS) $(SRC) -o $(OUTPUT)\$(EXE)$(VER)-x64-nopopcnt.exe
	$(CC) $(RFLAGS) $(SRC) $(POPCNTFLAGS) -o $(OUTPUT)\$(EXE)$(VER)-x64-popcnt.exe