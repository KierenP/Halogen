TARGET_EXEC := Halogen.exe
BUILD_DIR := ../build
EXE := $(BUILD_DIR)/$(TARGET_EXEC)
SRC_DIRS := .

WEIGHTS_URL = https://github.com/KierenP/Halogen-Networks/blob/6f6b5ec1436bf19212fa1667c1962871d51a7fb0/d7d569e5.nn?raw=true
WEIGHTS_FILE = d7d569e5.nn

#----------------------------------------------------------------------------------------------------------------------

# Find all the C++ files we want to compile
# I'd use a bash command but not all client have bash
SRCS := ./BitBoard.cpp \
	BitBoardDefine.cpp \
	BoardParameters.cpp \
	EvalCache.cpp \
	EvalNet.cpp \
	main.cpp \
	Move.cpp \
	MoveGeneration.cpp \
	MoveList.cpp \
	Network.cpp \
	Position.cpp \
	Search.cpp \
	SearchData.cpp \
	SearchLimits.cpp \
	StagedMoveGenerator.cpp \
	TimeManage.cpp \
	TranspositionTable.cpp \
	TTEntry.cpp \
	Zobrist.cpp \
	Pyrrhic/tbprobe.cpp

# String substitution for every C/C++ file.
# As an example, hello.cpp turns into ./build/hello.cpp.o
OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)

# String substitution (suffix version without %).
# As an example, ./build/hello.cpp.o turns into ./build/hello.cpp.d
DEPS := $(OBJS:.o=.d)

#----------------------------------------------------------------------------------------------------------------------

WFLAGS = -Wall -Wextra -Wshadow -std=c++17
CFLAGS = -O3 $(WFLAGS) -DNDEBUG -flto -march=native 
RFLAGS = -O3 $(WFLAGS) -DNDEBUG -flto

POPCNTFLAGS = -DUSE_POPCNT -msse3 -mpopcnt
PEXTFLAGS   = $(POPCNTFLAGS) -DUSE_PEXT -mbmi2
AVX2FLAGS   = -DUSE_AVX2 -msse -msse3 -mpopcnt -mavx2 -msse4.1 -mssse3 -msse2

LDFLAGS   = -Wl,--whole-archive -lpthread -Wl,--no-whole-archive -lm
CXXFLAGS := $(INC_FLAGS) -MMD -MP -DWEIGHTS_FILE=\"$(WEIGHTS_FILE)\"

.PHONY: native
native: CXXFLAGS += $(CFLAGS) $(POPCNTFLAGS)
native: LDFLAGS += -flto
native: net $(EXE)

.PHONY: debug
debug: CXXFLAGS += $(WFLAGS) -g
debug: net $(EXE)

.PHONY: sanitize
sanitize: CXXFLAGS += $(WFLAGS) -g -fsanitize=undefined -fsanitize=address
sanitize: LDFLAGS += -fsanitize=undefined -fsanitize=address
sanitize: net $(EXE)

.PHONY: nopopcnt
nopopcnt: CXXFLAGS += $(RFLAGS)
nopopcnt: LDFLAGS += -flto -static 
nopopcnt: net $(EXE)

.PHONY: popcnt
popcnt: CXXFLAGS += $(RFLAGS) $(POPCNTFLAGS)
popcnt: LDFLAGS += -flto -static 
popcnt: net $(EXE)

.PHONY: pext
pext: CXXFLAGS += $(RFLAGS) $(PEXTFLAGS)
pext: LDFLAGS += -flto -static 
pext: net $(EXE)

.PHONY: popcnt-avx2
popcnt-avx2: CXXFLAGS += $(RFLAGS) $(AVX2FLAGS) $(POPCNTFLAGS)
popcnt-avx2: LDFLAGS += -flto -static 
popcnt-avx2: net $(EXE)

.PHONY: pext-avx2
pext-avx2: CXXFLAGS += $(RFLAGS) $(AVX2FLAGS) $(PEXTFLAGS)
pext-avx2: LDFLAGS += -flto -static 
pext-avx2: net $(EXE)

#----------------------------------------------------------------------------------------------------------------------

# Copied from the Stockfish makefile
net:
	@echo "Default net: $(WEIGHTS_FILE)"
	$(eval curl_or_wget := $(shell if hash curl 2>/dev/null; then echo "curl -skL"; elif hash wget 2>/dev/null; then echo "wget -qO-"; fi))
	@if test -f "$(WEIGHTS_FILE)"; then \
            echo "Already available."; \
	else \
		if [ "x$(curl_or_wget)" = "x" ]; then \
			echo "Automatic download failed: neither curl nor wget is installed. Install one of these tools or download the net manually"; exit 1; \
		else \
			echo "Downloading $(WEIGHTS_URL)"; $(curl_or_wget) $(WEIGHTS_URL) > $(WEIGHTS_FILE);\
		fi; \
	fi;
	$(eval shasum_command := $(shell if hash shasum 2>/dev/null; then echo "shasum -a 256 "; elif hash sha256sum 2>/dev/null; then echo "sha256sum "; fi))
	@if [ "x$(shasum_command)" != "x" ]; then \
	    if [ "$(WEIGHTS_FILE)" != `$(shasum_command) $(WEIGHTS_FILE) | cut -c1-8`".nn" ]; then \
			echo "Failed download or $(WEIGHTS_FILE) corrupted, please delete!"; exit 1; \
		fi \
	else \
		echo "shasum / sha256sum not found, skipping net validation"; \
	fi

#----------------------------------------------------------------------------------------------------------------------

# The final build step.
$(EXE): $(OBJS)
	$(CXX) $(OBJS) -o $@ $(LDFLAGS)

# Build step for C++ source
$(BUILD_DIR)/%.cpp.o: %.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -r $(BUILD_DIR)

# Include the .d makefiles. The - at the front suppresses the errors of missing
# Makefiles. Initially, all the .d files will be missing, and we don't want those
# errors to show up.
-include $(DEPS)