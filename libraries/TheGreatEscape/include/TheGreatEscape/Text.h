#ifndef TEXT_H
#define TEXT_H

#include "TheGreatEscape/State.h"

uint8_t *plot_glyph(tgestate_t *state,
                    const char *pcharacter,
                    uint8_t    *output);
uint8_t *plot_single_glyph(tgestate_t *state,
                           int         character,
                           uint8_t    *output);

#endif /* TEXT_H */
