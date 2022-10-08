/* File with implementations for the file manipulation auxiliar. (File Manager)
   Created: 24-apr-2007
   Last modified: 18-jan-2008
   Author: Willian dos Santos Lima*/

/* ========================================================================= */

#include "fileman.h"

/** Function for 2-byte fields reading
  * Retorns: - 0, for sucessful execution;
             - 1, for failure;
  * Arguments:
  * * field: place to store de read value
  * * _fp: pointer to the file being read
  */
char fread2 ( u2 *field, FILE *_fp ){
   register u1 i;
   u1 buff1[2];
   u2 buff2;
   
   if ( fread( buff1, 1, 2, _fp ) != 2 )
      return 1;
   
   *field = 0;
   for ( i = 0; i < 2; i++ ){
      buff2 = buff1[i];
      buff2 = buff2 << 8 * (1-i);
      *field = *field | buff2;
   }
   
   return 0;
}

/* ------------------------------------------------------------------------- */

/** Function for 4-byte fields reading
  * Retorns: - 0, for sucessful execution;
             - 1, for failure;
  * Arguments:
  * * field: place to store de read value
  * * _fp: pointer to the file being read
  */
char fread4 ( u4 *field, FILE *_fp ){
   register u1 i;
   u1 buff1[4];
   u4 buff4;
   
   if ( fread( buff1, 1, 4, _fp ) != 4 )
      return 1;
   
   *field = 0;
   for ( i = 0; i < 4; i++ ){
      buff4 = buff1[i];
      buff4 = buff4 << 8 * (3-i);
      *field = *field | buff4;
   }
   
   return 0;
}
