/* File with implementations for the control flow graph manager.
   Created: 20-feb-2007
   Last modified: 27-dec-2008
   Author: Willian dos Santos Lima*/

/* ========================================================================= */

#include "graphgen.h"

/* ------------------------------------------------------------------------- */
/* CFG global variables declaration*/

/* the so-called Control Flow Graph -----------------------------------------*/
TCFG cfg;

/*TBasicBlock *cfg;
u4 cfg_length;    /* measured in numbers of basic blocks*/

/*TBasicBlock* cfg_get ( u4 *cfglen );
/* ------------------------------------------------------------------------- */

/* map: instruction -> basic block*/
/*s4 *basic_block_of;*/

/** Function to create the control flow graph
  * Returns: - 0, for successful execution;
             - 1, for failure;
  ** Arguments:
  * * *int_code: the intermediate code
  * * code_length: the intermediate code's length
  * * file_gen: whether create .dat file for information or not
  */
char cfg_gen ( const TInstr *int_code, u4 code_length, boolean file_gen ){
   register u4 i;
   FILE* _tmp_file;
   
   /* pre-allocating space for CFG*/
   cfg.block = malloc( code_length * sizeof( TBasicBlock ) );
   if ( cfg.block == NULL ){
      err_++;
      if ( show_error( GRAPH_GEN, 1 ) )
         return 1;
   }
   
   /* allocating space for target addresses list*/
   from_instr = malloc( code_length * sizeof( u4 ) );
   if ( from_instr == NULL ){
      err_++;
      if ( show_error( GRAPH_GEN, 1 ) )
         return 1;
   }
   
   /* finding all basic blocks*/
   cfg.basic_block_of = malloc( code_length * sizeof( u4 ) );
   cfg.length = 0;
   if ( cfg_find_basic_blocks( int_code, code_length ) ){
      err_++;
      if ( show_error( GRAPH_GEN, 2 ) )
         return 1;
   }
   
   /* linking basic blocks*/
   for ( i = 0; i < cfg.length; i++ ){
      /* inspect the last instruction of each basic block*/
      if ( is_branch[int_code[cfg.block[i].idx_end].op] ){
         /* it can follow throught 2 paths*/
         cfg.block[i].next_bb = malloc( 2 * sizeof( u4 ) );
         cfg.block[i].num_next = 2;
         cfg.block[i].next_bb[0] = i + 1 < cfg.length && int_code[cfg.block[i].idx_end].op != BR_OP ? i + 1 : -1;
         if ( cfg.block[i].idx_end + ( int_code[cfg.block[i].idx_end].imm16 / 4 ) + 1 < code_length )
            cfg.block[i].next_bb[1] = cfg.basic_block_of[cfg.block[i].idx_end + ( int_code[cfg.block[i].idx_end].imm16 / 4 ) + 1];
         else cfg.block[i].next_bb[1] = -1;
         /* cutting neighbors*/
         if ( cfg.block[i].next_bb[0] == -1 && cfg.block[i].next_bb[1] == -1 )
            cfg.block[i].num_next = 0;
         else if ( cfg.block[i].next_bb[0] == cfg.block[i].next_bb[1] )
                 cfg.block[i].num_next = 1;
         /* assigning parent*/
         if ( cfg.block[i].next_bb[0] != -1 )
            cfg.block[cfg.block[i].next_bb[0]].parent = i;
         if ( cfg.block[i].next_bb[1] != -1 )
            cfg.block[cfg.block[i].next_bb[1]].parent = i;
      }else{
         /* it can follow throught 1 path only*/
         cfg.block[i].next_bb = malloc( sizeof( u4 ) );
         cfg.block[i].num_next = 1;
         cfg.block[i].next_bb[0] = i + 1 < cfg.length && int_code[cfg.block[i].idx_end].op != BR_OP ? i + 1 : -1;
         /* cutting neighbors and assigning parent*/
         if ( cfg.block[i].next_bb[0] == -1 )
            cfg.block[i].num_next = 0;
         else cfg.block[i+1].parent = i;
      }
   }
   
   if ( file_gen ){
      /* writing for information data file*/
      /*i*/ _tmp_file = fopen( "output/cfg.dat", "a" );
      /*i*/ fprintf(_tmp_file, "\n\n");
      /*i*/ for ( i = 0; i < cfg.length; i++ ){
      /*i*/    if ( cfg.block[i].num_next > 0 && cfg.block[i].next_bb[0] != -1 )
      /*i*/       fprintf(_tmp_file, "B%d -> B%d\n",i,cfg.block[i].next_bb[0]);
      /*i*/    if ( cfg.block[i].num_next == 2 && cfg.block[i].next_bb[1] != -1 )
      /*i*/       fprintf(_tmp_file, "B%d -> B%d\n",i,cfg.block[i].next_bb[1]);
      /*i*/ }
      /*i*/ fclose( _tmp_file );
   }

   return 0;
}

/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */

/** Function to find all basic blocks
  * Returns: - 0, for successful execution;
             - 1, for failure;
  ** Arguments:
  * * *int_code: the intermediate code
  * * code_length: the intermediate code's length
  */
char cfg_find_basic_blocks ( const TInstr *int_code, u4 code_length ){
   register u4 i, j;
   TBasicBlock b;
   FILE* _txt;
   
   for ( i = 0; i < code_length; i++ )
      from_instr[i] = -1;
   
   for ( i = 0; i < code_length; i++ ){
      if ( is_branch[int_code[i].op] ){
         if ( i + ( int_code[i].imm16 / 4 ) + 1 < code_length )
            from_instr[i+(int_code[i].imm16/4)+1] = i;
      }
   }
   
   /* identifying basic blocks*/
   cfg.length = b.idx_start = b.idx_end = 0;
   cfg.basic_block_of[0] = cfg.length;
   for ( i = 1; i < code_length; i++ ){
      if ( from_instr[i] != -1 ){
         /* a target - always starting a basic block*/
         b.idx_end = i - 1;
         if ( b.idx_start <= b.idx_end ){
            cfg.block[cfg.length] = b;
            cfg.length++;
            b.idx_start = i;
         }
      }
      cfg.basic_block_of[i] = cfg.length;
      if ( is_branch[int_code[i].op] ){
         /* a branch - always ending a basic block*/
         b.idx_end = i;
         cfg.block[cfg.length] = b;
         cfg.length++;
         b.idx_start = i + 1;
      }
   }
   if ( b.idx_start < code_length ){
      b.idx_end = i - 1;
      cfg.block[cfg.length] = b;
      cfg.length++;
   }
   
   /* writing to information data file*/
   _txt = fopen( "output/cfg.dat", "w" );
   fprintf(_txt, "INFO: %d basic blocks were found!\n",cfg.length);
   for ( i = 0; i < cfg.length; i++ ){
      fprintf(_txt, "B%d (%d - %d):\n",i,cfg.block[i].idx_start,cfg.block[i].idx_end);
      for ( j = cfg.block[i].idx_start; j <= cfg.block[i].idx_end; j++ )
         fprintf(_txt, "\top = %s e opx = %s\n",op_name[int_code[j].op],opx_name[int_code[j].opx]);
   }
   fclose( _txt );
   
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
TCFG* cfg_get (  ){
   return &cfg;
}
