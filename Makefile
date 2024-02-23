
export TARGET_EXEC := babchess
export BUILD_DIR := ./build
export SRC_DIR := ./src

CPPFLAGS := -Wall -std=c++20 -fno-rtti -mbmi -mbmi2 -mpopcnt -msse2 -msse3 -msse4.1 -mavx2
DEBUG_CPPFLAGS := $(CPPFLAGS) -g -O0 -DDEBUG
RELEASE_CPPFLAGS := $(CPPFLAGS) -O3 -funroll-loops -finline -fomit-frame-pointer -flto -DNDEBUG
PROFILE_CPPFLAGS := $(CPPFLAGS) -O3 -funroll-loops -finline -fomit-frame-pointer -flto -DNDEBUG -g -DPROFILING

LDFLAGS := -Wall -std=c++20 -fno-rtti -mbmi -mbmi2 -mpopcnt -msse2 -msse3 -msse4.1 -mavx2
DEBUG_LDFLAGS := $(LDFLAGS)
RELEASE_LDFLAGS := $(LDFLAGS) -flto
PROFILE_LDFLAGS := $(LDFLAGS) -flto -g

.PHONY: all debug release profile

all: debug release

release:
	$(MAKE) -f build.mk clean TARGET=Release
	$(MAKE) -f build.mk TARGET=Release CPPFLAGS="$(RELEASE_CPPFLAGS)" LDFLAGS="$(RELEASE_LDFLAGS)"

profile:
	$(MAKE) -f build.mk clean TARGET=Profile
	$(MAKE) -f build.mk TARGET=Profile CPPFLAGS="$(PROFILE_CPPFLAGS)" LDFLAGS="$(PROFILE_LDFLAGS)"

debug:
	$(MAKE) -f build.mk TARGET=Debug CPPFLAGS="$(DEBUG_CPPFLAGS)" LDFLAGS="$(DEBUG_LDFLAGS)"

clean:
	$(MAKE) -f build.mk clean TARGET=Debug
	$(MAKE) -f build.mk clean TARGET=Release