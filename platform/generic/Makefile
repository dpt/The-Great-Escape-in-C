TOP=../..

INCLUDES=-I$(TOP)/include -I$(TOP)/libraries/TheGreatEscape/include
WARNINGS=-Wall -Wextra -Wno-unused-parameter

#CLANG=~/src/llvm-build/Debug+Asserts/bin/clang
CLANG=clang
CTAGS=/usr/local/bin/ctags

CC=$(CLANG)
LD=$(CLANG)
CFLAGS=-std=c99 $(WARNINGS) -MMD -MP $(INCLUDES) -ferror-limit=2
LDFLAGS=

SANITIZE=-fsanitize=integer -fsanitize=address

ifeq ($(MODE),release)
  CFLAGS+=-O2 -DNDEBUG
else
  MODE=debug
  CFLAGS+=-g
endif

SRC=$(shell find $(TOP)/libraries $(TOP)/platform/generic -type f -name '*.c' -print)
OBJ=$(SRC:.c=.o)
DEP=$(SRC:.c=.d)

.c.o:;	$(CC) -c $(CFLAGS) $(SANITIZE) $< -o $@

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
	$(LD) $(LDFLAGS) $(SANITIZE) -o $@ $^

.PHONY: analyze
analyze:
	clang --analyze $(CFLAGS) -pedantic $(SRC)

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

