/* File with implementations for the register allocator functions.
   Created: 26-feb-2008
   Last modified: 03-feb-2009
   Author: Willian dos Santos Lima*/

/* ========================================================================= */

#include "def.h"
#include "graphgen.h"
#include "cfg.h"
#include "bypass.h"
#include "regalloc.h"

/* ------------------------------------------------------------------------- */
/* register allocator global variables declaration*/

/* the lower/higher used variable*/
u4 first_variable, last_variable;

/* the intermediate code after register allocation ------------------------- */
TInstr *regalloc_int_code;
u4 ra_code_position;

/* instructions that must be descarted when this phase concludes*/
boolean *int_deleted;

/* the interference graph - adj. lists*/
TInterNode *ig;
u4 ig_length;
/* the interference graph - adj. matrix*/
u1 **inter;
/* information about the graphs*/
u4 num_edges, num_nodes;
/* number of spilled variables and the spilled vars*/
u1 *spilled_vars;
u4 num_spill;

/* ------------------------------------------------------------------------- */

/** Function to perform the register allocation policy
  * Returns: - 0, for successful execution;
             - 1, for failure;
  * Arguments:
  * * *int_code: the intermediate code
  * * code_length: the intermediate code's length
  * * *cfg: pointer to CFG
  * * *dfg: pointer to DFG
  */
char reg_alloc ( const TInstr *int_code, u4 code_length, const TCFG *cfg, const TDFG *dfg ){
   register u4 i, j, cnt;
   FILE* _tmp_file;
   u4 *stack, stack_top, old_nedges;
   boolean first_ig = TRUE;
   
   num_nodes = code_length;
   num_spill = 1;
   
   /* allocating memory space for modified intermediate code*/
   regalloc_int_code = malloc( 3 * code_length * sizeof( TInstr ) );    /*more space for some spill code*/
   int_deleted = malloc( code_length * sizeof( boolean ) );
   
   /* initializing RA code*/
   for ( i = 0; i < code_length; i++ )
      regalloc_int_code[i] = int_code[i], int_deleted[i] = FALSE;
   
   /* finding the lower/higher index variable out of the used variables*/
   last_variable = OFFSET_VARS;
   first_variable = INT_MAX;
   for ( i = 0; i < code_length; i++ ){
      last_variable = regalloc_int_code[i].ra > last_variable ? regalloc_int_code[i].ra : last_variable;
      last_variable = regalloc_int_code[i].rb > last_variable ? regalloc_int_code[i].rb : last_variable;
      last_variable = regalloc_int_code[i].rc > last_variable ? regalloc_int_code[i].rc : last_variable;
      first_variable = regalloc_int_code[i].ra < first_variable && regalloc_int_code[i].ra >= OFFSET_VARS ? regalloc_int_code[i].ra : first_variable;
      first_variable = regalloc_int_code[i].rb < first_variable && regalloc_int_code[i].rb >= OFFSET_VARS ? regalloc_int_code[i].rb : first_variable;
      first_variable = regalloc_int_code[i].rc < first_variable && regalloc_int_code[i].rc >= OFFSET_VARS ? regalloc_int_code[i].rc : first_variable;
   }
   
   ig_length = last_variable;
   
   /* structure that tracks variable spilled to memory*/
   spilled_vars = malloc ( ( 2 + ig_length ) * sizeof( u1 ) );
   for ( i = 0; i < 2 + ig_length; i++ ){
      if ( i > 24 )
         spilled_vars[i] = (u1) 0;
      else spilled_vars[i] = (u1) 1;
   }
   
   /* creating the interference graph only if there are more variables than registers*/
   if ( last_variable - first_variable + 1 > NIOS_NUM_GPREG ){
      /* initializing adj. matrix graph*/
      inter = malloc( ( ig_length + 2 ) * sizeof( u1* ) );
      for ( i = 0; i < ig_length + 2; i++ )
         inter[i] = malloc( ( ig_length + 2 ) * sizeof( u1 ) );
      /* initializing adj. lists graph and each node's color*/
      ig = malloc( ( 2 + ig_length ) * sizeof( TInterNode ) );
      
      for ( i = 0; i < ig_length + 2; i++ )
         ig[i].color = 0;  /* uncolored*/
      
      /* Chaitin-Briggs register allocation policy*/
      old_nedges = 0;
      while ( TRUE ){
         /* rebuilding DFG for the iteration*/
         /* -because CFG will not be used ahead, we can overwrite it*/
         if ( cfg_gen( regalloc_int_code, code_length, FALSE ) ){
            err_++;
            if ( show_error( REGISTER_ALLOC, 1 ) )
               return 1;
         }
         /* -with this new CFG, we can build the new DFG, also overwritting it*/
         if ( dfg_gen( regalloc_int_code, code_length, cfg_get(), FALSE ) ){
            err_++;
            if ( show_error( REGISTER_ALLOC, 1 ) )
               return 1;
         }
         
         /* BE WARE OF NEW MEMORY ALLOCATION*/
         /* building interference graph*/
         if ( !build_inter_graph( regalloc_int_code, cfg_get(), dfg_get() ) ){
            err_++;
            if ( show_error( REGISTER_ALLOC, 1 ) )
               return 1;
         }
         
         if ( first_ig ){
            cnt = 0;
            /* writing information to file - first interference graph built*/
            /*i*/ _tmp_file = fopen( "output/inter_graph.dat", "w" );
            /*i*/ fprintf( _tmp_file, "ig_length = %d\n   ", ig_length );
            /*i*/ for ( j = 2; j < 2 + ig_length; j++ )
            /*i*/    fprintf( _tmp_file, "%2d: ", j );
            /*i*/ fprintf( _tmp_file, "\n" );
            /*i*/ for ( i = 2; i < 2 + ig_length; i++ ){
            /*i*/    fprintf( _tmp_file, "%2d: ", i );
            /*i*/    for ( j = 2; j < 2 + ig_length; j++ ){
            /*i*/       fprintf( _tmp_file, "%02d  ", inter[i][j] );
            /*i*/       if ( inter[i][j] )
            /*i*/          cnt++;
            /*i*/    }
            /*i*/    fprintf( _tmp_file, "\n" );
            /*i*/ }
            /*i*/ fprintf( _tmp_file, "\n%d edges.\n", cnt );
            /*i*/ fclose( _tmp_file );
            first_ig = FALSE;
         }
         
         /* verifying continuation condition*/
         if ( !num_edges /*num_edges == old_nedges*/ )
            break;
         old_nedges = num_edges;
         
         /* registers coalescing*/
         if ( /*!coalesce_regs( &code_length, cfg )*/ 1 ){
            /* sorting nodes by spill cost*/
            qsort( ig, ig_length+2, sizeof(TInterNode), cmp_spill_cost );
            
            /* assigning colors (registers)*/
            stack = malloc( (ig_length + 2) * sizeof( u4 ) );
            stack_top = 0;
            prune_graph( stack, &stack_top );
            assign_regs( stack, &stack_top );
            code_length = ra_code_position;  /* because of gen_spill_code*/
            
            free( stack );
         }
      }
      
      /* destroying interference graph structures - REVIEW!!!*/
      /*free( ig );
      for ( i = 0; i < ig_length + 2; i++ )
         free( inter[i] );
      free( inter );*/
   }else{
      /* if there are less variables than registers, just make a simpler policy*/
      /* for optimization reasons, we make at least one registers coalescing*/
      /*coalesce_regs( &code_length, cfg );*/
      
      /* normalizing the variables indices*/
      for ( i = 0; i < code_length; i++ ){
         if ( regalloc_int_code[i].ra >= OFFSET_VARS )
            regalloc_int_code[i].ra = regalloc_int_code[i].ra - first_variable + 2;
         if ( regalloc_int_code[i].rb >= OFFSET_VARS )
            regalloc_int_code[i].rb = regalloc_int_code[i].rb - first_variable + 2;
         if ( regalloc_int_code[i].rc >= OFFSET_VARS )
            regalloc_int_code[i].rc = regalloc_int_code[i].rc - first_variable + 2;
      }
      /* writing information to file - last interference graph built*/
      /*i*/ _tmp_file = fopen( "output/inter_graph.dat", "w" );
      /*i*/ fprintf( _tmp_file, "No interference graph built.\n" );
      /*i*/ fclose( _tmp_file );
   }
   ra_code_position = code_length;
   
   /* at last, adjusting the displacements registers*/
   regalloc_int_code[0].imm16 = code_length * 4;
   regalloc_int_code[1].imm16 = regalloc_int_code[0].imm16 + num_spill * 4;
   
   /* writing information to file - changed intermediate code*/
   /*i*/ _tmp_file = fopen( "output/ra_intcode.dat", "w" );
   /*i*/ for ( i = 0; i < code_length; i++ )
   /*i*/    if ( regalloc_int_code[i].type == RTYPE )
   /*i*/       fprintf( _tmp_file, "%3d - %s: A = %d, B = %d, C = %d e IMM16 = %d\n",i,
   /*i*/                opx_name[regalloc_int_code[i].opx], regalloc_int_code[i].ra,
   /*i*/                regalloc_int_code[i].rb, regalloc_int_code[i].rc, regalloc_int_code[i].imm16 );
   /*i*/    else fprintf( _tmp_file, "%3d - %s: A = %d, B = %d, C = %d e IMM16 = %d\n",i,
   /*i*/                  op_name[regalloc_int_code[i].op], regalloc_int_code[i].ra,
   /*i*/                  regalloc_int_code[i].rb, regalloc_int_code[i].rc, regalloc_int_code[i].imm16 );
   /*i*/ fclose( _tmp_file );
   
   /* some destruction*/
   free( int_deleted );
   free( spilled_vars );
   
   return 0;
}

/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */

/** Function to build the interference graph. Here, we assume that every
    variable might be allocated in any general purpose register
  * Returns: - 0, for success;
             - 1, for failure;
  * Arguments:
  * * *int_code: the intermediate code
  * * *cfg: pointer to the CFG
  * * *dfg: pointer to the DFG
  */
char build_inter_graph ( const TInstr *int_code, const TCFG *cfg, const TDFG *dfg ){
   register u4 i, j, ins, var, last, from, to;
   
   num_edges = 0;
   
   /* initializing adj. matrix*/
   for ( i = 0; i < 2 + ig_length - 1; i++ )
      for ( j = i + 1; j < 2 + ig_length; j++ )
         inter[i][j] = inter[j][i] = 0;
   
   /* traversing DFG*/
   for ( i = 0; i < dfg->length; i++ ){
      if ( int_code[dfg->instruction[i]].type == RTYPE )
         var = int_code[dfg->instruction[i]].rc;
      else var = int_code[dfg->instruction[i]].rb;
      
      /* find uses untill the (possible) definition reaches*/
      if ( !spilled_vars[var] && dfg->num_neighbors[i] > 0 ){
         from = i;
         for ( ins = 0; ins < dfg->num_neighbors[i]; ins++ ){
            /* mark the variables that are used in the use instruction*/
            to = dfg->instr_neighbor[i][ins];
            /* apply restrictions to all variables defined within the range of
               instructions between 'ins' and the last instruction at DFG - use CFG*/
            for ( j = 0; j < cfg->length; j++ )
               cfg->block[j].flag = FALSE;
            instr_restrictions( from, to, cfg, var );
         }
      }
   }
   
   /* a node never interfere with itself*/
   for ( i = 0; i < 2 + ig_length; i++ )
      inter[i][i] = 0;
      
   /* node 0 is special and must not be in interference graph*/
   for ( i = 0; i < 2 + ig_length; i++ )
      inter[i][0] = inter[0][i] = 0;
   
   /* reporting only variables that interfere - TEMP*/
   /*printf("&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&\n");
   for ( i = 0; i < 2 + ig_length; i++ ){
      for ( j = 0; j < 2 + ig_length; j++ ){
         if ( inter[i][j] )
            printf("%u interfere com %u.\n",i,j);
      }
   }
   printf("&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&\n");*/
   
   /* writing graph to adj. lists representation*/
   for ( i = 0; i < 2 + ig_length; i++ ){
      ig[i].var = i;
      ig[i].spill_cost = 1;  /*initial value*/
      ig[i].num_neighbors = 0;
      ig[i].in_stack = FALSE;
      for ( j = 0; j < 2 + ig_length; j++ )
         if ( inter[i][j] )
            (ig[i].num_neighbors)++;
      ig[i].neighbor = malloc( ig[i].num_neighbors * sizeof( u4 ) );
      last = 0;
      for ( j = 0; j < 2 + ig_length; j++ )
         if ( inter[i][j] ){
            ig[i].neighbor[last] = j;
            last++;
         }
   }
   /*printf("====================\n");*/
}

/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */

/** Function to coalesce registers
  * Returns: - 0, if no coalescing was performed;
             - 1, if any coalescing was performed;
  * Arguments:
  * * *code_length: number of instructions at intermediate code
  * * *cfg: pointer to CFG
  */
char coalesce_regs ( u4 *code_length, const TCFG *cfg ){
   register u4 i, k;
   boolean coalescing = FALSE;
   u4 j;
   
   /* analyzing each instruction at intermediate code to catch move
      instructions between two registers*/
   for ( i = 0; i < *code_length; i++ ){
      if ( regalloc_int_code[i].type == RTYPE && regalloc_int_code[i].opx == ADD_OPX )
         if ( regalloc_int_code[i].ra == 0 && regalloc_int_code[i].rb != 0
           || regalloc_int_code[i].ra != 0 && regalloc_int_code[i].rb == 0 ){
            /* move instruction found*/
            if ( !( i > 0 && regalloc_int_code[i-1].op == ADDI_OP ) )
               continue;
            coalescing = TRUE;
            int_deleted[i] = TRUE;
            for ( j = 0; j < cfg->length; j++ )
               cfg->block[j].flag = FALSE;
            trav_sub( regalloc_int_code[i].ra != 0 ? regalloc_int_code[i].ra : regalloc_int_code[i].rb, regalloc_int_code[i].rc, i-1, cfg );
         }
   }
   
   /* removing deleted instructions from intermediate code, taking branches into account*/
   if ( coalescing ){
      for ( i = 0; i < *code_length; i++ ){
         if ( int_deleted[i] ){
            /* if instruction is deleted, update branches related to it:*/
            /* - with branch address before instruction 'i'*/
            for ( k = 0; k < i; k++ ){
               if ( is_branch[regalloc_int_code[k].op] && regalloc_int_code[k].imm16 + k * 4 >= i * 4 )
                  regalloc_int_code[k].imm16 -= 4;
            }
            /* - with branch address after instruction 'i'*/
            for ( k = i + 1; k < *code_length; k++ ){
               if ( is_branch[regalloc_int_code[k].op] && regalloc_int_code[k].imm16 + k * 4 < i * 4 )
                  regalloc_int_code[k].imm16 += 4;
            }
         }
      }
   }
   
   if ( coalescing ){
      j = 0;
      for ( i = 0; i < *code_length; i++ ){
         if ( !int_deleted[i] ){
            if ( i != j )
               regalloc_int_code[j] = regalloc_int_code[i];
            j++;
         }
      }
      *code_length -= ( i - j );
      ra_code_position = *code_length;
   }
   
   return coalescing;
}


/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */

/** Function to substitute any ocurrence of r_old by r_new at all code lines
    that may be reached by program's control flow - uses DFS over CFG
  * Returns: [none]
  * Arguments:
  * * r_old: the register which's ocurrence would be replaced
  * * r_new: the register that will replace ocurrences of r_old
  * * pos: code's position of coalesced instruction
  * * *cfg: pointer to CFG
  */
void trav_sub ( u4 r_old, u4 r_new, u4 pos, const TCFG *cfg ){
   register u4 i;
   u4 bb;
   
   /* perfoming replacements within one basic block*/
   bb = cfg->basic_block_of[pos];
   for ( i = pos; i <= cfg->block[bb].idx_end; i++ ){
      /* may not be a branch*/
      if ( is_branch[regalloc_int_code[i].op] )
         continue;
      /* transforming registers*/
      if ( regalloc_int_code[i].ra == r_old )
         regalloc_int_code[i].ra = r_new;
      if ( regalloc_int_code[i].rb == r_old )
         regalloc_int_code[i].rb = r_new;
      if ( regalloc_int_code[i].rc == r_old )
         regalloc_int_code[i].rc = r_new;
   }
   
   /* using BB's flag to track visits*/
   cfg->block[bb].flag = TRUE;
   
   /* going to next basic block*/
   for ( i = 0; i < cfg->block[bb].num_next; i++ )
      if ( cfg->block[bb].next_bb[i] != -1 && !(cfg->block[bb].flag) )
         trav_sub( r_old, r_new, cfg->block[cfg->block[bb].next_bb[i]].idx_start, cfg );
   
}

/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */

/** Function to prune graph in its matrix representation, filling the stack
  * Returns: - 0, for successful running;
             - 1, for failure;
  * Arguments:
  * * *stack: the vertex stack
  * * *stack_top: pointer to the top of the stack
  */
char prune_graph ( u4 *stack, u4 *stack_top ){
   register u4 i, j;
   boolean app;
   
   /* backup information*/
   for ( i = 0; i < ig_length + 2; i++ )
      ig[i].bkp_nn = ig[i].num_neighbors;
   
   /* search for nodes having fewer than R (NIOS_NUM_GPREG) neighbors*/
   i = OFFSET_VARS;
   while ( i < ig_length + 2 ){
      /* applying degree < R rule*/
      if ( ig[i].num_neighbors < NIOS_NUM_GPREG && !ig[i].in_stack ){
         /* remove from graph (matrix rep. - more efficient) and place onto stack*/
         for ( j = 0; j < ig_length + 2; j++ ){
            if ( inter[i][j] > 0 ){
               inter[i][j] = inter[j][i] = 0;
               (ig[j].num_neighbors)--;
            }
         }
         stack[*stack_top] = i;
         (*stack_top)++;
         ig[i].in_stack = TRUE;
         ig[i].spill_cost = 0;   /* differentiate from the ones which will have spill code around*/
         /* return to analyze previous vertices*/
         i = OFFSET_VARS;
      }else i++;
   }
   /* using optimistic heuristic, if applicable*/
   app = FALSE;
   for ( i = OFFSET_VARS; !app && i < ig_length + 2; i++ )
      if ( !ig[i].in_stack )
         app = TRUE;
   if ( app ){
      for ( i = OFFSET_VARS; i < ig_length + 2; i++ ){
         if ( !ig[i].in_stack ){
            stack[*stack_top] = i;
            (*stack_top)++;
            ig[i].in_stack = TRUE;
         }
      }
   }
   
   /* restoring adj. lists graph*/
   for ( i = 0; i < ig_length + 2; i++ )
      ig[i].num_neighbors = ig[i].bkp_nn;
   
   return 0;
}

/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */

/** Function for registers assignment according to the stack built, and also
    for generating spill code
  * Returns: - 0, for successful running;
             - 1, for failure;
  * Arguments:
  * * *stack: the vertex stack
  * * *stack_top: pointer to the top of the stack
  */
char assign_regs ( u4 *stack, u4 *stack_top ){
   register u4 i;
   u1 color;
   
   /* popping elements from stack and trying to assign a color*/
   for ( i = *stack_top; i > 0; i-- ){
      color = min_color( stack[i-1] );
      if ( color != 0 ){
         /* it was possible to find a color for vertex at the stack top*/
         modif_code( color, stack[i-1] );
         ig[stack[i-1]].color = color;
      }else{
         /* not possible to find a real register for this symbolic*/
         gen_spill_code( stack[i-1] );
      }
   }
   *stack_top = 0;
   
   return 0;
}

/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */

/** Function that returns the mininum color among a vertex neighbors
  * Returns: - the color identification;
             - 0, if no color could be found;
  * Arguments:
  * * v: the graph's node that represents a symbolic register
  */
u1 min_color ( u4 v ){
   boolean color[NIOS_NUM_GPREG];
   register u4 i;
   
   /* initialing colors vector*/
   color[0] = color[1] = TRUE;
   for ( i = 2; i < NIOS_NUM_GPREG; i++ )
      color[i] = FALSE;
   
   /* inspecting vertex neighbors*/
   for ( i = 0; i < ig[v].num_neighbors; i++ ){
      color[ig[ig[v].neighbor[i]].color] = TRUE;
   }
   
   /* finding and returning the mininum color*/
   for ( i = 2; i < NIOS_NUM_GPREG; i++ )
      if ( !color[i] )
         return (u1) i;
   
   return 0;
}

/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */

/** Function that modifies the intermediate code to consider new spillings and
    real registers
  * Returns: - 0, for success;
             - 1, for failure;
  * Arguments:
  * * color: the "color" of the real register to be assigned
  * * var: the graph's node that represents a symbolic register to be turned
           into real
  */
char modif_code ( u1 color, u4 var ){
   register u4 i;
      
   if ( color == 0 )
      return 1;
   
   ig[var].color = color;
   
   /* coloring (innital approach - verify the need for changes)*/
   for ( i = 0; i < ra_code_position; i++ ){
      if ( regalloc_int_code[i].ra == var ){
         regalloc_int_code[i].ra = color;
      }
      if ( regalloc_int_code[i].rb == var ){
         regalloc_int_code[i].rb = color;
      }
      if ( regalloc_int_code[i].rc == var ){
         regalloc_int_code[i].rc = color;
      }
   }
   
   return 0;
}

/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */

/** Function to generate spill code when necessary
  * Returns: - 0, for success;
             - 1, for failure;
  * Arguments:
  * * var: variable to be spilled to memory
  */
char gen_spill_code ( u4 var ){
   register u4 i;
   register u1 count_modif;
   TInstr instr_load, instr_store;  /*the load/store instructions*/
   
   /* marking this variable as spilled*/
   spilled_vars[var] = TRUE;
   
   /* associate the variable to a place in memory*/
   instr_load.type = ITYPE;
   instr_load.ra = DISP_REG_MEM;
   instr_load.rb = SPILL_REG;
   instr_load.rc = 0;
   instr_load.op = LDW_OP;
   instr_load.opx = ITYPE_OPX;
   instr_load.imm5 = instr_load.imm26 = 0x00;
   instr_load.imm16 = num_spill * 4;
   
   instr_store.type = ITYPE;
   instr_store.ra = DISP_REG_MEM;
   instr_store.rb = SPILL_REG;
   instr_store.rc = 0;
   instr_store.op = STW_OP;
   instr_store.opx = ITYPE_OPX;
   instr_store.imm5 = instr_load.imm26 = 0x00;
   instr_store.imm16 = num_spill * 4;
   
   num_spill++;
   
   /* for each ocurrence of 'var'*/
   for ( i = 0; i < ra_code_position; i++ ){
      count_modif = 0;
      
      /* whether we have a definition*/
      if ( regalloc_int_code[i].type == RTYPE && regalloc_int_code[i].rc == var )
         regalloc_int_code[i].rc = SPILL_REG, insert_instr( instr_store, i+1 ), count_modif++;
      if ( regalloc_int_code[i].type == ITYPE && regalloc_int_code[i].rb == var )
         regalloc_int_code[i].rb = SPILL_REG, insert_instr( instr_store, i+1 ), count_modif++;
      
      /* whether we have an use, or both*/
      if ( regalloc_int_code[i].type == RTYPE ){
         if ( regalloc_int_code[i].ra == var ){
            regalloc_int_code[i].ra = SPILL_REG;
            insert_instr( instr_load, i ), count_modif++;
         }
         if ( regalloc_int_code[i].rb == var ){
            regalloc_int_code[i].rb = SPILL_REG;
            insert_instr( instr_load, i ), count_modif++;
         }
      }
      if ( regalloc_int_code[i].type == ITYPE )
         if ( regalloc_int_code[i].ra == var )
            regalloc_int_code[i].ra = SPILL_REG, insert_instr( instr_load, i ), count_modif++;
      i += count_modif;
   }
      
   return 0;
}

/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */

/** Function to insert an instruction into intermediate code
  * Returns: - 0, for success;
             - 1, for failure;
  * Arguments:
  * * instr: the instruction that will be added
  * * pos: the position where this instruction must go
  */
char insert_instr ( TInstr instr, u4 pos ){
   register s4 i;
   
   /* moving instructions forward - be aware of memory overflow!*/
   for ( i = ra_code_position; i >= pos; i-- ){
      regalloc_int_code[i+1] = regalloc_int_code[i];
   }
   ra_code_position++;
   
   /* updating branch instructions addresses*/
   /* finding instructions that have address to instruction at pos or forward - positive*/
   for ( i = 0; i < pos; i++ ){
      if ( is_branch[regalloc_int_code[i].op] && regalloc_int_code[i].imm16 + i * 4 >= pos * 4 )
         regalloc_int_code[i].imm16 += 4;
   }
   /* finding instructions that have address to instruction before pos - negative*/
   for ( i = pos + 1; i < ra_code_position; i++ ){
      if ( is_branch[regalloc_int_code[i].op] && regalloc_int_code[i].imm16 + (i - 1) * 4 <= pos * 4 )
         regalloc_int_code[i].imm16 -= 4;
   }
   
   /* placing the new instruction*/
   regalloc_int_code[pos] = instr;
   
   return 0;
}

/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */

/** Function to build interference graph within DFS over CFG in a tree. It will
    build restrictions within the tree rooted by the basic block where
    instruction 'curr' belongs to
  * Returns: - 0, for success;
             - 1, for failure;
  * Arguments:
  * * curr: the current instruction
  * * dest: stop insert restrictions when find this instruction
  * * *cfg: the CFG to be analysed
  * * var: the variable that must interfere
  */
char instr_restrictions ( u4 curr, u4 dest, const TCFG *cfg, u4 var ){
   register u4 i;
   u4 bb;
   
   /* perfoming restrictions within one basic block*/
   bb = cfg->basic_block_of[curr];
   for ( i = curr; i <= cfg->block[bb].idx_end; i++ ){
      /* stop*/
      if ( i == dest )
         return 0;
      /* including restrictions*/
      if ( regalloc_int_code[i].type == RTYPE ){
         inter[var][regalloc_int_code[i].ra]++;
         inter[var][regalloc_int_code[i].rb]++;
         inter[var][regalloc_int_code[i].rc]++;
         inter[regalloc_int_code[i].ra][var]++;
         inter[regalloc_int_code[i].rb][var]++;
         inter[regalloc_int_code[i].rc][var]++;
         num_edges += 3;
      }else{
         inter[var][regalloc_int_code[i].ra]++;
         inter[var][regalloc_int_code[i].rb]++;
         inter[regalloc_int_code[i].ra][var]++;
         inter[regalloc_int_code[i].rb][var]++;
         num_edges += 2;
      }
   }
   
   /* using BB's flag to track visits*/
   cfg->block[bb].flag = TRUE;
   
   /* going to next basic block*/
   for ( i = 0; i < cfg->block[bb].num_next; i++ )
      if ( cfg->block[bb].next_bb[i] != -1 && !(cfg->block[bb].flag) )
         instr_restrictions( cfg->block[cfg->block[bb].next_bb[i]].idx_start, dest, cfg, var );
   
   /* returned without finding instruction 'dest' in the block*/
   return 1;
}

/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */

/** Function for qsort (stdlib.h) compare nodes to be sorted
  */
int cmp_spill_cost ( const void *par1, const void *par2 ){
   TInterNode *x, *y;
   x = (TInterNode*) par1;
   y = (TInterNode*) par2;
   
   if ( x->spill_cost > y->spill_cost ) return 1;
   if ( x->spill_cost < y->spill_cost ) return -1;
   return 0;
}

/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */

/** Function to return the intermediate code transformed
  * Returns: the set of intemediate instructions
  * Arguments:
  * * *code_length: the code's length to be returned by reference
  */
TInstr* regalloc_get_code ( u4 *code_length ){
   *code_length = ra_code_position;
   return regalloc_int_code;
}
