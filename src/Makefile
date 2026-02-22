#----------------------------------------------------------------------------------------------------------------------
# Project Configuration
#----------------------------------------------------------------------------------------------------------------------

PROJECT := Halogen
BUILD_DIR := ../build
BINARY_DIR := ../bin

# Default target (to support openbench)
.DEFAULT_GOAL := pgo

# Network configuration
ifndef EVALFILE
    NET_HASH := 7b44b83b
    EVALFILE := $(BUILD_DIR)/$(NET_HASH).nn
    NO_EVALFILE_SET = true
endif

# Architecture configuration (can be overridden: make ARCH=avx2 debug)
ARCH ?= native

#----------------------------------------------------------------------------------------------------------------------
# Source Files
#----------------------------------------------------------------------------------------------------------------------

SRCS := \
    main.cpp \
    chessboard/board_state.cpp \
    chessboard/game_state.cpp \
    evaluation/evaluate.cpp \
    movegen/move.cpp \
    movegen/movegen.cpp \
    network/network.cpp \
    network/accumulator/king_bucket.cpp \
    network/accumulator/threat.cpp \
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
    datagen/encode.cpp \
    utility/arch.cpp

OBJS := $(SRCS:%=$(BUILD_DIR)/$(ARCH)/%.o)

#----------------------------------------------------------------------------------------------------------------------
# Platform and Compiler Detection
#----------------------------------------------------------------------------------------------------------------------

ifeq ($(OS),Windows_NT)
    DETECTED_OS := Windows
else
    DETECTED_OS := $(shell uname)
endif

$(info Detected OS: $(DETECTED_OS))

CXX_VERSION := $(shell $(CXX) --version 2>/dev/null)
ifneq ($(findstring clang,$(CXX_VERSION)),)
    COMPILER := clang
    $(info Detected compiler: clang++)
else
    COMPILER := gcc
    $(info Detected compiler: g++)
endif

#----------------------------------------------------------------------------------------------------------------------
# Base Compiler Flags
#----------------------------------------------------------------------------------------------------------------------

WARN_FLAGS := -Wall -Wextra -Wshadow -Wno-missing-field-initializers -Wno-deprecated-declarations -Wno-ignored-attributes
BASE_FLAGS := $(WARN_FLAGS) -pthread -g -I. -std=c++20 -fno-exceptions -DEVALFILE=\"$(BUILD_DIR)/verbatim.nn\" -ffp-contract=off
BASE_FLAGS += $(EXTRA_CXXFLAGS)

# Optimization levels
OPT_NONE := -O0 -Werror
OPT_SANITIZE := -O1 -fno-omit-frame-pointer
OPT_RELEASE := -O3 -DNDEBUG

#----------------------------------------------------------------------------------------------------------------------
# Linker Flags
#----------------------------------------------------------------------------------------------------------------------

BASE_LDFLAGS := -pthread -lm
BASE_LDFLAGS += $(EXTRA_LDFLAGS)

ifeq ($(DETECTED_OS),Windows)
    STATIC_LDFLAGS := -static
endif

#----------------------------------------------------------------------------------------------------------------------
# Architecture Definitions
#----------------------------------------------------------------------------------------------------------------------

# Format: ARCH_DEFINES (preprocessor flags) and ARCH_MARCH (compiler codegen flags)

# Native - auto-detect
ifeq ($(ARCH),native)
    SUPPORTED := $(shell echo | $(CC) -march=native -E -dM -)
    ARCH_MARCH := -march=native
    ARCH_DEFINES :=
    
    ifneq ($(findstring __POPCNT__, $(SUPPORTED)),)
        ARCH_DEFINES += -DUSE_POPCNT
    endif
    
    ifneq ($(findstring __BMI2__, $(SUPPORTED)),)
        ifeq ($(findstring __znver1, $(SUPPORTED)),)
            ifeq ($(findstring __znver2, $(SUPPORTED)),)
                ARCH_DEFINES += -DUSE_PEXT
            endif
        endif
    endif
    
    ifneq ($(findstring __AVX512VNNI__, $(SUPPORTED)),)
        ifeq ($(findstring __znver4, $(SUPPORTED)),)
            ARCH_DEFINES += -DUSE_AVX512_VNNI
        endif
    endif
    
    ifneq ($(findstring __AVX512F__, $(SUPPORTED)),)
        ARCH_DEFINES += -DUSE_AVX512
    endif
    ifneq ($(findstring __AVX2__, $(SUPPORTED)),)
        ARCH_DEFINES += -DUSE_AVX2
    endif
    ifneq ($(findstring __AVX__, $(SUPPORTED)),)
        ARCH_DEFINES += -DUSE_AVX
    endif
    ifneq ($(findstring __SSE4_2__, $(SUPPORTED)),)
        ARCH_DEFINES += -DUSE_SSE4
    endif

    ifneq ($(findstring __ARM_NEON, $(SUPPORTED)),)
        # Assume if we support NEON, we also support DOTPROD. There's a small range of ARMv8.0-8.1 CPUs without 
        # DOTPROD, but they are rare. Use ARCH=neon if you need to disable DOTPROD.
        ARCH_DEFINES += -DUSE_NEON
        ARCH_DEFINES += -DUSE_NEON_DOTPROD
    endif
endif

# Legacy - no special instructions
ifeq ($(ARCH),legacy)
    ARCH_DEFINES :=
    ARCH_MARCH :=
endif

# Nehalem (2008)
ifeq ($(ARCH),sse4)
    ARCH_DEFINES := -DUSE_SSE4
    ARCH_MARCH := -msse -msse2 -msse3 -mssse3 -msse4 -msse4.1 -msse4.2
endif

# Sandy Bridge (2011)
ifeq ($(ARCH),avx)
    ARCH_DEFINES := -DUSE_SSE4 -DUSE_AVX
    ARCH_MARCH := -msse -msse2 -msse3 -mssse3 -msse4 -msse4.1 -msse4.2 -mavx -mfma
endif

# Haswell (2013)
ifeq ($(ARCH),avx2)
    ARCH_DEFINES := -DUSE_SSE4 -DUSE_AVX -DUSE_AVX2 -DUSE_POPCNT
    ARCH_MARCH := -msse -msse2 -msse3 -mssse3 -msse4 -msse4.1 -msse4.2 -mavx -mfma -mavx2 -mpopcnt
endif

# Haswell with BMI2
ifeq ($(ARCH),avx2-pext)
    ARCH_DEFINES := -DUSE_SSE4 -DUSE_AVX -DUSE_AVX2 -DUSE_POPCNT -DUSE_PEXT
    ARCH_MARCH := -msse -msse2 -msse3 -mssse3 -msse4 -msse4.1 -msse4.2 -mavx -mfma -mavx2 -mpopcnt -mbmi -mbmi2
endif

# Skylake (2017)
ifeq ($(ARCH),avx512)
    ARCH_DEFINES := -DUSE_SSE4 -DUSE_AVX -DUSE_AVX2 -DUSE_POPCNT -DUSE_PEXT -DUSE_AVX512
    ARCH_MARCH := -msse -msse2 -msse3 -mssse3 -msse4 -msse4.1 -msse4.2 -mavx -mfma -mavx2 -mpopcnt -mbmi -mbmi2 -mavx512f -mavx512cd -mavx512vl -mavx512dq -mavx512bw
endif

# Cascade Lake (2019)
ifeq ($(ARCH),avx512vnni)
    ARCH_DEFINES := -DUSE_SSE4 -DUSE_AVX -DUSE_AVX2 -DUSE_POPCNT -DUSE_PEXT -DUSE_AVX512 -DUSE_AVX512_VNNI
    ARCH_MARCH := -msse -msse2 -msse3 -mssse3 -msse4 -msse4.1 -msse4.2 -mavx -mfma -mavx2 -mpopcnt -mbmi -mbmi2 -mavx512f -mavx512cd -mavx512vl -mavx512dq -mavx512bw -mavx512ifma -mavx512vbmi -mavx512vbmi2 -mavx512bitalg -mavx512vnni -mavx512vpopcntdq
endif

# armv8
ifeq ($(ARCH),neon)
    ARCH_DEFINES := -DUSE_NEON
    ARCH_MARCH := -march=armv8-a+simd
endif

# armv8.2
ifeq ($(ARCH),neon-dotprod)
    ARCH_DEFINES := -DUSE_NEON -DUSE_NEON_DOTPROD
    ARCH_MARCH := -march=armv8.2-a+dotprod
endif

$(info Building for architecture: $(ARCH))

#----------------------------------------------------------------------------------------------------------------------
# PGO Configuration
#----------------------------------------------------------------------------------------------------------------------

LLVM_PROFDATA := llvm-profdata

ifneq ($(shell command -v llvm-profdata >/dev/null 2>&1 && echo found),found)
    ifeq ($(DETECTED_OS),Darwin)
        LLVM_PROFDATA := xcrun llvm-profdata
    endif
endif

ifeq ($(COMPILER),gcc)
    LTO_FLAGS := -flto=auto -flto-partition=one
    PGO_GEN := -fprofile-generate
    PGO_USE := -fprofile-use -fprofile-partial-training
    CXXFLAGS += -Wno-interference-size
else
    LTO_FLAGS := -flto
    PGO_GEN := -fprofile-instr-generate=$(BUILD_DIR)/$(ARCH)/default.profraw
    PGO_MERGE := $(LLVM_PROFDATA) merge -output=$(BUILD_DIR)/$(ARCH)/merged.profdata $(BUILD_DIR)/$(ARCH)/default.profraw --sparse=true
    # Some auxiliary translation units (tests, datagen tools) won't have profile hits; silence the noisy warning from
    # clang when applying PGO data.
    PGO_USE := -fprofile-instr-use=$(BUILD_DIR)/$(ARCH)/merged.profdata -Wno-profile-instr-unprofiled
endif

#----------------------------------------------------------------------------------------------------------------------
# Build Type Definitions
#----------------------------------------------------------------------------------------------------------------------

# Debug build
ifeq ($(MAKECMDGOALS),debug)
    BUILD_TYPE := debug
    CXXFLAGS += $(OPT_NONE) $(BASE_FLAGS) $(ARCH_DEFINES) $(ARCH_MARCH)
    LDFLAGS += $(BASE_LDFLAGS)
    VERBATIM_FLAGS := $(BASE_FLAGS) $(ARCH_DEFINES)
    EXE := $(BINARY_DIR)/$(PROJECT)-$(ARCH)-debug
endif

# Release build
ifeq ($(MAKECMDGOALS),release)
    BUILD_TYPE := release
    CXXFLAGS += $(OPT_RELEASE) $(BASE_FLAGS) $(LTO_FLAGS) $(ARCH_DEFINES) $(ARCH_MARCH)
    LDFLAGS += $(BASE_LDFLAGS) $(LTO_FLAGS) $(STATIC_LDFLAGS)
    VERBATIM_FLAGS := $(OPT_RELEASE) $(BASE_FLAGS) $(ARCH_DEFINES)
    EXE := $(BINARY_DIR)/$(PROJECT)-$(ARCH)
endif

# Sanitizer builds
ifeq ($(MAKECMDGOALS),sanitize-address)
    BUILD_TYPE := sanitize-address
    CXXFLAGS += $(OPT_SANITIZE) $(BASE_FLAGS) $(ARCH_DEFINES) $(ARCH_MARCH) -fsanitize=address
    LDFLAGS += $(BASE_LDFLAGS) -fsanitize=address
    VERBATIM_FLAGS := $(BASE_FLAGS) $(ARCH_DEFINES)
    EXE := $(BINARY_DIR)/$(PROJECT)-$(ARCH)-asan
endif

ifeq ($(MAKECMDGOALS),sanitize-undefined)
    BUILD_TYPE := sanitize-undefined
    CXXFLAGS += $(OPT_SANITIZE) $(BASE_FLAGS) $(ARCH_DEFINES) $(ARCH_MARCH) -fsanitize=undefined
    LDFLAGS += $(BASE_LDFLAGS) -fsanitize=undefined
    VERBATIM_FLAGS := $(BASE_FLAGS) $(ARCH_DEFINES)
    EXE := $(BINARY_DIR)/$(PROJECT)-$(ARCH)-ubsan
endif

ifeq ($(MAKECMDGOALS),sanitize-thread)
    BUILD_TYPE := sanitize-thread
    CXXFLAGS += $(OPT_SANITIZE) $(BASE_FLAGS) $(ARCH_DEFINES) $(ARCH_MARCH) -fsanitize=thread
    LDFLAGS += $(BASE_LDFLAGS) -fsanitize=thread
    VERBATIM_FLAGS := $(BASE_FLAGS) $(ARCH_DEFINES)
    EXE := $(BINARY_DIR)/$(PROJECT)-$(ARCH)-tsan
endif

# Tune build
ifeq ($(MAKECMDGOALS),tune)
    BUILD_TYPE := tune
    CXXFLAGS += $(OPT_RELEASE) $(BASE_FLAGS) $(LTO_FLAGS) $(ARCH_DEFINES) $(ARCH_MARCH) -DTUNE
    LDFLAGS += $(BASE_LDFLAGS) $(LTO_FLAGS)
    VERBATIM_FLAGS := $(OPT_RELEASE) $(BASE_FLAGS) $(ARCH_DEFINES)
    EXE := $(BINARY_DIR)/$(PROJECT)-tune
endif

# Shuffle build
ifeq ($(MAKECMDGOALS),shuffle)
    BUILD_TYPE := shuffle
    CXXFLAGS += $(OPT_RELEASE) $(BASE_FLAGS) $(LTO_FLAGS) $(ARCH_DEFINES) $(ARCH_MARCH) -fopenmp -DNETWORK_SHUFFLE
    LDFLAGS += $(BASE_LDFLAGS) $(LTO_FLAGS) -fopenmp
    VERBATIM_FLAGS := $(OPT_RELEASE) $(BASE_FLAGS) $(ARCH_DEFINES) -DNETWORK_SHUFFLE
    EXE := $(BINARY_DIR)/$(PROJECT)-shuffle
endif

# Special target that only builds the object files, but doesn't link the final binary
ifeq ($(MAKECMDGOALS),analyze)
    BUILD_TYPE := analyze
    CXXFLAGS += $(OPT_NONE) $(BASE_FLAGS) $(ARCH_DEFINES) $(ARCH_MARCH)
    LDFLAGS += $(BASE_LDFLAGS)
    VERBATIM_FLAGS := $(BASE_FLAGS) $(ARCH_DEFINES)
endif

# Tournament build
ifeq ($(MAKECMDGOALS),tournament)
    BUILD_TYPE := tournament
    CXXFLAGS += $(OPT_RELEASE) $(BASE_FLAGS) $(LTO_FLAGS) $(ARCH_DEFINES) $(ARCH_MARCH) -DTOURNAMENT_MODE
    LDFLAGS += $(BASE_LDFLAGS) $(LTO_FLAGS) -lnuma
    VERBATIM_FLAGS := $(OPT_RELEASE) $(BASE_FLAGS) $(ARCH_DEFINES)
    EXE := $(BINARY_DIR)/$(PROJECT)-tournament
endif

# PGO instrumented build
ifeq ($(MAKECMDGOALS),pgo-instrumented)
    BUILD_TYPE := pgo-instrumented
    CXXFLAGS += $(OPT_RELEASE) $(BASE_FLAGS) $(ARCH_DEFINES) $(ARCH_MARCH) $(PGO_GEN) -fno-lto
    LDFLAGS += $(BASE_LDFLAGS) $(PGO_GEN) -fno-lto
    VERBATIM_FLAGS := $(OPT_RELEASE) $(BASE_FLAGS) $(ARCH_DEFINES)
    EXE := $(BUILD_DIR)/$(ARCH)/$(PROJECT)-pgo-instrumented
endif

# PGO optimized build
ifeq ($(MAKECMDGOALS),pgo-compile)
    BUILD_TYPE := pgo-compile
    CXXFLAGS += $(OPT_RELEASE) $(BASE_FLAGS) $(LTO_FLAGS) $(ARCH_DEFINES) $(ARCH_MARCH) $(PGO_USE)
    LDFLAGS += $(BASE_LDFLAGS) $(LTO_FLAGS) $(PGO_USE) $(STATIC_LDFLAGS)
    VERBATIM_FLAGS := $(OPT_RELEASE) $(BASE_FLAGS) $(ARCH_DEFINES)
    EXE := $(BINARY_DIR)/$(PROJECT)-pgo
endif

#----------------------------------------------------------------------------------------------------------------------
# Main Build Targets
#----------------------------------------------------------------------------------------------------------------------

.PHONY: debug release native sanitize-address sanitize-undefined sanitize-thread tune shuffle tournament analyze
debug release sanitize-address sanitize-undefined sanitize-thread tune shuffle tournament: verbatim_binary binary
analyze: verbatim_binary $(OBJS)

.PHONY: pgo pgo-instrumented pgo-compile
pgo:
	$(MAKE) pgo-instrumented EXE=$(BUILD_DIR)/$(ARCH)/$(PROJECT)-pgo-instrumented
	./$(BUILD_DIR)/$(ARCH)/$(PROJECT)-pgo-instrumented bench
	$(PGO_MERGE)
	$(MAKE) pgo-compile

pgo-instrumented pgo-compile: verbatim_binary binary

#----------------------------------------------------------------------------------------------------------------------
# Network File Management
#----------------------------------------------------------------------------------------------------------------------

$(EVALFILE):
ifdef NO_EVALFILE_SET
	@ mkdir -p $(dir $@)
	$(info Downloading default network $(NET_HASH).nn)
	curl -sL https://github.com/KierenP/Halogen-Networks/releases/download/$(NET_HASH)/$(NET_HASH).nn > $(EVALFILE)
endif

#----------------------------------------------------------------------------------------------------------------------
# Verbatim Binary (Network Embedding)
#----------------------------------------------------------------------------------------------------------------------

.PHONY: verbatim_binary
verbatim_binary: tools/verbatim.cpp $(EVALFILE) FORCE
	@ mkdir -p $(BUILD_DIR)
	$(CXX) $(VERBATIM_FLAGS) tools/verbatim.cpp -o $(BUILD_DIR)/verbatim $(BASE_LDFLAGS)

$(BUILD_DIR)/verbatim.nn: verbatim_binary FORCE
	./$(BUILD_DIR)/verbatim $(EVALFILE) $(BUILD_DIR)/verbatim.nn

#----------------------------------------------------------------------------------------------------------------------
# Build Rules
#----------------------------------------------------------------------------------------------------------------------

.PHONY: binary
binary: $(OBJS)
	@ mkdir -p $(BINARY_DIR)
	$(CXX) $(OBJS) -o $(EXE) $(LDFLAGS)

$(BUILD_DIR)/$(ARCH)/%.o: % $(BUILD_DIR)/verbatim.nn FORCE
	@ mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

#----------------------------------------------------------------------------------------------------------------------
# Utility Targets
#----------------------------------------------------------------------------------------------------------------------

FORCE:
