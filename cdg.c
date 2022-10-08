/* File with implementations for the control dependence graph manager.
   Created: 04-apr-2008
   Last modified: 08-jul-2008
   Author: Willian dos Santos Lima*/

/* ========================================================================= */

#include "graphgen.h"

/* ------------------------------------------------------------------------- */
/* CDG global variables declaration*/

u4 end_node;   /*index for the CFG's end node*/
u1 *visited;

/* the so-called Control Dependence Graph -----------------------------------*/
TCDG cdg;

/* ------------------------------------------------------------------------- */

/** Function to create the control flow graph
  * Returns: - 0, for successful execution;
             - 1, for failure;
  ** Arguments:
  * * *int_code: the intermediate code
  * * *cfg: pointer to the Control Flow Graph
  * * code_length: the intermediate code's length
  */
char cdg_gen ( const TCFG *cfg ){
   register u4 i, j, count;
   FILE* _tmp_file;
   
   /* allocating space for CDG*/
   cdg.block = malloc( cfg->length * sizeof( TBasicBlock ) );
   if ( cdg.block == NULL ){
      err_++;
      if ( show_error( GRAPH_GEN, 4 ) )
         return 1;
   }
   
   /* initially, make CDG equals to CFG*/
   for ( i = 0; i < cfg->length; i++ ){
      cdg.block[i].idx_start = cfg->block[i].idx_start;
      cdg.block[i].idx_end = cfg->block[i].idx_end;
      /* overwrite neighbors information*/
      cdg.block[i].num_next = 0;
      /* pre-allocating space for node neighbors*/
      cdg.block[i].next_bb = malloc( cfg->length * sizeof( s4 ) );
   }
   cdg.length = cfg->length;
   
   /* identifying de end node t of CFG*/
   /* according to the generated CFG's properties, the end node will be the
      last basic block in its list*/
   end_node = cfg->length - 1;
   
   /* for each two nodes n (i) and n' (j) in CFG, verify control dependence*/
   visited = malloc( cfg->length * sizeof( u1 ) );
   for ( i = 0; i < cfg->length; i++ ){
      for ( j = 0; j < cfg->length; j++ ){
         if ( i != j && is_control_dep( i, j, cfg ) ){
            cdg.block[i].next_bb[cdg.block[i].num_next] = j;
            (cdg.block[i].num_next)++;
         }
      }
   }
   free( visited );
   
   /* writing for information data file*/
   /*i*/ _tmp_file = fopen( "output/cdg.dat", "w" );
   /*i*/ count = 0;
   /*i*/ for ( i = 0; i < cdg.length; i++ ){
   /*i*/    fprintf(_tmp_file, "B%d ->", i);
   /*i*/    for ( j = 0; j < cdg.block[i].num_next; j++ ){
   /*i*/       fprintf(_tmp_file, " B%d", cdg.block[i].next_bb[j]);
   /*i*/       count++;
   /*i*/    }
   /*i*/    fprintf(_tmp_file, "\n");
   /*i*/ }
   /*i*/ fprintf(_tmp_file, "\n# of edges: %d\n",count);
   /*i*/ fclose( _tmp_file );
   
   return 0;
}

/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */

/** Function check a control dependence between a pair of basic blocks
  * Returns: - 0, when no control dependence was found;
             - 1, when a control dependence was found;
  ** Arguments:
  * * n: one node
  * * m: another node
  * * *cfg: the control flow graph
  */
char is_control_dep ( u4 n, u4 m, const TCFG *cfg ){
   register u4 i;
   
   for ( i = 0; i < cfg->length; i++ )
      visited[i] = 0;
   
   /* try to reach end_node from n without passing through m*/
   cdg_dfs( n, m, cfg );
   
   if ( !visited[end_node] )
      return 1;
   
   return 0;
}

/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */

/** Function check control dependence via DFS
  * Returns: [none]
  ** Arguments:
  * * cuur: the block being currently visited
  * * avoid: the block that must be avoided
  * * *cfg: the control flow graph
  */
void cdg_dfs ( u4 curr, u4 avoid, const TCFG *cfg ){
   register u4 i;
   
   if ( curr == -1 )
      return;
   
   visited[curr] = 1;
   if ( curr == avoid )
      return;
   
   for ( i = 0; i < cfg->block[curr].num_next; i++ ){
      if ( !visited[cfg->block[curr].next_bb[i]] )
         cdg_dfs( cfg->block[curr].next_bb[i], avoid, cfg );
   }
}
