/* Header file for the file manipulation auxiliar. (File Manager)
   Created: 24-apr-2007
   Last modified: 24-apr-2007
   Author: Willian dos Santos Lima*/

/* ========================================================================= */
#ifndef FILEMAN
#define FILEMAN

/* ------------------------------------------------------------------------- */

#include "def.h"

/* ------------------------------------------------------------------------- */

/* 
/* function for 2-byte fields reading, inverting the bytes order at file*/
char fread2 ( u2 *field, FILE *_fp );

/* function for 4-byte fields reading, inverting the bytes order at file*/
char fread4 ( u4 *field, FILE *_fp );

/* ========================================================================= */

#endif
