/* Header file for the code generator.
   Created: 26-feb-2008
   Last modified: 26-feb-2008
   Author: Willian dos Santos Lima*/

/* ========================================================================= */
#ifndef CODEGEN
#define CODEGEN

/* ------------------------------------------------------------------------- */

/* function to generate binary code from the set of instructions*/
char code_gen ( const TInstr *int_code, u4 code_length );

/* ========================================================================= */
/* error codes for this phase:
   - 1, instruction fetching failure
   - 2, error in recording word on binary file

/* ========================================================================= */

#endif
