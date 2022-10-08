/* Header file for the register allocator.
   Created: 26-feb-2008
   Last modified: 27-dec-2008
   Author: Willian dos Santos Lima*/

/* ========================================================================= */
#ifndef REGALLOC
#define REGALLOC

/* ------------------------------------------------------------------------- */
/* a node for the interference graph*/
typedef struct INTERNODE{
   u4 var;           /* id. of the variable the node represents*/
   u1 color;         /* the node's color: 2..23*/
   u2 spill_cost;    /* node's spill cost*/
   u4 *neighbor;     /* vars that are simultaneously alive*/
   u4 num_neighbors; /* number of vars that are simultaneously alive*/
   u4 bkp_nn;        /* the backup value for num_neighbors*/
   boolean in_stack; /* if this vertex is pushed onto the vertex stack*/
}TInterNode;

/* ------------------------------------------------------------------------- */

/* function to perform the register allocation*/
char reg_alloc ( const TInstr *int_code, u4 code_length, const TCFG *cfg, const TDFG *dfg );

/* function to build the interference graph*/
char build_inter_graph ( const TInstr *int_code, const TCFG *cfg, const TDFG *dfg );

/* function to coalesce registers, returning whether any
   coalescing was performed*/
char coalesce_regs ( u4 *code_length, const TCFG *cfg );

/* function to substitute any ocurrence of r_old by r_new*/
void trav_sub ( u4 r_old, u4 r_new, u4 pos, const TCFG *cfg );

/* function to prune the graph and to fill the vertex stack*/
char prune_graph ( u4 *stack, u4 *stack_top );

/* function for registers assignment according to the stack built, and also
   for generating spill code*/
char assign_regs ( u4 *stack, u4 *stack_top );

/* ------------------------------------------------------------------------- */

/* function that returns the mininum color among a vertex neighbors*/
u1 min_color ( u4 v );

/* function that modifies the intermediate code to consider new spillings and
   real registers*/
char modif_code ( u1 color, u4 var );

/* function to generate spill code when necessary*/
char gen_spill_code ( u4 var );

/* function to insert an instruction into the intermediate code*/
char insert_instr ( TInstr instr, u4 pos );

/* function to build interference restrictions in a tree*/
char instr_restrictions ( u4 curr, u4 dest, const TCFG *cfg, u4 var );

/* ------------------------------------------------------------------------- */

/* comparison function for qsort - spill cost sorting*/
int cmp_spill_cost ( const void *par1, const void *par2 );

/* function to return no other modules the transformed intermediate code*/
TInstr* regalloc_get_code ( u4 *code_length );
/* ------------------------------------------------------------------------- */

/* ========================================================================= */
/* error codes for this phase:
   - 1, an error ocurred when trying to build the interference graph

/* ========================================================================= */

#endif
