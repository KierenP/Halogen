LLVM_PROFDATA := llvm-profdata

BUILD_DIR := ../build
BINARY_DIR := ../bin

ifndef EVALFILE
    NET_HASH := 2a2c41df
    EVALFILE := $(BUILD_DIR)/$(NET_HASH).nn
    NO_EVALFILE_SET = true
endif

SRCS := \
    main.cpp \
    chessboard/board_state.cpp \
    chessboard/game_state.cpp \
    evaluation/evaluate.cpp \
    movegen/move.cpp \
    movegen/movegen.cpp \
    network/network.cpp \
    numa/numa.cpp \
    search/cuckoo.cpp \
    search/data.cpp \
    search/history.cpp \
    search/search.cpp \
    search/staged_movegen.cpp \
    search/static_exchange_evaluation.cpp \
    search/syzygy.cpp \
    search/zobrist.cpp \
    search/limit/time.cpp \
    search/transposition/table.cpp \
    search/transposition/entry.cpp \
    search/thread.cpp \
    test/static_exchange_evaluation_test.cpp \
    third-party/Pyrrhic/tbprobe.cpp \
    uci/uci.cpp \
    datagen/datagen.cpp \
    datagen/encode.cpp

# String substitution for every C/C++ file.
# As an example, hello.cpp turns into ./build/hello.cpp.o
OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)

#----------------------------------------------------------------------------------------------------------------------
# Define compiler flags and instruction sets

WFLAGS = -Wall -Wextra -Wshadow
AFLAGS = $(WFLAGS) -I. -g -std=c++20 -fno-exceptions -DEVALFILE=\"$(BUILD_DIR)/verbatim.nn\"
CFLAGS = -O3 $(AFLAGS) -DNDEBUG -flto -march=native
RFLAGS = -O3 $(AFLAGS) -DNDEBUG -flto
# only enable Werror on debug builds, so we don't cause issues for people compiling Halogen on a different compiler
SFLAGS = -O1 $(AFLAGS) -Werror -fno-omit-frame-pointer
DFLAGS = -O0 $(AFLAGS) -Werror

LDFLAGS += -Wl,--whole-archive -lpthread -Wl,--no-whole-archive -lm

LDFLAGS += $(EXTRA_LDFLAGS)
CXXFLAGS += $(EXTRA_CXXFLAGS)

# Different instruction sets targeting different architectures.

# AMD processors before Zen 3 implement PEXT in microcode, so we support bmi as a optional orthogonal configuration for AVX2
# For AVX512 and above, we use PEXT as a default instruction set
PEXTFLAGS = -DUSE_PEXT -mbmi -mbmi2

# Nehalem (2008)
SSE4FLAGS = -DUSE_SSE4 -msse -msse2 -msse3 -mssse3 -msse4 -msse4.1 -msse4.2
# Sandy Bridge (2011)
AVXFLAGS = -DUSE_AVX $(SSE4FLAGS) -mavx -mfma
# Haswell (2013)
AVX2FLAGS = -DUSE_AVX2 $(AVXFLAGS) -mavx2 -DUSE_POPCNT -mpopcnt
# Skylake (2017)
AVX512FLAGS = -DUSE_AVX512 $(PEXTFLAGS) $(AVX2FLAGS) -mavx512f -mavx512cd -mavx512vl -mavx512dq -mavx512bw
# Skylake (2019)
AVX512VNNIFLAGS = -DUSE_AVX512_VNNI $(PEXTFLAGS) $(AVX512FLAGS) -mavx512ifma -mavx512vbmi -mavx512vbmi2 -mavx512bitalg -mavx512vnni -mavx512vpopcntdq

#----------------------------------------------------------------------------------------------------------------------
# OS and compiler specific flags

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

ifeq ($(findstring clang++, $(CXX)),)
    PGOGEN = -fprofile-generate
    PGOUSE = -fprofile-use -fprofile-partial-training
    CXXFLAGS += -Wno-interference-size
    $(info detected compiler is [g++])
else
    PGOGEN = -fprofile-instr-generate=$(BUILD_DIR)/default.profraw
    PGOMERGE = $(LLVM_PROFDATA) merge -output=$(BUILD_DIR)/merged.profdata $(BUILD_DIR)/default.profraw --sparse=true
    PGOUSE = -fprofile-instr-use=$(BUILD_DIR)/merged.profdata
    $(info detected compiler is [clang++])
endif

#----------------------------------------------------------------------------------------------------------------------
# Optimized native builds, including PGO optimization. Detects supported instruction sets on the target

SUPPORTED = $(shell echo | $(CC) -march=native -E -dM -)

ifneq ($(findstring __POPCNT__, $(SUPPORTED)),)
    CFLAGS += -DUSE_POPCNT
endif

ifneq ($(findstring __BMI2__, $(SUPPORTED)),)
    ifeq ($(findstring __znver1, $(SUPPORTED)),)
        ifeq ($(findstring __znver2, $(SUPPORTED)),)
            CFLAGS += -DUSE_PEXT
        endif
    endif
endif

ifneq ($(findstring __AVX512VNNI__, $(SUPPORTED)),)
    ifeq ($(findstring __znver4, $(SUPPORTED)),)
        CFLAGS += -DUSE_AVX512_VNNI
    endif
endif

ifneq ($(findstring __AVX512F__, $(SUPPORTED)),)
    CFLAGS += -DUSE_AVX512
endif
ifneq ($(findstring __AVX2__, $(SUPPORTED)),)
    CFLAGS += -DUSE_AVX2
endif
ifneq ($(findstring __AVX__, $(SUPPORTED)),)
    CFLAGS += -DUSE_AVX
endif
ifneq ($(findstring __SSE4_2__, $(SUPPORTED)),)
    CFLAGS += -DUSE_SSE4
endif

.PHONY: pgo pgo-instrumented pgo-compile native

pgo: VERBATIM_FLAGS=$(CFLAGS)
pgo: verbatim_binary
	$(MAKE) pgo-instrumented
	$(MAKE) pgo-compile

pgo-instrumented: VERBATIM_FLAGS=$(CFLAGS)
pgo-instrumented: CXXFLAGS += $(CFLAGS) $(PGOGEN) -fno-lto
pgo-instrumented: LDFLAGS += $(PGOGEN) -fno-lto
pgo-instrumented: EXE = $(BUILD_DIR)/Halogen-pgo-instrumented
pgo-instrumented: binary
	./$(EXE) bench
	$(PGOMERGE)

pgo-compile: VERBATIM_FLAGS=$(CFLAGS)
pgo-compile: CXXFLAGS += $(CFLAGS) $(PGOUSE)
pgo-compile: LDFLAGS += -flto $(PGOUSE)
pgo-compile: EXE = $(BINARY_DIR)/Halogen-pgo
pgo-compile: binary

native: VERBATIM_FLAGS=$(CFLAGS)
native: CXXFLAGS += $(CFLAGS)
native: LDFLAGS += -flto
native: EXE = $(BINARY_DIR)/Halogen-native
native: verbatim_binary binary

#----------------------------------------------------------------------------------------------------------------------
# Debug and sanitizer builds

.PHONY: debug sanitize-address sanitize-undefined sanitize-thread tune shuffle

debug: VERBATIM_FLAGS=$(AFLAGS)
debug: CXXFLAGS += $(DFLAGS)
debug: EXE = $(BINARY_DIR)/Halogen-debug
debug: verbatim_binary binary

sanitize-address: VERBATIM_FLAGS=$(AFLAGS)
sanitize-address: CXXFLAGS += $(SFLAGS) -fsanitize=address
sanitize-address: LDFLAGS += -fsanitize=address
sanitize-address: EXE = $(BINARY_DIR)/Halogen-sanitize-address
sanitize-address: verbatim_binary binary

sanitize-undefined: VERBATIM_FLAGS=$(AFLAGS)
sanitize-undefined: CXXFLAGS += $(SFLAGS) -fsanitize=undefined
sanitize-undefined: LDFLAGS += -fsanitize=undefined
sanitize-undefined: EXE = $(BINARY_DIR)/Halogen-sanitize-undefined
sanitize-undefined: verbatim_binary binary

sanitize-thread: VERBATIM_FLAGS=$(AFLAGS)
sanitize-thread: CXXFLAGS += $(SFLAGS) -fsanitize=thread
sanitize-thread: LDFLAGS += -fsanitize=thread
sanitize-thread: EXE = $(BINARY_DIR)/Halogen-sanitize-thread
sanitize-thread: verbatim_binary binary

tune: VERBATIM_FLAGS=$(CFLAGS)
tune: CXXFLAGS += $(CFLAGS) -DTUNE
tune: LDFLAGS += -flto
tune: EXE = $(BINARY_DIR)/Halogen-tune
tune: verbatim_binary binary

shuffle: VERBATIM_FLAGS=$(CFLAGS)
shuffle: CXXFLAGS += $(CFLAGS) -DNETWORK_SHUFFLE -fopenmp
shuffle: LDFLAGS += -flto -fopenmp
shuffle: EXE = $(BINARY_DIR)/Halogen-shuffle
shuffle: verbatim_binary binary

#----------------------------------------------------------------------------------------------------------------------
# Release builds that are statically linked and target specific instruction sets

.PHONY: legacy sse4 avx avx2 avx2-pext avx512 avx512vnni release

legacy: VERBATIM_FLAGS=$(AFLAGS)
legacy: CXXFLAGS += $(RFLAGS)
legacy: LDFLAGS += -flto -static 
legacy: EXE = $(BINARY_DIR)/Halogen-legacy
legacy: verbatim_binary binary

sse4: VERBATIM_FLAGS=$(AFLAGS) -DUSE_SSE4
sse4: CXXFLAGS += $(RFLAGS) $(SSE4FLAGS)
sse4: LDFLAGS += -flto -static 
sse4: EXE = $(BINARY_DIR)/Halogen-sse4
sse4: verbatim_binary binary

avx: VERBATIM_FLAGS=$(AFLAGS) -DUSE_SSE4 -DUSE_AVX
avx: CXXFLAGS += $(RFLAGS) $(AVXFLAGS)
avx: LDFLAGS += -flto -static 
avx: EXE = $(BINARY_DIR)/Halogen-avx
avx: verbatim_binary binary

avx2: VERBATIM_FLAGS=$(AFLAGS) -DUSE_SSE4 -DUSE_AVX -DUSE_AVX2
avx2: CXXFLAGS += $(RFLAGS) $(AVX2FLAGS)
avx2: LDFLAGS += -flto -static 
avx2: EXE = $(BINARY_DIR)/Halogen-avx2
avx2: verbatim_binary binary

avx2-pext: VERBATIM_FLAGS=$(AFLAGS) -DUSE_SSE4 -DUSE_AVX -DUSE_AVX2 -DUSE_PEXT
avx2-pext: CXXFLAGS += $(RFLAGS) $(AVX2FLAGS) $(PEXTFLAGS)
avx2-pext: LDFLAGS += -flto -static 
avx2-pext: EXE = $(BINARY_DIR)/Halogen-avx2-pext
avx2-pext: verbatim_binary binary

avx512: VERBATIM_FLAGS=$(AFLAGS) -DUSE_SSE4 -DUSE_AVX -DUSE_AVX2 -DUSE_PEXT -DUSE_AVX512
avx512: CXXFLAGS += $(RFLAGS) $(AVX512FLAGS) $(PEXTFLAGS)
avx512: LDFLAGS += -flto -static 
avx512: EXE = $(BINARY_DIR)/Halogen-avx512
avx512: verbatim_binary binary

avx512vnni: VERBATIM_FLAGS=$(AFLAGS) -DUSE_SSE4 -DUSE_AVX -DUSE_AVX2 -DUSE_PEXT -DUSE_AVX512 -DUSE_AVX512_VNNI
avx512vnni: CXXFLAGS += $(RFLAGS) $(AVX512VNNIFLAGS) $(PEXTFLAGS)
avx512vnni: LDFLAGS += -flto -static 
avx512vnni: EXE = $(BINARY_DIR)/Halogen-avx512vnni
avx512vnni: verbatim_binary binary

release:
	$(MAKE) legacy
	$(MAKE) sse4
	$(MAKE) avx
	$(MAKE) avx2
	$(MAKE) avx2-pext
	$(MAKE) avx512
	$(MAKE) avx512vnni

#----------------------------------------------------------------------------------------------------------------------

$(EVALFILE):
ifdef NO_EVALFILE_SET
	@ mkdir -p $(dir $@)
	$(info Downloading default network $(NET_HASH).nn)
	curl -sL https://github.com/KierenP/Halogen-Networks/releases/download/$(NET_HASH)/$(NET_HASH).nn > $(EVALFILE)
endif

verbatim_binary: tools/verbatim.cpp $(EVALFILE) FORCE
	@ mkdir -p $(BUILD_DIR)
	$(CXX) $(VERBATIM_FLAGS) tools/verbatim.cpp -o $(BUILD_DIR)/verbatim $(LDFLAGS)

$(BUILD_DIR)/verbatim.nn: verbatim_binary FORCE
	./$(BUILD_DIR)/verbatim $(EVALFILE) $(BUILD_DIR)/verbatim.nn

#----------------------------------------------------------------------------------------------------------------------

.PHONY: binary
binary: $(OBJS)
	@ mkdir -p $(BINARY_DIR)
	$(CXX) $(OBJS) -o $(EXE) $(LDFLAGS)

$(BUILD_DIR)/%.o: % $(BUILD_DIR)/verbatim.nn FORCE
	@ mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

.PHONY: clean_build
clean_build:
	rm -rf $(BUILD_DIR)

.PHONY: clean
clean: clean_build
	rm -rf $(BINARY_DIR)

FORCE: