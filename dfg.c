/* File with implementations for the data flow graph manager.
   Created: 29-feb-2008
   Last modified: 18-jul-2008
   Author: Willian dos Santos Lima*/

/* ========================================================================= */

#include "def.h"
#include "graphgen.h"

/* ------------------------------------------------------------------------- */
/* DFG global variables declaration*/

/* the so-called Data Flow Graph --------------------------------------------*/
TDFG dfg;
/* ------------------------------------------------------------------------- */

/* list of visited blocks by DFS*/
u2 *vis_block;

/* temporary list of destinies from a instruction in DFG*/
u4 *tmp_dest;
u4 last_dest;

/* ------------------------------------------------------------------------- */

/** Function to create the data flow graph. Get a definition of a variable in
    a instruction, and connects the instruction to others instructions where
    this variable is used and reached by the definition.
  * Returns: - 0, for successful execution;
             - 1, for failure;
  ** Arguments:
  * * *int_code: the intermediate code as a set of instructions
  * * code_length: number of instructions in the intermediate code
  * * *cfg: the CFG
  * * file_gen: whether create .dat file for information or not
  */
char dfg_gen ( const TInstr *int_code, u4 code_length, const TCFG *cfg, boolean file_gen ){
   register u4 i, j, counter;
   FILE* _tmp_file;
   
   /* each vertex in the DFG will be an index for an intermediate instruction*/
   dfg.instruction = malloc( code_length * sizeof( u4 ) );
   if ( dfg.instruction == NULL ){
      err_++;
      if ( show_error( GRAPH_GEN, 3 ) )
         return 1;
   }
   
   /* each vertex could have a number of neighbors (indices for int. instructions)*/
   dfg.instr_neighbor = malloc( code_length * sizeof( u4* ) );
   dfg.num_neighbors = malloc( code_length * sizeof( u4 ) );
   if ( dfg.instr_neighbor == NULL || dfg.num_neighbors == NULL ){
      err_++;
      if ( show_error( GRAPH_GEN, 3 ) )
         return 1;
   }
   
   /* a temporary list of neighbors*/
   tmp_dest = malloc( code_length * sizeof( u4 ) );
   if ( tmp_dest == NULL ){
      err_++;
      if ( show_error( GRAPH_GEN, 3 ) )
         return 1;
   }
   
   /* there's exactly one vertex for each int. instruction*/
   dfg.length = code_length;
   for ( i = 0; i < dfg.length; i++ )
      dfg.instruction[i] = i, dfg.num_neighbors[i] = 0;
   
   /* for each definition encountered, find its uses*/
   for ( i = 0; i < dfg.length; i++ ){
      j = dfg.instruction[i];
      /* identifying the instruction type and whether it defines a value or not*/
      if ( int_code[j].type == RTYPE ){
         if ( is_rdefine[int_code[j].opx] )
            dfg_find_uses( j, int_code, cfg );
      }else{
         /* I-TYPE or J-TYPE*/
         if ( is_idefine[int_code[j].op] )
            dfg_find_uses( j, int_code, cfg );
      }
   }
   
   free( tmp_dest );
   
   if ( file_gen ){
      /* writing to information data file*/
      /*i*/ counter = 0;
      /*i*/ _tmp_file = fopen( "output/dfg.dat", "w" );
      /*i*/ for ( i = 0; i < dfg.length; i++ ){
      /*i*/    fprintf(_tmp_file, "Instr. %03d:", dfg.instruction[i]);
      /*i*/    for ( j = 0; j < dfg.num_neighbors[dfg.instruction[i]]; j++ ){
      /*i*/       fprintf(_tmp_file, "  %d", dfg.instr_neighbor[i][j]);
      /*i*/       counter++;
      /*i*/    }
      /*i*/    fprintf(_tmp_file, "\n");
      /*i*/ }
      /*i*/ fprintf(_tmp_file, "# of edges: %d\n", counter);
      /*i*/ fclose( _tmp_file );
   }
   
   return 0;
}

/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */

/** Function to find uses given a definition, and connects the edges in DFG. It
    traverses the CFG beginning at the basic block which contains the instruction
    passed as parameter, find all basic blocks that can be reached from the
    first and analyse the instructions of these basic blocks. If another
    definition for the same variable is found, the search finishes at that path.
    Makes use of DFS over CFG.
  * Returns: [none]
  * Arguments:
  * * i_instr: index to the instruction where the value is defined
  * * *int_code: pointer to the intermediate code
  * * *cfg: pointer to the Control Flow Graph
  */
void dfg_find_uses ( u4 i_instr, const TInstr *int_code, const TCFG *cfg ){
   register u4 i;
   u4 ib = cfg->basic_block_of[i_instr], ivar;
   
   /* finding the variable that is being defined*/
   if ( int_code[i_instr].type == RTYPE )
      ivar = int_code[i_instr].rc;
   else ivar = int_code[i_instr].rb;
   if ( ivar == REG0 )
      return;
   
   last_dest = 0;
   if ( i_instr < cfg->block[ib].idx_end )
      i = dfg_analyse( i_instr, i_instr + 1, ivar, int_code, cfg );
   
   if ( !i ){
      /* Depth-First Search over CFG*/
      vis_block = malloc( cfg->length * sizeof( u2 ) );
      for ( i = 0; i < cfg->length; i++ )
         vis_block[i] = 0;
      dfg_dfs( ib, i_instr, ivar, int_code, cfg );
      free( vis_block );
   }
   /* including edges in the DFG*/
   dfg.num_neighbors[i_instr] = last_dest;
   dfg.instr_neighbor[i_instr] = malloc( last_dest * sizeof( u4 ) );
   for ( i = 0; i < last_dest; i++ )
      dfg.instr_neighbor[i_instr][i] = tmp_dest[i];
   last_dest = 0;
}

/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */

/** Function to implement Depth-First Search over the blocks of CFG, and
    create edges for DFG (recursion).
  * Returns: [none]
  * Arguments:
  * * curr_block: index for the current basic block
  * * ivar: the variable's number that are to be analysed
  * * *int_code: the intermediate code as a set of instructions
  * * *cfg: the CFG
  */
void dfg_dfs ( u4 curr_block, u4 i_instr, u4 ivar, const TInstr *int_code, const TCFG *cfg ){
   u4 i;
   char r;
   
   for ( i = 0; i < cfg->block[curr_block].num_next; i++ ){
      if ( cfg->block[curr_block].next_bb[i] != -1 && !vis_block[cfg->block[curr_block].next_bb[i]] ){
         /* analyse the neighbor and...*/
         r = dfg_analyse( i_instr, cfg->block[cfg->block[curr_block].next_bb[i]].idx_start, ivar, int_code, cfg );
         /* ...mark it as visited and visit it, if should*/
         vis_block[cfg->block[curr_block].next_bb[i]] = 1;
         if ( !r )
            dfg_dfs( cfg->block[curr_block].next_bb[i], i_instr, ivar, int_code, cfg );
      }
   }
}

/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */

/** Function to analyse a basic block serching for data flow
  * Returns: - 0, if no definition for the variable was encountered.
             - 1, if a new definition for the same variable was found.
  * Arguments:
  * * i_instr: index for the instruction in intermediate code
  * * i_instr_first: the first instruction to be analysed within the block
  * * i_var: number of the variable to be analysed
  * * *int_code: the intermediate code as a set of instructions
  * * *cfg: the CFG
  */
char dfg_analyse ( u4 i_instr, u4 i_instr_first, u4 i_var, const TInstr *int_code, const TCFG *cfg ){
   register u4 i;
   u4 ib = cfg->basic_block_of[i_instr_first];
   
   /* analyse this block untill its end*/
   for ( i = i_instr_first; i <= cfg->block[ib].idx_end; i++ ){
      if ( int_code[i].type == RTYPE ){
         if ( int_code[i].ra == i_var || int_code[i].rb == i_var ){
            /* new egde from i_instr to i in DFG*/
            tmp_dest[last_dest] = i;
            last_dest++;
         }
         /* verify whether another definition for the same variable is reached*/
         if ( is_rdefine[int_code[i].opx] && int_code[i].rc == i_var )
            return 1;
         else if ( int_code[i].rc == i_var ){
            /* new egde from i_instr to i in DFG*/
            tmp_dest[last_dest] = i;
            last_dest++;
         }
      }else{
         if ( int_code[i].ra == i_var ){
            /* new egde from i_instr to i in DFG*/
            tmp_dest[last_dest] = i;
            last_dest++;
         }
         /* verify whether another definition for the same variable is reached*/
         if ( is_idefine[int_code[i].op] && int_code[i].rb == i_var )
            return 1;
         else if ( int_code[i].rb == i_var ){
            /* new egde from i_instr to i in DFG*/
            tmp_dest[last_dest] = i;
            last_dest++;
         }
      }
   }
   
   return 0;
}

/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */

/** Function to return the Control Flow Graph
  * Returns: the CFG structure
  * Arguments:
  * * [none]
  */
TDFG* dfg_get (  ){
   return &dfg;
}
