/* Header file for data flow graph manager.
   Created: 29-feb-2008
   Last modified: 18-jul-2008
   Author: Willian dos Santos Lima*/

/* ========================================================================= */
#ifndef DFG
#define DFG

/* ------------------------------------------------------------------------- */

/* function to create the control flow graph*/
char dfg_gen ( const TInstr *int_code, u4 code_length, const TCFG *cfg, boolean file_gen );

/* function to find all reachable uses from a definition*/
void dfg_find_uses ( u4 i_instr, const TInstr *int_code, const TCFG *cfg );

/* function for DFS, also responsible for connection DFG*/
void dfg_dfs ( u4 curr_block, u4 i_instr, u4 ivar, const TInstr *int_code, const TCFG *cfg );

/* function to do the analysis work*/
char dfg_analyse ( u4 i_instr, u4 i_instr_first, u4 i_var, const TInstr *int_code, const TCFG *cfg );

TDFG *dfg_get ( void );

/* ========================================================================= */

#endif
