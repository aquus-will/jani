/* Header file for control dependence graph manager.
   Created: 04-apr-2008
   Last modified: 09-apr-2008
   Author: Willian dos Santos Lima*/

/* ========================================================================= */
#ifndef CDG
#define CDG

/* ------------------------------------------------------------------------- */

/* function to create the control dependence graph*/
char cdg_gen ( const TCFG *cfg );

/* function to check control dependende between two nodes of CFG*/
char is_control_dep ( u4 n, u4 m, const TCFG *cfg );

/* function to implement a DFS-based checking for control dependences*/
void cdg_dfs ( u4 curr, u4 avoid, const TCFG *cfg );

/* ========================================================================= */

#endif
