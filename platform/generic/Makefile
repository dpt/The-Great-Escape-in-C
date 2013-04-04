TOP=../..

.PHONY: usage
usage:
	@echo 'Usage:'
	@echo '  analyze	Perform a clang analyze run'
	@echo '  splint		Perform a splint run'
	@echo '  ctags		Generate ctags'

.PHONY: analyze
analyze:
	clang --analyze -std=c99 -Wall -Wextra -pedantic $(TOP)/libraries/TheGreatEscape/TheGreatEscape.c

.PHONY: splint
splint:
	splint -f $(TOP)/.splintrc $(TOP)/libraries/TheGreatEscape/TheGreatEscape.c

.PHONY: ctags
ctags:
	ctags -R .

.PHONY: clean
clean:
	@echo 'Cleaning...'

