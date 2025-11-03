#----------------------------------------------------------------------------------------------------------------------
# Project Configuration
#----------------------------------------------------------------------------------------------------------------------

PROJECT := Halogen
BUILD_DIR := ../build
BINARY_DIR := ../bin

# Network configuration
ifndef EVALFILE
    NET_HASH := 637096c1
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

ifeq ($(findstring clang++, $(CXX)),)
    COMPILER := gcc
    $(info Detected compiler: g++)
else
    COMPILER := clang
    $(info Detected compiler: clang++)
endif

#----------------------------------------------------------------------------------------------------------------------
# Base Compiler Flags
#----------------------------------------------------------------------------------------------------------------------

WARN_FLAGS := -Wall -Wextra -Wshadow -Wno-missing-field-initializers
BASE_FLAGS := $(WARN_FLAGS) -I. -std=c++20 -fno-exceptions -DEVALFILE=\"$(BUILD_DIR)/verbatim.nn\"

# Optimization levels
OPT_NONE := -O0
OPT_SANITIZE := -O1
OPT_RELEASE := -O3

#----------------------------------------------------------------------------------------------------------------------
# Linker Flags
#----------------------------------------------------------------------------------------------------------------------

BASE_LDFLAGS := -Wl,--whole-archive -lpthread -Wl,--no-whole-archive -lm
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

$(info Building for architecture: $(ARCH))

#----------------------------------------------------------------------------------------------------------------------
# PGO Configuration
#----------------------------------------------------------------------------------------------------------------------

LLVM_PROFDATA := llvm-profdata

ifeq ($(COMPILER),gcc)
    PGO_GEN := -fprofile-generate
    PGO_USE := -fprofile-use -fprofile-partial-training
    CXXFLAGS += -Wno-interference-size
else
    PGO_GEN := -fprofile-instr-generate=$(BUILD_DIR)/$(ARCH)/default.profraw
    PGO_MERGE := $(LLVM_PROFDATA) merge -output=$(BUILD_DIR)/$(ARCH)/merged.profdata $(BUILD_DIR)/$(ARCH)/default.profraw --sparse=true
    PGO_USE := -fprofile-instr-use=$(BUILD_DIR)/$(ARCH)/merged.profdata
endif

#----------------------------------------------------------------------------------------------------------------------
# Build Type Definitions
#----------------------------------------------------------------------------------------------------------------------

# Debug build
ifeq ($(MAKECMDGOALS),debug)
    BUILD_TYPE := debug
    CXXFLAGS += $(OPT_NONE) $(BASE_FLAGS) -g -Werror $(ARCH_DEFINES) $(EXTRA_CXXFLAGS)
    LDFLAGS += $(BASE_LDFLAGS)
    VERBATIM_FLAGS := $(BASE_FLAGS) -g $(ARCH_DEFINES)
    EXE := $(BINARY_DIR)/$(PROJECT)-$(ARCH)-debug
endif

# Release build
ifeq ($(MAKECMDGOALS),release)
    BUILD_TYPE := release
    CXXFLAGS += $(OPT_RELEASE) $(BASE_FLAGS) -DNDEBUG -flto $(ARCH_DEFINES) $(ARCH_MARCH) $(EXTRA_CXXFLAGS)
    LDFLAGS += $(BASE_LDFLAGS) -flto $(STATIC_LDFLAGS)
    VERBATIM_FLAGS := $(OPT_RELEASE) $(BASE_FLAGS) $(ARCH_DEFINES)
    EXE := $(BINARY_DIR)/$(PROJECT)-$(ARCH)
endif

# Native optimized build
ifeq ($(MAKECMDGOALS),native)
    BUILD_TYPE := native
    override ARCH := native
    CXXFLAGS += $(OPT_RELEASE) $(BASE_FLAGS) -DNDEBUG -flto $(ARCH_DEFINES) $(ARCH_MARCH) $(EXTRA_CXXFLAGS)
    LDFLAGS += $(BASE_LDFLAGS) -flto
    VERBATIM_FLAGS := $(OPT_RELEASE) $(BASE_FLAGS) $(ARCH_DEFINES)
    EXE := $(BINARY_DIR)/$(PROJECT)-native
endif

# Sanitizer builds
ifeq ($(MAKECMDGOALS),sanitize-address)
    BUILD_TYPE := sanitize-address
    CXXFLAGS += $(OPT_SANITIZE) $(BASE_FLAGS) -g -Werror -fno-omit-frame-pointer -fsanitize=address $(ARCH_DEFINES) $(EXTRA_CXXFLAGS)
    LDFLAGS += $(BASE_LDFLAGS) -fsanitize=address
    VERBATIM_FLAGS := $(BASE_FLAGS) -g $(ARCH_DEFINES)
    EXE := $(BINARY_DIR)/$(PROJECT)-$(ARCH)-asan
endif

ifeq ($(MAKECMDGOALS),sanitize-undefined)
    BUILD_TYPE := sanitize-undefined
    CXXFLAGS += $(OPT_SANITIZE) $(BASE_FLAGS) -g -Werror -fno-omit-frame-pointer -fsanitize=undefined $(ARCH_DEFINES) $(EXTRA_CXXFLAGS)
    LDFLAGS += $(BASE_LDFLAGS) -fsanitize=undefined
    VERBATIM_FLAGS := $(BASE_FLAGS) -g $(ARCH_DEFINES)
    EXE := $(BINARY_DIR)/$(PROJECT)-$(ARCH)-ubsan
endif

ifeq ($(MAKECMDGOALS),sanitize-thread)
    BUILD_TYPE := sanitize-thread
    CXXFLAGS += $(OPT_SANITIZE) $(BASE_FLAGS) -g -Werror -fno-omit-frame-pointer -fsanitize=thread $(ARCH_DEFINES) $(EXTRA_CXXFLAGS)
    LDFLAGS += $(BASE_LDFLAGS) -fsanitize=thread
    VERBATIM_FLAGS := $(BASE_FLAGS) -g $(ARCH_DEFINES)
    EXE := $(BINARY_DIR)/$(PROJECT)-$(ARCH)-tsan
endif

# Tune build
ifeq ($(MAKECMDGOALS),tune)
    BUILD_TYPE := tune
    override ARCH := native
    CXXFLAGS += $(OPT_RELEASE) $(BASE_FLAGS) -DNDEBUG -DTUNE -flto $(ARCH_DEFINES) $(ARCH_MARCH) $(EXTRA_CXXFLAGS)
    LDFLAGS += $(BASE_LDFLAGS) -flto
    VERBATIM_FLAGS := $(OPT_RELEASE) $(BASE_FLAGS) $(ARCH_DEFINES)
    EXE := $(BINARY_DIR)/$(PROJECT)-tune
endif

# Shuffle build
ifeq ($(MAKECMDGOALS),shuffle)
    BUILD_TYPE := shuffle
    override ARCH := native
    CXXFLAGS += $(OPT_RELEASE) $(BASE_FLAGS) -DNDEBUG -DNETWORK_SHUFFLE -flto $(ARCH_DEFINES) $(ARCH_MARCH) -fopenmp $(EXTRA_CXXFLAGS)
    LDFLAGS += $(BASE_LDFLAGS) -flto -fopenmp
    VERBATIM_FLAGS := $(OPT_RELEASE) $(BASE_FLAGS) $(ARCH_DEFINES) -DNETWORK_SHUFFLE
    EXE := $(BINARY_DIR)/$(PROJECT)-shuffle
endif

# PGO instrumented build
ifeq ($(MAKECMDGOALS),pgo-instrumented)
    BUILD_TYPE := pgo-instrumented
    override ARCH := native
    CXXFLAGS += $(OPT_RELEASE) $(BASE_FLAGS) -DNDEBUG $(ARCH_DEFINES) $(ARCH_MARCH) $(PGO_GEN) -fno-lto $(EXTRA_CXXFLAGS)
    LDFLAGS += $(BASE_LDFLAGS) $(PGO_GEN) -fno-lto
    VERBATIM_FLAGS := $(OPT_RELEASE) $(BASE_FLAGS) $(ARCH_DEFINES)
    EXE := $(BUILD_DIR)/$(ARCH)/$(PROJECT)-pgo-instrumented
endif

# PGO optimized build
ifeq ($(MAKECMDGOALS),pgo-compile)
    BUILD_TYPE := pgo-compile
    override ARCH := native
    CXXFLAGS += $(OPT_RELEASE) $(BASE_FLAGS) -DNDEBUG $(ARCH_DEFINES) $(ARCH_MARCH) $(PGO_USE) $(EXTRA_CXXFLAGS)
    LDFLAGS += $(BASE_LDFLAGS) -flto $(PGO_USE)
    VERBATIM_FLAGS := $(OPT_RELEASE) $(BASE_FLAGS) $(ARCH_DEFINES)
    EXE := $(BINARY_DIR)/$(PROJECT)-pgo
endif

#----------------------------------------------------------------------------------------------------------------------
# Main Build Targets
#----------------------------------------------------------------------------------------------------------------------

.PHONY: debug release native sanitize-address sanitize-undefined sanitize-thread tune shuffle
debug release native sanitize-address sanitize-undefined sanitize-thread tune shuffle: verbatim_binary binary

.PHONY: pgo pgo-instrumented pgo-compile
pgo: verbatim_binary
    $(MAKE) pgo-instrumented
    ./$(BUILD_DIR)/$(ARCH)/$(PROJECT)-pgo-instrumented bench
    $(PGO_MERGE)
    $(MAKE) pgo-compile

pgo-instrumented pgo-compile: verbatim_binary binary

#----------------------------------------------------------------------------------------------------------------------
# Multi-Architecture Release Target
#----------------------------------------------------------------------------------------------------------------------

.PHONY: release-all
release-all:
    $(MAKE) ARCH=legacy release
    $(MAKE) ARCH=sse4 release
    $(MAKE) ARCH=avx release
    $(MAKE) ARCH=avx2 release
    $(MAKE) ARCH=avx2-pext release
    $(MAKE) ARCH=avx512 release
    $(MAKE) ARCH=avx512vnni release

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

.PHONY: clean clean_build
clean_build:
    rm -rf $(BUILD_DIR)

clean: clean_build
    rm -rf $(BINARY_DIR)

FORCE: