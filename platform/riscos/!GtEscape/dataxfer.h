/* --------------------------------------------------------------------------
 *    Name: dataxfer.h
 * Purpose: Data transfer
 * ----------------------------------------------------------------------- */

#ifndef DATAXFER_H
#define DATAXFER_H

#include "oslib/types.h"

#include "appengine/base/errors.h"

error dataxfer_init(void);
void dataxfer_fin(void);

#endif /* DATAXFER_H */
