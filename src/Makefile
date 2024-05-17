BUILD_DIR := ../build
BINARY_DIR := ../bin
SRC_DIRS := .

EVALURL = https://github.com/KierenP/Halogen-Networks/blob/6f6b5ec1436bf19212fa1667c1962871d51a7fb0/d7d569e5.nn?raw=true
EVALFILE  = d7d569e5.nn

#----------------------------------------------------------------------------------------------------------------------

# Find all the C++ files we want to compile
# I'd use a bash command but not all client have bash
SRCS := \
	BitBoardDefine.cpp \
	BoardState.cpp \
	EvalCache.cpp \
	EvalNet.cpp \
	main.cpp \
	Move.cpp \
	MoveGeneration.cpp \
	MoveList.cpp \
	Network.cpp \
	GameState.cpp \
	Search.cpp \
	SearchData.cpp \
	SearchLimits.cpp \
	StagedMoveGenerator.cpp \
	TimeManage.cpp \
	TranspositionTable.cpp \
	TTEntry.cpp \
	Zobrist.cpp \
	Score.cpp \
	EGTB.cpp \
	History.cpp \
	Magic.cpp \
	Pyrrhic/tbprobe.c

# String substitution for every C/C++ file.
# As an example, hello.cpp turns into ./build/hello.cpp.o
OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)

# String substitution (suffix version without %).
# As an example, ./build/hello.cpp.o turns into ./build/hello.cpp.d
DEPS := $(OBJS:.o=.d)

#----------------------------------------------------------------------------------------------------------------------

WFLAGS = -Wall -Wextra -Wshadow -Wno-interference-size -Wno-unknown-warning-option -Wno-deprecated
AFLAGS = $(WFLAGS) -g -std=c++17 
CFLAGS = -O3 $(AFLAGS) -DNDEBUG -flto -march=native 
RFLAGS = -O3 $(AFLAGS) -DNDEBUG -flto
# only enable Werror on debug builds, so we don't cause issues for people compiling Halogen on a different compiler
SFLAGS = -O1 $(AFLAGS) -Werror -fno-omit-frame-pointer
DFLAGS = -O0 $(AFLAGS) -Werror

POPCNTFLAGS = -DUSE_POPCNT -msse3 -mpopcnt
PEXTFLAGS   = $(POPCNTFLAGS) -DUSE_PEXT -mbmi2
AVX2FLAGS   = -DUSE_AVX2 -msse -msse3 -mpopcnt -mavx2 -msse4.1 -mssse3 -msse2

LDFLAGS   = -Wl,--whole-archive -lpthread -Wl,--no-whole-archive -lm
CXXFLAGS := -fno-exceptions -MMD -MP -DEVALFILE=\"$(EVALFILE)\"

# Detect OS
ifeq ($(OS),Windows_NT) # is Windows_NT on XP, 2000, 7, Vista, 10...
    detected_OS := Windows
else
    detected_OS := $(shell uname)
endif

$(info detected OS is [${detected_OS}])

# Many users on windows use a environment to build so we must link statically
ifeq ($(detected_OS),Windows)
    LDFLAGS += -static
endif

# Detect if CXX is g++ or clang++, in this order.
ifeq '' '$(findstring clang++,$(CXX))'
  	CXXFLAGS += -fconstexpr-ops-limit=1000000000
  	$(info detected compiler is [g++])
else
  	CXXFLAGS += -fconstexpr-steps=100000000
  	$(info detected compiler is [clang++])
endif

$(info LDFLAGS = [${LDFLAGS}])
$(info CXXFLAGS = [${CXXFLAGS}])

# Each target sets particular flags, and then builds the corresponding exe file.

.PHONY: pgo
pgo:
	$(MAKE) pgo-instrumented
	$(MAKE) pgo-compile

.PHONY: pgo-instrumented
pgo-instrumented: CXXFLAGS += $(CFLAGS) $(POPCNTFLAGS) -fprofile-generate
pgo-instrumented: LDFLAGS += -flto -fprofile-generate
pgo-instrumented: EXE = $(BINARY_DIR)/Halogen-pgo-instrumented.exe
pgo-instrumented: binary
	./$(EXE) bench

.PHONY: pgo-compile
pgo-compile: CXXFLAGS += $(CFLAGS) $(POPCNTFLAGS) -fprofile-use
pgo-compile: LDFLAGS += -flto -fprofile-use
pgo-compile: EXE = $(BINARY_DIR)/Halogen-pgo.exe
pgo-compile: binary

.PHONY: native
native: CXXFLAGS += $(CFLAGS) $(POPCNTFLAGS)
native: LDFLAGS += -flto
native: EXE = $(BINARY_DIR)/Halogen-native.exe
native: binary

.PHONY: debug
debug: CXXFLAGS += $(DFLAGS)
debug: EXE = $(BINARY_DIR)/Halogen-debug.exe
debug: binary

.PHONY: sanitize-address
sanitize-address: CXXFLAGS += $(SFLAGS) -fsanitize=address
sanitize-address: LDFLAGS += -fsanitize=address
sanitize-address: EXE = $(BINARY_DIR)/Halogen-sanitize-address.exe
sanitize-address: binary

.PHONY: sanitize-undefined
sanitize-undefined: CXXFLAGS += $(SFLAGS) -fsanitize=undefined
sanitize-undefined: LDFLAGS += -fsanitize=undefined
sanitize-undefined: EXE = $(BINARY_DIR)/Halogen-sanitize-undefined.exe
sanitize-undefined: binary

.PHONY: sanitize-thread
sanitize-thread: CXXFLAGS += $(SFLAGS) -fsanitize=thread
sanitize-thread: LDFLAGS += -fsanitize=thread
sanitize-thread: EXE = $(BINARY_DIR)/Halogen-sanitize-thread.exe
sanitize-thread: binary

.PHONY: nopopcnt
nopopcnt: CXXFLAGS += $(RFLAGS)
nopopcnt: LDFLAGS += -flto -static 
nopopcnt: EXE = $(BINARY_DIR)/Halogen-nopopcnt.exe
nopopcnt: binary

.PHONY: popcnt
popcnt: CXXFLAGS += $(RFLAGS) $(POPCNTFLAGS)
popcnt: LDFLAGS += -flto -static 
popcnt: EXE = $(BINARY_DIR)/Halogen-popcnt.exe
popcnt: binary

.PHONY: pext
pext: CXXFLAGS += $(RFLAGS) $(PEXTFLAGS)
pext: LDFLAGS += -flto -static 
pext: EXE = $(BINARY_DIR)/Halogen-pext.exe
pext: binary

.PHONY: popcnt-avx2
popcnt-avx2: CXXFLAGS += $(RFLAGS) $(AVX2FLAGS) $(POPCNTFLAGS)
popcnt-avx2: LDFLAGS += -flto -static 
popcnt-avx2: EXE = $(BINARY_DIR)/Halogen-popcnt-avx2.exe
popcnt-avx2: binary

.PHONY: pext-avx2
pext-avx2: CXXFLAGS += $(RFLAGS) $(AVX2FLAGS) $(PEXTFLAGS)
pext-avx2: LDFLAGS += -flto -static 
pext-avx2: EXE = $(BINARY_DIR)/Halogen-pext-avx2.exe
pext-avx2: binary

.PHONY: release
release:
	$(MAKE) native
	$(MAKE) nopopcnt
	$(MAKE) popcnt
	$(MAKE) pext
	$(MAKE) popcnt-avx2
	$(MAKE) pext-avx2

#----------------------------------------------------------------------------------------------------------------------

# Copied from the Stockfish makefile
.PHONY: net
net:
	@echo "net: $(EVALFILE)"
	$(eval curl_or_wget := $(shell if hash curl 2>/dev/null; then echo "curl -skL"; elif hash wget 2>/dev/null; then echo "wget -qO-"; fi))
	@if test -f "$(EVALFILE)"; then \
        echo "Already available."; \
	else \
		if [ "x$(curl_or_wget)" = "x" ]; then \
			echo "Automatic download failed: neither curl nor wget is installed. Install one of these tools or download the net manually"; exit 1; \
		else \
			echo "Downloading... $(EVALURL)"; $(curl_or_wget) $(EVALURL) > $(EVALFILE);\
			echo "Download complete."; \
		fi; \
	fi;

#----------------------------------------------------------------------------------------------------------------------

# The final build step.
.PHONY: binary
binary: $(OBJS)
	@ mkdir -p $(BINARY_DIR)
	$(CXX) $(OBJS) -o $(EXE) $(LDFLAGS)

# Build step for C++ source
$(BUILD_DIR)/%.o: % net
	@ mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

.PHONY: clean_build
clean_build:
	rm -rf $(BUILD_DIR)

.PHONY: clean
clean: clean_build
	rm -rf $(BINARY_DIR)

# Include the .d makefiles. The - at the front suppresses the errors of missing
# Makefiles. Initially, all the .d files will be missing, and we don't want those
# errors to show up.
-include $(DEPS)