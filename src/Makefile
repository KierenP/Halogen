BUILD_DIR := ../build
BINARY_DIR := ../bin

SRCS := \
	BitBoardDefine.cpp \
	BoardState.cpp \
	EvalCache.cpp \
	EvalNet.cpp \
	main.cpp \
	Move.cpp \
	MoveGeneration.cpp \
	MoveList.cpp \
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
	Pyrrhic/tbprobe.c \
	td-leaf/HalogenNetwork.cpp \
	td-leaf/td-leaf-learn.cpp \
	td-leaf/TrainableNetwork.cpp \
	td-leaf/temporal_coherence.cpp

# String substitution for every C/C++ file.
# As an example, hello.cpp turns into ./build/hello.cpp.o
OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)

#----------------------------------------------------------------------------------------------------------------------
# Define compiler flags and instruction sets

WFLAGS = -Wall -Wextra -Wshadow -Wno-interference-size -Wno-unknown-warning-option -Wno-deprecated
AFLAGS = $(WFLAGS) -g -std=c++17 -fno-exceptions -ffast-math
CFLAGS = -O3 $(AFLAGS) -DNDEBUG -flto=auto -march=native
RFLAGS = -O3 $(AFLAGS) -DNDEBUG -flto=auto
# only enable Werror on debug builds, so we don't cause issues for people compiling Halogen on a different compiler
SFLAGS = -O1 $(AFLAGS) -Werror -fno-omit-frame-pointer
DFLAGS = -O0 $(AFLAGS) -Werror

LDFLAGS   = -Wl,--whole-archive -lpthread -Wl,--no-whole-archive -lm

# Different instruction sets targeting different architectures. For the AVX sets, we consider them a series where each level contains
# all previous sets instructions

# Nehalem (2008)
SSE4FLAGS = -DUSE_SSE4 -msse -msse2 -msse3 -mssse3 -msse4 -msse4.1 -msse4.2
# Sandy Bridge (2011)
AVXFLAGS = -DUSE_AVX $(SSE4FLAGS) -mavx -mfma
# Haswell (2013)
AVX2FLAGS = -DUSE_AVX2 $(AVXFLAGS) -mavx2 -DUSE_POPCNT -mpopcnt
# Skylake (2017)
AVX512FLAGS = -DUSE_AVX512 $(AVX2FLAGS) -mavx512f -mavx512cd -mavx512vl -mavx512dq -mavx512bw
# Skylake (2019)
AVX512VNNIFLAGS = -DUSE_AVX512_VNNI $(AVX512FLAGS) -mavx512ifma -mavx512vbmi -mavx512vbmi2 -mavx512bitalg -mavx512vnni -mavx512vpopcntdq

# AMD processors before Zen 3 implement PEXT in microcode, so we support bmi as a optional orthogonal configuration, on AVX2 and newer
PEXTFLAGS = -DUSE_PEXT -mbmi -mbmi2

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

# Detect if CXX is g++ or clang++, in this order.
ifeq '' '$(findstring clang++,$(CXX))'
  	CXXFLAGS += -fconstexpr-ops-limit=1000000000
  	$(info detected compiler is [g++])
else
  	CXXFLAGS += -fconstexpr-steps=100000000
  	$(info detected compiler is [clang++])
endif

#----------------------------------------------------------------------------------------------------------------------
# Optimized native builds, including PGO optimization. Detects supported instruction sets on the target

SUPPORTED = $(shell echo | $(CC) -march=native -E -dM -)

# Detect POPCNT
ifneq ($(findstring __POPCNT__, $(SUPPORTED)),)
	CFLAGS += -DUSE_POPCNT
endif

# Detect BMI2, but not on zen1 or zen2 architectures
ifneq ($(findstring __BMI2__, $(SUPPORTED)),)
	ifeq ($(findstring __znver1, $(SUPPORTED)),)
		ifeq ($(findstring __znver2, $(SUPPORTED)),)
			CFLAGS += -DUSE_PEXT
		endif
	endif
endif

# Detect the highest AVX instruction set supported

ifneq ($(findstring __AVX512VNNI__, $(SUPPORTED)),)
	CFLAGS += -DUSE_AVX512_VNNI
else ifneq ($(findstring __AVX512__, $(SUPPORTED)),)
	CFLAGS += -DUSE_AVX512
else ifneq ($(findstring __AVX2__, $(SUPPORTED)),)
	CFLAGS += -DUSE_AVX2
else ifneq ($(findstring __AVX__, $(SUPPORTED)),)
	CFLAGS += -DUSE_AVX
else ifneq ($(findstring __SSE4_2__, $(SUPPORTED)),)
	CFLAGS += -DUSE_SSE4
endif

.PHONY: pgo
pgo:
	$(MAKE) pgo-instrumented
	$(MAKE) pgo-compile

.PHONY: pgo-instrumented
pgo-instrumented: CXXFLAGS += $(CFLAGS) -fno-lto -fprofile-generate
pgo-instrumented: LDFLAGS += -fno-lto -fprofile-generate
pgo-instrumented: EXE = $(BUILD_DIR)/Halogen-pgo-instrumented.exe
pgo-instrumented: binary
	./$(EXE) bench

.PHONY: pgo-compile
pgo-compile: CXXFLAGS += $(CFLAGS) -fprofile-use
pgo-compile: LDFLAGS += -flto=auto -fprofile-use
pgo-compile: EXE = $(BINARY_DIR)/Halogen-pgo.exe
pgo-compile: binary

.PHONY: native
native: CXXFLAGS += $(CFLAGS)
native: LDFLAGS += -flto=auto
native: EXE = $(BINARY_DIR)/Halogen-native.exe
native: binary

#----------------------------------------------------------------------------------------------------------------------
# Debug and sanitizer builds

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

#----------------------------------------------------------------------------------------------------------------------
# Release builds that are statically linked and target specific instruction sets

.PHONY: legacy
legacy: CXXFLAGS += $(RFLAGS)
legacy: LDFLAGS += -flto=auto -static 
legacy: EXE = $(BINARY_DIR)/Halogen-legacy.exe
legacy: binary

.PHONY: sse4
sse4: CXXFLAGS += $(RFLAGS) $(SSE4FLAGS)
sse4: LDFLAGS += -flto=auto -static 
sse4: EXE = $(BINARY_DIR)/Halogen-sse4.exe
sse4: binary

.PHONY: avx
avx: CXXFLAGS += $(RFLAGS) $(AVXFLAGS)
avx: LDFLAGS += -flto=auto -static 
avx: EXE = $(BINARY_DIR)/Halogen-avx.exe
avx: binary

.PHONY: avx2
avx2: CXXFLAGS += $(RFLAGS) $(AVX2FLAGS)
avx2: LDFLAGS += -flto=auto -static 
avx2: EXE = $(BINARY_DIR)/Halogen-avx2.exe
avx2: binary

.PHONY: avx2-pext
avx2-pext: CXXFLAGS += $(RFLAGS) $(AVX2FLAGS) $(PEXTFLAGS)
avx2-pext: LDFLAGS += -flto=auto -static 
avx2-pext: EXE = $(BINARY_DIR)/Halogen-avx2-pext.exe
avx2-pext: binary

.PHONY: avx512
avx512: CXXFLAGS += $(RFLAGS) $(AVX512FLAGS)
avx512: LDFLAGS += -flto=auto -static 
avx512: EXE = $(BINARY_DIR)/Halogen-avx512.exe
avx512: binary

.PHONY: avx512-pext
avx512-pext: CXXFLAGS += $(RFLAGS) $(AVX512FLAGS) $(PEXTFLAGS)
avx512-pext: LDFLAGS += -flto -static 
avx512-pext: EXE = $(BINARY_DIR)/Halogen-avx512-pext.exe
avx512-pext: binary

.PHONY: avx512vnni
avx512vnni: CXXFLAGS += $(RFLAGS) $(AVX512VNNIFLAGS)
avx512vnni: LDFLAGS += -flto=auto -static 
avx512vnni: EXE = $(BINARY_DIR)/Halogen-avx512vnni.exe
avx512vnni: binary

.PHONY: avx512vnni-pext
avx512vnni-pext: CXXFLAGS += $(RFLAGS) $(AVX512VNNIFLAGS) $(PEXTFLAGS)
avx512vnni-pext: LDFLAGS += -flto=auto -static 
avx512vnni-pext: EXE = $(BINARY_DIR)/Halogen-avx512vnni-pext.exe
avx512vnni-pext: binary

.PHONY: release
release:
	$(MAKE) legacy
	$(MAKE) sse4
	$(MAKE) avx
	$(MAKE) avx2
	$(MAKE) avx2-pext
	$(MAKE) avx512
	$(MAKE) avx512-pext
	$(MAKE) avx512vnni
	$(MAKE) avx512vnni-pext

#----------------------------------------------------------------------------------------------------------------------

# The final build step.
.PHONY: binary
binary: $(OBJS)
	@ mkdir -p $(BINARY_DIR)
	$(CXX) $(OBJS) -o $(EXE) $(LDFLAGS)

# Build step for C++ source
$(BUILD_DIR)/%.o: % FORCE
	@ mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

.PHONY: clean_build
clean_build:
	rm -rf $(BUILD_DIR)

.PHONY: clean
clean: clean_build
	rm -rf $(BINARY_DIR)

# triggers make to always rebuild anything that depends on this
FORCE: