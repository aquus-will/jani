/* Header file for data dependence graph manager.
   Created: 10-mar-2008
   Last modified: 11-mar-2008
   Author: Willian dos Santos Lima*/

/* ========================================================================= */
#ifndef DDG
#define DDG

/* ------------------------------------------------------------------------- */

/* function to create the Data Dependence Graph*/
char ddg_gen ( const TInstr *int_code, u4 code_length );

/* fuuntion to verify input dependence between a pair of instructions*/
char ddg_check_input ( const TInstr *instr1, u1 def1, const TInstr *instr2, u1 def2 );

/* fuuntion to verify output dependence between a pair of instructions*/
char ddg_check_output ( const TInstr *instr1, u1 def1, const TInstr *instr2, u1 def2 );

/* fuuntion to verify anti-dependence between a pair of instructions*/
char ddg_check_anti ( const TInstr *instr1, u1 def1, const TInstr *instr2, u1 def2 );

/* ========================================================================= */

#endif
