# Generic Makefile for The Great Escape
#
# by dpt

# Examples of use
#
# Build for OS X using clang, in release mode:
# make TARGET=osx-any-clang MODE=release build
#
# Build for OS X using clang, in debug mode, building in parallel:
# make TARGET=osx-any-clang -j build
#
# Build for iOS using clang in analyze mode:
# make TARGET=ios-arm-clang analyze
#
# Build using gcc:
# make build

# Project
#
PROJECT=TheGreatEscape
LIBS=
DONTCOMPILE=nonexistent.c

# Paths
#
ROOT:=$(abspath $(dir $(lastword $(MAKEFILE_LIST)))../..)
PUBLIC_INCLUDES=$(ROOT)/include
LIBRARIES=$(ROOT)/libraries
# Private includes would apply to all builds. It would be better if this
# were handled on a per-module basis.
PRIVATE_INCLUDES=$(LIBRARIES)/$(PROJECT)/include
PLATFORM_DIR=$(ROOT)/platform/generic
EXE=$(PROJECT)

# Tools (choices)
#
# Select a compiler based on TARGET
#
# Target is a quad consisting of:
#   <OS>-<CPU architecture>-<compiler>
# Where 'any' can be given as a wildcard element.
#
ifeq ($(TARGET),ios-arm-clang)
	DEVROOT=/Applications/Xcode.app/Contents/Developer
	SDK=$(DEVROOT)/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS8.0.sdk
	SDKBIN=$(DEVROOT)/Toolchains/XcodeDefault.xctoolchain/usr/bin
	CLANG=$(SDKBIN)/clang
	CROSSFLAGS=-isysroot $(SDK) -Os -arch armv7 -mthumb
	SANITIZE=
	CLANGFLAGS=-ferror-limit=2
	CC=$(CLANG) $(CLANGFLAGS) $(SANITIZE)
	LD=$(CLANG) $(SANITIZE)
else ifeq ($(TARGET),osx-any-clang)
	#CLANG=~/bin/clang+llvm-3.4/bin/clang
	CLANG=clang
	CLANGFLAGS=-ferror-limit=2
	#SANITIZE=-fsanitize=integer -fsanitize=address -fsanitize=undefined
	CC=$(CLANG) $(CLANGFLAGS) $(SANITIZE)
	LD=$(CLANG) $(SANITIZE)
else ifeq ($(TARGET),linux-arm-clang)
	CLANG=clang
	CROSSFLAGS=-target arm-none-eabi -march=arm7tdmi -mthumb -mfloat-abi=soft
	# or: armv5-none-eabi -fpu=hard
	CLANGFLAGS=-ferror-limit=2
	CC=$(CLANG) $(CLANGFLAGS) $(SANITIZE)
	LD=$(CLANG) $(SANITIZE)
else ifeq ($(TARGET),linux-any-clang)
	CLANG=clang
	CLANGFLAGS=-ferror-limit=2
	SANITIZE=
	CC=$(CLANG) $(CLANGFLAGS) $(SANITIZE)
	LD=$(CLANG) $(SANITIZE)
else ifeq ($(TARGET),linux-arm-gcc)
	GCC=arm-linux-gnueabi-gcc-4.7
	CROSSFLAGS=-mcpu=arm7tdmi -mthumb -mfloat-abi=soft
	CC=$(GCC)
	LD=$(GCC)
else
	CC=gcc
	LD=gcc
endif

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
	CTAGS=/usr/local/bin/ctags
else
	CTAGS=ctags
endif

# Tools (config)
#
INCLUDES=-I$(PUBLIC_INCLUDES) -I$(LIBRARIES) -I$(PLATFORM_DIR) -I$(PRIVATE_INCLUDES)
WARNINGS=-Wall -Wextra -Wno-unused-parameter

# Tools (generic)
#
CFLAGS=-std=c99 $(WARNINGS) -MMD -MP $(INCLUDES) $(CROSSFLAGS)
LDFLAGS=$(CROSSFLAGS) $(LIBS)
LDLIBS=$(LIBS)

ifeq ($(MODE),release)
  CFLAGS+=-Os -DNDEBUG
else
  CFLAGS+=-g
endif

SRC=$(shell find $(LIBRARIES) $(PLATFORM_DIR) -type f \( -name '*.c' ! -name '$(DONTCOMPILE)' \) -print)
OBJ=$(SRC:.c=.o)
DEP=$(SRC:.c=.d)

.c.o:;	$(CC) -c $(CFLAGS) $< -o $@

.PHONY: usage
usage:
	@echo 'Usage:'
	@echo '  build		Build'
	@echo '  clean		Clean a previous build'
	@echo '  analyze	Perform a clang analyze run'
	@echo '  lint		Perform a lint run'
	@echo '  tags		Generate tags'
	@echo '  cscope		Generate cscope database'
	@echo '  astyle	Run astyle on the source'
	@echo '  docs		Generate docs'
	@echo
	@echo 'MODE=<release|debug>'

.PHONY: build
build: $(EXE)

$(EXE): $(OBJ)
	$(LD) $(LDFLAGS) $^ $(LDLIBS) -o $@

.PHONY: analyze
analyze:
	$(CLANG) --analyze $(CFLAGS) -pedantic $(SRC)

.PHONY: lint
lint:
	splint -f $(ROOT)/.splintrc $(INCLUDES) $(SRC)

.PHONY: tags
tags:
	$(CTAGS) -R --sort=yes --c++-kinds=+p --fields=+iaS --extra=+q -f $(ROOT)/tags $(ROOT)

.PHONY: cscope
cscope:
	cscope -b -R

.PHONY: astyle
astyle:
	astyle --options=$(ROOT)/.astylerc --recursive "$(ROOT)/*.h" "$(ROOT)/*.c"

.PHONY: docs
docs:
	doxygen Doxyfile

.PHONY: clean
clean:
	@echo 'Cleaning...'
	-@rm -vf $(OBJ)
	-@rm -vf $(DEP)
	-@rm -vf $(EXE)
	-@rm -vf $(ROOT)/tags
	-@rm -vrf $(ROOT)/doxydocs

-include $(DEP)

