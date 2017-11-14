#ifndef MENU_H
#define MENU_H

#include "TheGreatEscape/State.h"

void set_menu_item_attributes(tgestate_t *state,
                              int         index,
                              attribute_t attrs);

int menu_screen(tgestate_t *state);

#endif /* MENU_H */
