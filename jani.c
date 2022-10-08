/* File containing project's main function.
   Created: 23-apr-2007
   Last modified: 17-jul-2008
   Author: Willian dos Santos Lima*/

/* ========================================================================= */

/* including other files*/
#include "def.h"
#include "recog.h"
#include "bypass.h"
#include "graphgen.h"
#include "regalloc.h"
#include "codegen.h"

/* prototipe for the arguments processing function*/
void process_arg ( char *param );

/* ========================================================================= */
/* main program*/
int main ( int argc, char *argv[] ){
   char fname[U1_MAX+1], err;
   u1 argp;
   u4 t_inicio, t_fim;
   double t_decorrido;
   register u2 i;
   
   u4 tmp_u4;
   
   /* welcome message*/
   printf("======================================================================\n");
   printf("%s v%s initialized. Welcome!\n",__NAME,__VERSION);
   
   /* obtaining necessary arguments*/
   if ( argc < 2 || argv[1][0] == '-' ){
      printf("Please, inform .class file: ");
      scanf("%s",fname);
   }else strcpy( fname, argv[1] );
   
   /* setting default values*/
   strcpy( output_, "out.bin" );
   gravar_info_ = 1;    /* allow .info file generation*/
   
   /* if there is more than the necessary arguments, process the rest*/
   argp = 2;
   if ( argc >= 2 && argv[1][0] == '-' ) argp--;
   while ( argp < argc ){
      process_arg( argv[argp] );
      argp++;
   }
   
   /* begining compiling*/
   printf("\nCompiling \"%s\":\n",fname);
   err_ = warn_ = 0;
   
   t_inicio = clock( );
   
   /* step 1: recognize .class file*/
   err = recognize( fname );
   
   /* step 2: translating the main code from bytecodes to intermediate language*/
   if ( !err ){
      if ( recog_get_class_file( ) != NULL &&  recog_get_main_code( ) != NULL ){
         err = bypass( recog_get_class_file( ), recog_get_main_code( ) );
      }else{
         show_error( RECOGNIZER, 6 );   /* null pointer*/
         err_++;
         err = 1;
      }
   }
   
   /* step 3: building up the graphs*/
   if ( !err ){
      if ( bypass_get_code( &tmp_u4 ) != NULL )
         /* step 3.1: control flow graph*/
         err = cfg_gen( bypass_get_code( &tmp_u4 ), tmp_u4, TRUE );
         /* step 3.2: data flow graph*/
         if ( !err )
            err = dfg_gen( bypass_get_code( &tmp_u4 ), tmp_u4, cfg_get( ), TRUE );
         /* step 3.3: control dependence graph*/
         if ( !err )
            err = cdg_gen( cfg_get( ) );
         /* step 3.4: data dependence graph*/
         if ( !err )
            err = ddg_gen( bypass_get_code( &tmp_u4 ), tmp_u4 );
      else{
         show_error( BYPASSER, 5 );   /* null pointer*/
         err_++;
         err = 1;
      }
   }
   
   /* step 4: register allocation*/
   if ( !err ){
      if ( bypass_get_code( &tmp_u4 ) != NULL )
         err = reg_alloc( bypass_get_code( &tmp_u4 ), tmp_u4, cfg_get( ), dfg_get( ) );
      else{
         show_error( BYPASSER, 5 );   /* null pointer*/
         err_++;
         err = 1;
      }
   }
   
   /* step 5: generating binary code*/
   if ( !err ){
      if ( regalloc_get_code( &tmp_u4 ) != NULL )
         err = code_gen( regalloc_get_code( &tmp_u4 ), tmp_u4 );
      else{
         show_error( REGISTER_ALLOC, 5 );   /* null pointer*/
         err_++;
         err = 1;
      }
   }
   
   t_fim = clock( );
   t_decorrido = (double)( t_fim - t_inicio ) / CLOCKS_PER_SEC;   /*approached measure*/
   
   /* exit messages*/
   printf("\nProcess finished with %u error(s) and %u warning(s).\n",err_,warn_);
   printf("Time elapsed: %.1lf seconds.\n",t_decorrido);
   
   printf("\n%s finished.\n",__NAME);
   printf("\n----------------------------------------------------------------------\n");
   printf("By: %s\n",__AUTHOR);
   printf("    %s\n    %s\n",__GROUP,__LOCAL);
   printf("======================================================================\n");
   
   return 0;
}

/* ========================================================================= */
/* function for extra arguments processing (parameters)*/
void process_arg ( char *param ){
   register u4 i;
   
   if ( param[0] == '-' && param[1] == 'o' ){
      /* defining executable name*/
      for ( i = 2; i < strlen( param ); i++ )
         output_[i-2] = param[i];
      output_[i-2] = 0;
      return;
   }
   
   if ( strcmp( param, "-aopt" ) == 0 ){
      /* using all optimizations*/
      
      return;
   }
   
   if ( strcmp( param, "-af" ) == 0 ){
      /* generating all output files*/
      
      return;
   }
   
   if ( strcmp( param, "-nf" ) == 0 ){
      /* generating no output files besides the executable one*/
      
      return;
   }
   
   if ( strcmp( param, "-all" ) == 0 ){
      /* using all optimizations and generating all output files*/
      
      return;
   }
   
   if ( strcmp( param, "-fast" ) == 0 ){
      /* using fast compiling, without optimization and output files generation*/
      
      return;
   }
   
   /* if it's here, no parameter was identified*/
   printf("CONF.: Unable to identify parameter \"%s\".\n",param);
}
