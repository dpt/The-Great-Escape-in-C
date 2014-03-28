# Makefile for The Great Escape
#

TOP := $(abspath $(dir $(lastword $(MAKEFILE_LIST)))../..)

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
	CLANGFLAGS = -ferror-limit=2
	CC=$(CLANG)
	LD=$(CLANG)
else ifeq ($(TARGET),osx-any-clang)
	#CLANG = ~/bin/clang+llvm-3.4/bin/clang
	#SANITIZE=-fsanitize=integer -fsanitize=address
	CLANG = clang
	CLANGFLAGS = -ferror-limit=2
	CC=$(CLANG)
	LD=$(CLANG)
else ifeq ($(TARGET),linux-arm-any)
	#CROSSFLAGS=-target arm-none-eabi -mcpu=arm7tdmi -mfloat-abi=soft
	# armv5-none-eabi -fpu=hard
	CC = gcc
	LD = gcc
else
	CC = gcc
	LD = gcc
endif

INCLUDES=-I$(TOP)/include -I$(TOP)/libraries/TheGreatEscape/include
WARNINGS=-Wall -Wextra -Wno-unused-parameter

CTAGS=/usr/local/bin/ctags

CFLAGS=-std=c99 $(WARNINGS) -MMD -MP $(INCLUDES) $(CLANGFLAGS) $(CROSSFLAGS) $(SANITIZE)
LDFLAGS=$(CROSSFLAGS) $(SANITIZE)

ifeq ($(MODE),release)
  CFLAGS+=-O2 -DNDEBUG
else
  MODE=debug
  CFLAGS+=-g
endif

SRC=$(shell find $(TOP)/libraries $(TOP)/platform/generic -type f -name '*.c' -print)
OBJ=$(SRC:.c=.o)
DEP=$(SRC:.c=.d)

.c.o:;	$(CC) -c $(CFLAGS) $< -o $@

.PHONY: usage
usage:
	@echo 'Usage:'
	@echo '  build		Build'
	@echo '  analyze	Perform a clang analyze run'
	@echo '  lint	Perform a lint run'
	@echo '  tags		Generate tags'
	@echo '  astyle	Run astyle on the source'
	@echo
	@echo 'MODE=<release|debug>'

.PHONY: build
build: TheGreatEscape

TheGreatEscape: $(OBJ)
	$(LD) $(LDFLAGS) -o $@ $^

.PHONY: analyze
analyze:
	$(CLANG) --analyze $(CFLAGS) -pedantic $(SRC)

.PHONY: lint
lint:
	splint -f $(TOP)/.splintrc $(INCLUDES) $(SRC)

.PHONY: tags
tags:
	$(CTAGS) -R --sort=yes --c++-kinds=+p --fields=+iaS --extra=+q -f $(TOP)/tags $(TOP)

.PHONY: astyle
astyle:
	astyle --options=$(TOP)/.astylerc --recursive "$(TOP)/*.h" "$(TOP)/*.c"

.PHONY: clean
clean:
	@echo 'Cleaning...'
	-rm -f $(OBJ)
	-rm -f $(DEP)
	-rm -f TheGreatEscape

-include $(DEP)

