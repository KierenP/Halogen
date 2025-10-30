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

OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)

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

# Common flag sets
DEBUG_FLAGS := $(OPT_NONE) $(BASE_FLAGS) -g -Werror
SANITIZE_FLAGS := $(OPT_SANITIZE) $(BASE_FLAGS) -g -Werror -fno-omit-frame-pointer
RELEASE_FLAGS := $(OPT_RELEASE) $(BASE_FLAGS) -DNDEBUG -flto
NATIVE_FLAGS := $(OPT_RELEASE) $(BASE_FLAGS) -DNDEBUG -flto -march=native

#----------------------------------------------------------------------------------------------------------------------
# Linker Flags
#----------------------------------------------------------------------------------------------------------------------

BASE_LDFLAGS := -Wl,--whole-archive -lpthread -Wl,--no-whole-archive -lm
BASE_LDFLAGS += $(EXTRA_LDFLAGS)

ifeq ($(DETECTED_OS),Windows)
    STATIC_LDFLAGS := -static
endif

#----------------------------------------------------------------------------------------------------------------------
# Instruction Set Flags
#----------------------------------------------------------------------------------------------------------------------

PEXT_FLAGS := -DUSE_PEXT -mbmi -mbmi2
POPCNT_FLAGS := -DUSE_POPCNT -mpopcnt

# Nehalem (2008)
SSE4_FLAGS := -DUSE_SSE4 -msse -msse2 -msse3 -mssse3 -msse4 -msse4.1 -msse4.2

# Sandy Bridge (2011)
AVX_FLAGS := -DUSE_AVX $(SSE4_FLAGS) -mavx -mfma

# Haswell (2013)
AVX2_FLAGS := -DUSE_AVX2 $(AVX_FLAGS) $(POPCNT_FLAGS) -mavx2

# Skylake (2017)
AVX512_FLAGS := -DUSE_AVX512 $(PEXT_FLAGS) $(AVX2_FLAGS) -mavx512f -mavx512cd -mavx512vl -mavx512dq -mavx512bw

# Skylake (2019)
AVX512VNNI_FLAGS := -DUSE_AVX512_VNNI $(AVX512_FLAGS) -mavx512ifma -mavx512vbmi -mavx512vbmi2 -mavx512bitalg -mavx512vnni -mavx512vpopcntdq

#----------------------------------------------------------------------------------------------------------------------
# PGO Configuration
#----------------------------------------------------------------------------------------------------------------------

LLVM_PROFDATA := llvm-profdata

ifeq ($(COMPILER),gcc)
    PGO_GEN := -fprofile-generate
    PGO_USE := -fprofile-use -fprofile-partial-training
    CXXFLAGS += -Wno-interference-size
else
    PGO_GEN := -fprofile-instr-generate=$(BUILD_DIR)/default.profraw
    PGO_MERGE := $(LLVM_PROFDATA) merge -output=$(BUILD_DIR)/merged.profdata $(BUILD_DIR)/default.profraw --sparse=true
    PGO_USE := -fprofile-instr-use=$(BUILD_DIR)/merged.profdata
endif

#----------------------------------------------------------------------------------------------------------------------
# Native CPU Feature Detection
#----------------------------------------------------------------------------------------------------------------------

SUPPORTED := $(shell echo | $(CC) -march=native -E -dM -)

NATIVE_ARCH_FLAGS :=

ifneq ($(findstring __POPCNT__, $(SUPPORTED)),)
    NATIVE_ARCH_FLAGS += -DUSE_POPCNT
endif

ifneq ($(findstring __BMI2__, $(SUPPORTED)),)
    ifeq ($(findstring __znver1, $(SUPPORTED)),)
        ifeq ($(findstring __znver2, $(SUPPORTED)),)
            NATIVE_ARCH_FLAGS += -DUSE_PEXT
        endif
    endif
endif

ifneq ($(findstring __AVX512VNNI__, $(SUPPORTED)),)
    ifeq ($(findstring __znver4, $(SUPPORTED)),)
        NATIVE_ARCH_FLAGS += -DUSE_AVX512_VNNI
    endif
endif

ifneq ($(findstring __AVX512F__, $(SUPPORTED)),)
    NATIVE_ARCH_FLAGS += -DUSE_AVX512
endif
ifneq ($(findstring __AVX2__, $(SUPPORTED)),)
    NATIVE_ARCH_FLAGS += -DUSE_AVX2
endif
ifneq ($(findstring __AVX__, $(SUPPORTED)),)
    NATIVE_ARCH_FLAGS += -DUSE_AVX
endif
ifneq ($(findstring __SSE4_2__, $(SUPPORTED)),)
    NATIVE_ARCH_FLAGS += -DUSE_SSE4
endif

#----------------------------------------------------------------------------------------------------------------------
# Build Target Template
#----------------------------------------------------------------------------------------------------------------------

# Usage: $(call BUILD_TARGET,name,cxxflags,ldflags,verbatim_flags)
define BUILD_TARGET
.PHONY: $(1)
$(1): EXE = $(BINARY_DIR)/$(PROJECT)-$(1)
$(1): CXXFLAGS += $(2) $(EXTRA_CXXFLAGS)
$(1): LDFLAGS += $(BASE_LDFLAGS) $(3)
$(1): VERBATIM_FLAGS = $(4)
$(1): verbatim_binary binary
endef

#----------------------------------------------------------------------------------------------------------------------
# Native and PGO Builds
#----------------------------------------------------------------------------------------------------------------------

.PHONY: pgo pgo-instrumented pgo-compile native

pgo: verbatim_binary
	$(MAKE) pgo-instrumented
	$(MAKE) pgo-compile

pgo-instrumented: EXE = $(BUILD_DIR)/$(PROJECT)-pgo-instrumented
pgo-instrumented: VERBATIM_FLAGS = $(NATIVE_FLAGS) $(NATIVE_ARCH_FLAGS)
pgo-instrumented: CXXFLAGS += $(NATIVE_FLAGS) $(NATIVE_ARCH_FLAGS) $(PGO_GEN) -fno-lto $(EXTRA_CXXFLAGS)
pgo-instrumented: LDFLAGS += $(BASE_LDFLAGS) $(PGO_GEN) -fno-lto
pgo-instrumented: binary
	./$(EXE) bench
	 $(PGO_MERGE)

pgo-compile: EXE = $(BINARY_DIR)/$(PROJECT)-pgo
pgo-compile: VERBATIM_FLAGS = $(NATIVE_FLAGS) $(NATIVE_ARCH_FLAGS)
pgo-compile: CXXFLAGS += $(NATIVE_FLAGS) $(NATIVE_ARCH_FLAGS) $(PGO_USE) $(EXTRA_CXXFLAGS)
pgo-compile: LDFLAGS += $(BASE_LDFLAGS) -flto $(PGO_USE)
pgo-compile: verbatim_binary binary

$(eval $(call BUILD_TARGET,native,$(NATIVE_FLAGS) $(NATIVE_ARCH_FLAGS),-flto,$(NATIVE_FLAGS) $(NATIVE_ARCH_FLAGS)))

#----------------------------------------------------------------------------------------------------------------------
# Debug and Sanitizer Builds
#----------------------------------------------------------------------------------------------------------------------

$(eval $(call BUILD_TARGET,debug,$(DEBUG_FLAGS),,$(BASE_FLAGS) -g))
$(eval $(call BUILD_TARGET,sanitize-address,$(SANITIZE_FLAGS) -fsanitize=address,-fsanitize=address,$(BASE_FLAGS) -g))
$(eval $(call BUILD_TARGET,sanitize-undefined,$(SANITIZE_FLAGS) -fsanitize=undefined,-fsanitize=undefined,$(BASE_FLAGS) -g))
$(eval $(call BUILD_TARGET,sanitize-thread,$(SANITIZE_FLAGS) -fsanitize=thread,-fsanitize=thread,$(BASE_FLAGS) -g))

#----------------------------------------------------------------------------------------------------------------------
# Special Builds
#----------------------------------------------------------------------------------------------------------------------

$(eval $(call BUILD_TARGET,tune,$(NATIVE_FLAGS) $(NATIVE_ARCH_FLAGS) -DTUNE,-flto,$(NATIVE_FLAGS) $(NATIVE_ARCH_FLAGS)))
$(eval $(call BUILD_TARGET,shuffle,$(NATIVE_FLAGS) $(NATIVE_ARCH_FLAGS) -DNETWORK_SHUFFLE -fopenmp,-flto -fopenmp,$(NATIVE_FLAGS) -DNETWORK_SHUFFLE))

#----------------------------------------------------------------------------------------------------------------------
# Release Builds for Specific Architectures
#----------------------------------------------------------------------------------------------------------------------

$(eval $(call BUILD_TARGET,legacy,$(RELEASE_FLAGS),-flto $(STATIC_LDFLAGS),$(BASE_FLAGS) $(OPT_RELEASE)))
$(eval $(call BUILD_TARGET,sse4,$(RELEASE_FLAGS) $(SSE4_FLAGS),-flto $(STATIC_LDFLAGS),$(BASE_FLAGS) $(OPT_RELEASE) -DUSE_SSE4))
$(eval $(call BUILD_TARGET,avx,$(RELEASE_FLAGS) $(AVX_FLAGS),-flto $(STATIC_LDFLAGS),$(BASE_FLAGS) $(OPT_RELEASE) -DUSE_SSE4 -DUSE_AVX))
$(eval $(call BUILD_TARGET,avx2,$(RELEASE_FLAGS) $(AVX2_FLAGS),-flto $(STATIC_LDFLAGS),$(BASE_FLAGS) $(OPT_RELEASE) -DUSE_SSE4 -DUSE_AVX -DUSE_AVX2))
$(eval $(call BUILD_TARGET,avx2-pext,$(RELEASE_FLAGS) $(AVX2_FLAGS) $(PEXT_FLAGS),-flto $(STATIC_LDFLAGS),$(BASE_FLAGS) $(OPT_RELEASE) -DUSE_SSE4 -DUSE_AVX -DUSE_AVX2 -DUSE_PEXT))
$(eval $(call BUILD_TARGET,avx512,$(RELEASE_FLAGS) $(AVX512_FLAGS),-flto $(STATIC_LDFLAGS),$(BASE_FLAGS) $(OPT_RELEASE) -DUSE_SSE4 -DUSE_AVX -DUSE_AVX2 -DUSE_PEXT -DUSE_AVX512))
$(eval $(call BUILD_TARGET,avx512vnni,$(RELEASE_FLAGS) $(AVX512VNNI_FLAGS),-flto $(STATIC_LDFLAGS),$(BASE_FLAGS) $(OPT_RELEASE) -DUSE_SSE4 -DUSE_AVX -DUSE_AVX2 -DUSE_PEXT -DUSE_AVX512 -DUSE_AVX512_VNNI))

.PHONY: release
release:
	$(MAKE) legacy
	$(MAKE) sse4
	$(MAKE) avx
	$(MAKE) avx2
	$(MAKE) avx2-pext
	$(MAKE) avx512
	$(MAKE) avx512vnni

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

$(BUILD_DIR)/%.o: % $(BUILD_DIR)/verbatim.nn FORCE
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