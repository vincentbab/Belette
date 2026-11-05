TARGET_NAME := belette
TARGET_BIN_DIR := ./build
SRC_DIR := ./src
ifeq ($(origin CXX),default)
	CXX := clang++
endif

ARCH ?= native
ifeq ($(ARCH),avx2)
	ARCH_FLAGS := -march=x86-64-v3
else ifeq ($(ARCH),avx512)
	ARCH_FLAGS := -march=x86-64-v4
else
	ARCH_FLAGS := -march=$(ARCH)
endif

ifneq ($(and $(findstring clang,$(shell $(CXX) --version)),$(findstring LLD,$(shell $(CXX) -fuse-ld=lld -Wl,--version 2>&1))),)
	LD_FLAGS := -fuse-ld=lld
endif

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

CPPFLAGS := -Wall -std=c++20 -fno-rtti $(ARCH_FLAGS) -D_CRT_SECURE_NO_WARNINGS
ifeq ($(TUNE),1)
	CPPFLAGS += -DTUNE
endif
CPPFLAGS_DEBUG := $(CPPFLAGS) -g -O0 -DDEBUG
CPPFLAGS_RELEASE := $(CPPFLAGS) -O3 -funroll-loops -finline -fomit-frame-pointer -flto -DNDEBUG

LDFLAGS := -Wall -std=c++20 -fno-rtti $(ARCH_FLAGS) $(LD_FLAGS) -static
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