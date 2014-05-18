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
NONCOMPILE=nonexistent.c

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
	SDK=$(DEVROOT)/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS7.1.sdk
	CLANG=$(DEVROOT)/Toolchains/XcodeDefault.xctoolchain/usr/bin/clang
	CROSSFLAGS=-isysroot $(SDK) -Os -arch armv7 -mthumb
	SANITIZE=
	CLANGFLAGS=-ferror-limit=2
	CC=$(CLANG) $(CLANGFLAGS) $(SANITIZE)
	LD=$(CLANG) $(SANITIZE)
else ifeq ($(TARGET),osx-any-clang)
	#CLANG=~/bin/clang+llvm-3.4/bin/clang
        #SANITIZE=-fsanitize=integer -fsanitize=address -fsanitize=undefined
	CLANG=clang
	CLANGFLAGS=-ferror-limit=2
	CC=$(CLANG) $(CLANGFLAGS) $(SANITIZE)
	LD=$(CLANG) $(SANITIZE)
else ifeq ($(TARGET),linux-arm-any)
	#CROSSFLAGS=-target arm-none-eabi -mcpu=arm7tdmi -mfloat-abi=soft
	# armv5-none-eabi -fpu=hard
	CC=gcc
	LD=gcc
else
	CC=gcc
	LD=gcc
endif

CTAGS=/usr/local/bin/ctags

# Tools (config)
#
INCLUDES=-I$(PUBLIC_INCLUDES) -I$(LIBRARIES) -I$(PRIVATE_INCLUDES)
WARNINGS=-Wall -Wextra -Wno-unused-parameter

# Tools (generic)
#
CFLAGS=-std=c99 $(WARNINGS) -MMD -MP $(INCLUDES) $(CROSSFLAGS)
LDFLAGS=$(CROSSFLAGS) $(LIBS)

ifeq ($(MODE),release)
  CFLAGS+=-O2 -DNDEBUG
else
  CFLAGS+=-g
endif

SRC=$(shell find $(LIBRARIES) $(PLATFORM_DIR) -type f \( -name '*.c' ! -name '$(NONCOMPILE)' \) -print)
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
	@echo '  astyle	Run astyle on the source'
	@echo
	@echo 'MODE=<release|debug>'

.PHONY: build
build: $(EXE)

$(EXE): $(OBJ)
	$(LD) $(LDFLAGS) -o $@ $^

.PHONY: analyze
analyze:
	$(CLANG) --analyze $(CFLAGS) -pedantic $(SRC)

.PHONY: lint
lint:
	splint -f $(ROOT)/.splintrc $(INCLUDES) $(SRC)

.PHONY: tags
tags:
	$(CTAGS) -R --sort=yes --c++-kinds=+p --fields=+iaS --extra=+q -f $(ROOT)/tags $(ROOT)

.PHONY: astyle
astyle:
	astyle --options=$(ROOT)/.astylerc --recursive "$(ROOT)/*.h" "$(ROOT)/*.c"

.PHONY: clean
clean:
	@echo 'Cleaning...'
	-rm -f $(OBJ)
	-rm -f $(DEP)
	-rm -f $(EXE)

-include $(DEP)

