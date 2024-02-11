
export TARGET_EXEC := babchess
export BUILD_DIR := ./build
export SRC_DIR := ./src

CPPFLAGS := -Wall -fno-exceptions -fno-rtti -mbmi -mbmi2 -mpopcnt -msse
DEBUG_CPPFLAGS := $(CPPFLAGS) -g -O0
RELEASE_CPPFLAGS := $(CPPFLAGS) -O3 -funroll-loops -finline -fomit-frame-pointer -flto

LDFLAGS := -Wall -fno-exceptions -fno-rtti -mbmi -mbmi2 -mpopcnt -msse
DEBUG_LDFLAGS := $(LDFLAGS)
RELEASE_LDFLAGS := $(LDFLAGS) -flto

.PHONY: all debug release

all: debug release

release:
	$(MAKE) -f build.mk TARGET=Release CPPFLAGS='$(RELEASE_CPPFLAGS)' LDFLAGS='$(RELEASE_LDFLAGS)'

debug:
	$(MAKE) -f build.mk TARGET=Debug CPPFLAGS='$(DEBUG_CPPFLAGS)' LDFLAGS='$(DEBUG_LDFLAGS)'

clean:
	$(MAKE) -f build.mk clean TARGET=Debug
	$(MAKE) -f build.mk clean TARGET=Release