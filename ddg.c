/* File with implementations for the data dependence graph manager.
   Created: 10-mar-2008
   Last modified: 12-mar-2008
   Author: Willian dos Santos Lima*/

/* ========================================================================= */

#include "graphgen.h"

/* ------------------------------------------------------------------------- */
/* DDG global variables declaration*/

/* the so-called Data Dependence Graph --------------------------------------*/
TDDG ddg;

/* ------------------------------------------------------------------------- */

/* MUST MAY AN SPECIAL CONDERATION ABOUT BRANCH AND COMPARISION INSTRUCTIONS,
   BECAUSE THEY USUALLY DON'T WRITE TO REGISTERS AND THIS FACT MAY REDUCE
   DDG STRUCTURE*/

/** Function to create the data dependence graph. For each pair of instructions,
    call functions that verify input, output and anti-dependence dependencies.
  * Returns: - 0, for successful execution;
             - 1, for failure;
  ** Arguments:
  * * *int_code: the intermediate code as a set of instructions
  * * code_length: number of instructions in the intermediate code
  */
char ddg_gen ( const TInstr *int_code, u4 code_length ){
   register u4 i, j, counter;
   register u1 def1 = 0, def2 = 0;
   char r;  /* 0, no dep.; 1, output; 2, input; 3, anti-dependence*/
   u4 *ddg_tmp_dest, last_ddg_dest;
   TDependenceType *ddg_tmp_dt;
   FILE* _tmp_file;
   
   /* each vertex in the DDG will be an index for an intermediate instruction*/
   ddg.instruction = malloc( code_length * sizeof( u4 ) );
   if ( ddg.instruction == NULL ){
      err_++;
      if ( show_error( GRAPH_GEN, 4 ) )
         return 1;
   }
   
   /* each vertex could have a number of neighbors (indices for int. instructions)*/
   ddg.instr_neighbor = malloc( code_length * sizeof( u4* ) );
   ddg.dt = malloc( code_length * sizeof( TDependenceType* ) );
   ddg.num_neighbors = malloc( code_length * sizeof( u4 ) );
   if ( ddg.instr_neighbor == NULL || ddg.dt == NULL || ddg.num_neighbors == NULL ){
      err_++;
      if ( show_error( GRAPH_GEN, 4 ) )
         return 1;
   }
   
   /* a temporary list of neighbors and type of dependence*/
   ddg_tmp_dest = malloc( code_length * sizeof( u4 ) );
   ddg_tmp_dt = malloc( code_length * sizeof( TDependenceType ) );
   if ( ddg_tmp_dest == NULL || ddg_tmp_dt == NULL ){
      err_++;
      if ( show_error( GRAPH_GEN, 4 ) )   /*REVIEW*/
         return 1;
   }
   last_ddg_dest = 0;
   
   /* there's exactly one vertex for each int. instruction*/
   ddg.length = code_length;
   for ( i = 0; i < ddg.length; i++ )
      ddg.instruction[i] = i, ddg.num_neighbors[i] = 0;
   
   /* for each pair of instructions...*/
   for ( i = 0; i < ddg.length /*- 1*/; i++ ){
      last_ddg_dest = 0;
      for ( j = 0; j < ddg.length; j++ ){
         /* verifying definitions*/
         if ( int_code[i].type == RTYPE && int_code[j].type == RTYPE )
            def1 = is_rdefine[int_code[i].opx], def2 = is_rdefine[int_code[j].opx];
         else if ( int_code[i].type == RTYPE && int_code[j].type == ITYPE )
            def1 = is_rdefine[int_code[i].opx], def2 = is_idefine[int_code[j].op];
         else if ( int_code[i].type == ITYPE && int_code[j].type == RTYPE )
            def1 = is_idefine[int_code[i].op], def2 = is_rdefine[int_code[j].opx];
         else def1 = is_idefine[int_code[i].op], def2 = is_idefine[int_code[j].op];
         /* no definition implies no data dependence*/
         if ( !def1 && !def2 )
            continue;
         /* both instructions are definitions: check for all dependencies*/
         r = ddg_check_output( &int_code[i], def1, &int_code[j], def2 );
         if ( r != 1 ) r = ddg_check_input( &int_code[i], def1, &int_code[j], def2 ) + 10;
         if ( r != 1 && r != 11 ) r = ddg_check_anti( &int_code[i], def1, &int_code[j], def2 ) + 100;
         /* preparing for edge insertion in DDG*/
         if ( r == 1 || r == 11 || r == 101 ){
            ddg_tmp_dest[last_ddg_dest] = j;
            switch ( r ){
               case 1   : ddg_tmp_dt[last_ddg_dest] = OUTPUT_DEP;
                          break;
               case 11  : ddg_tmp_dt[last_ddg_dest] = INPUT_DEP;
                          break;
               case 101 : ddg_tmp_dt[last_ddg_dest] = ANTI_DEP;
                          break;
            }
            last_ddg_dest++;
         }
      }
      /*inserting neighbors of instruction 'i'*/
      ddg.instr_neighbor[i] = malloc( last_ddg_dest * sizeof( u4 ) );
      ddg.dt[i] = malloc( last_ddg_dest * sizeof( TDependenceType ) );
      ddg.num_neighbors[i] = last_ddg_dest;
      for ( j = 0; j < last_ddg_dest; j++ ){
         ddg.instr_neighbor[i][j] = ddg_tmp_dest[j];
         ddg.dt[i][j] = ddg_tmp_dt[j];
      }
   }
   
   free( ddg_tmp_dest );
   
   /* writing to information data file*/
   /*i*/ counter = 0;
   /*i*/ _tmp_file = fopen( "output/ddg.dat", "w" );
   /*i*/ for ( i = 0; i < ddg.length; i++ ){
   /*i*/    fprintf(_tmp_file, "Instr. %03d:", ddg.instruction[i]);
   /*i*/    for ( j = 0; j < ddg.num_neighbors[ddg.instruction[i]]; j++ ){
   /*i*/       fprintf(_tmp_file, "  %d(%d)", ddg.instr_neighbor[i][j], ddg.dt[i][j]);
   /*i*/       counter++;
   /*i*/    }
   /*i*/    fprintf(_tmp_file, "\n");
   /*i*/ }
   /*i*/ fprintf(_tmp_file, "# of edges: %d\n", counter);
   /*i*/ fclose( _tmp_file );
   
   return 0;
}

/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */

/** Function that verifies whether an input dependence exists.
  * Returns: - 0, no data dependence;
             - 1, input data dependence;
  * Arguments:
  * * *instr1: pointer to one instruction
  * * *instr2: pointer to the another instruction
  */
char ddg_check_input ( const TInstr *instr1, u1 def1, const TInstr *instr2, u1 def2 ){
   u4 v1;
   
   if ( !def1 )
      return 0;
   
   /* get the variable that is defined in instruction 1...*/
   if ( instr1->type == RTYPE )
      v1 = instr1->rc;
   else v1 = instr1->rb;
   if ( v1 == REG0 )
      return 0;
   
   /* ...and search for this one at instruction 2*/
   if ( instr2->type == RTYPE ){
      if ( instr2->ra == v1 ) return 1;
      if ( instr2->rb == v1 ) return 1;
      if ( !def2 && instr2->rc == v1 ) return 1;
   }else{
      if ( instr2->ra == v1 ) return 1;
      if ( !def2 && instr2->rb == v1 ) return 1;
   }
   
   return 0;
}

/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */

/** Function that verifies whether an output dependence exists.
  * Returns: - 0, no output data dependence;
             - 1, output data dependence;
  * Arguments:
  * * *instr1: pointer to one instruction
  * * *instr2: pointer to the another instruction
  */
char ddg_check_output ( const TInstr *instr1, u1 def1, const TInstr *instr2, u1 def2 ){
   u4 v1, v2;
   
   if ( !def1 || !def2 )
      return 0;
   
   if ( instr1->type == RTYPE )
      v1 = instr1->rc;
   else v1 = instr1->rb;
   
   if ( instr2->type == RTYPE )
      v2 = instr2->rc;
   else v2 = instr2->rb;
   
   if ( v1 == v2 )
      return 1;
   
   return 0;
}

/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */

/** Function that verifies whether an output dependence exists.
  * Returns: - 0, no anti-dependence data dependence;
             - 1, anti-dependence data dependence;
  * Arguments:
  * * *instr1: pointer to one instruction
  * * *instr2: pointer to the another instruction
  */
char ddg_check_anti ( const TInstr *instr1, u1 def1, const TInstr *instr2, u1 def2 ){
   u4 v2;
   
   if ( !def2 )
      return 0;
   
   /* get the variable that is defined in instruction 2...*/
   if ( instr2->type == RTYPE )
      v2 = instr2->rc;
   else v2 = instr2->rb;
   if ( v2 == REG0 )
      return 0;
   
   /* ...and search for this one at instruction 1*/
   if ( instr1->type == RTYPE ){
      if ( instr1->ra == v2 ) return 1;
      if ( instr1->rb == v2 ) return 1;
      if ( !def1 && instr1->rc == v2 ) return 1;
   }else{
      if ( instr1->ra == v2 ) return 1;
      if ( !def1 && instr1->rb == v2 ) return 1;
   }
   
   return 0;
}
