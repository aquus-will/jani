/* Header file for the errors and debugs handler.
   Created: 20-jan-2008
   Last modified: 15-jul-2008
   Author: Willian dos Santos Lima*/

/* ========================================================================= */
#ifndef ERROR
#define ERROR

/* ------------------------------------------------------------------------- */

#include "def.h"

#define UNKNOWN_ERROR printf(": Unknown. Trying to continue...\n")

/* ------------------------------------------------------------------------- */

/* function that will show an error message*/
char show_error ( TPhase p, u1 id );

/* function to help debugging*/
void lndb ( char *fname, u4 ln );

/* ------------------------------------------------------------------------- */

/* ========================================================================= */

#endif
