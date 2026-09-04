TARGET_NAME := belette
TARGET_BIN_DIR := ./build
SRC_DIR := ./src
CXX := clang++

EXE :=

TARGET_SUFFIX =
ifeq ($(OS), Windows_NT)
	TARGET_SUFFIX = .exe
endif

# Portable rm
ifeq ($(shell echo "x"),"x")
	RM := del /q
	FixPath = $(subst /,\,$1)
else
	RM := rm -f
	FixPath = $1
endif

SRCS := $(wildcard $(SRC_DIR)/*.cpp)

# idea from Stormphrax
PGO_EXEC := profile-belette
PGO_GENERATE := -fprofile-instr-generate
PGO_DATA := belette.profdata
PGO_MERGE := llvm-profdata merge -output=$(PGO_DATA) *.profraw
PGO_USE := -fprofile-instr-use=$(PGO_DATA)

CPPFLAGS := -Wall -std=c++20 -fno-rtti -mbmi -mbmi2 -mpopcnt -msse2 -msse3 -msse4.1 -mavx2 -D_CRT_SECURE_NO_WARNINGS
CPPFLAGS_DEBUG := $(CPPFLAGS) -g -O0 -DDEBUG
CPPFLAGS_RELEASE := $(CPPFLAGS) -O3 -funroll-loops -finline -fomit-frame-pointer -flto -DNDEBUG

LDFLAGS := -Wall -std=c++20 -fno-rtti -mbmi -mbmi2 -mpopcnt -msse2 -msse3 -msse4.1 -mavx2 -fuse-ld=lld -static
LDFLAGS_DEBUG := $(LDFLAGS)
LDFLAGS_RELEASE := $(LDFLAGS) -flto -static

OUT = $(if $(EXE),$(EXE),$(TARGET_BIN_DIR)/$(TARGET_EXEC)$(TARGET_SUFFIX))

.PHONY: all pgo debug release

all: release

pgo: $(SRCS)
# idea from Stormphrax
	$(eval TARGET_EXEC = $(TARGET_NAME)-pgo)
	$(CXX) $(CPPFLAGS_RELEASE) $(LDFLAGS_RELEASE) $(PGO_GENERATE) -o $(TARGET_BIN_DIR)/$(PGO_EXEC)$(TARGET_SUFFIX) $^
	$(call FixPath,$(TARGET_BIN_DIR)/$(PGO_EXEC)$(TARGET_SUFFIX)) bench
	-$(RM) $(call FixPath,$(TARGET_BIN_DIR)/$(PGO_EXEC)$(TARGET_SUFFIX))
	$(PGO_MERGE)
	$(CXX) $(CPPFLAGS_RELEASE) $(LDFLAGS_RELEASE) $(PGO_USE) -o $(OUT) $^
	-$(RM) *.profraw $(PGO_DATA)

release: $(SRCS)
	$(eval TARGET_EXEC = $(TARGET_NAME)-release)
	$(CXX) $(CPPFLAGS_RELEASE) $(LDFLAGS_RELEASE) -o $(OUT) $^

debug: $(SRCS)
	$(eval TARGET_EXEC = $(TARGET_NAME)-debug)
	$(CXX) $(CPPFLAGS_DEBUG) $(LDFLAGS_DEBUG) -o $(OUT) $^