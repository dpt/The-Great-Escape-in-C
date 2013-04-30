TOP=../..

INCLUDES=$(TOP)/include

CC=clang
LD=clang
CFLAGS=-std=c99 -Wall -Wextra -MMD -MP -I. -I$(INCLUDES)

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
	@echo '  splint	Perform a splint run'
	@echo '  ctags		Generate ctags'
	@echo '  astyle	Run astyle on the source'
	@echo
	@echo 'MODE=<release|debug>'

.PHONY: build
build: TheGreatEscape

TheGreatEscape: $(OBJ)
	$(LD) -o $@ $^

.PHONY: analyze
analyze:
	clang --analyze $(CFLAGS) -pedantic $(SRC)

.PHONY: splint
splint:
	splint -f $(TOP)/.splintrc -I$(INCLUDES) $(SRC)

.PHONY: ctags
ctags:
	ctags -R .

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

