/* --------------------------------------------------------------------------
 *    Name: zxsave.h
 * Purpose: Viewer save dialogue handler
 * ----------------------------------------------------------------------- */

#ifndef ZXGAMESAVE_DLG_H
#define ZXGAMESAVE_DLG_H

#include "appengine/base/errors.h"
#include "appengine/wimp/dialogue.h"

extern dialogue_t *zxgamesave_dlg;

error zxgamesave_dlg_init(void);
void zxgamesave_dlg_fin(void);

#endif /* ZXGAMESAVE_DLG_H */
