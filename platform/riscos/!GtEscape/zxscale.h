/* --------------------------------------------------------------------------
 *    Name: zxscale.h
 * Purpose: ZX game scale dialogue handler
 *  Author: David Thomas
 * ----------------------------------------------------------------------- */

#ifndef ZXGAMESCALE_DLG_H
#define ZXGAMESCALE_DLG_H

#include "appengine/base/errors.h"
#include "appengine/wimp/dialogue.h"

extern dialogue_t *zxgamescale_dlg;

error zxgamescale_dlg_init(void);
void zxgamescale_dlg_fin(void);

#endif /* ZXGAMESCALE_DLG_H */
