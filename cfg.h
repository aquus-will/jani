/* Header file for control flow graph manager.
   Created: 20-feb-2008
   Last modified: 18-jul-2008
   Author: Willian dos Santos Lima*/

/* ========================================================================= */
#ifndef CFG
#define CFG

/* ------------------------------------------------------------------------- */

/* function to create the control flow graph*/
char cfg_gen ( const TInstr *int_code, u4 code_length, boolean file_gen );

/* function to find the basic blocks*/
char cfg_find_basic_blocks ( const TInstr *int_code, u4 code_length );

/* function to return a pointer for CFG to other modules*/
TCFG* cfg_get ( void );

/* ========================================================================= */

#endif
