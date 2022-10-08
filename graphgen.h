/* Header file for graph generator.
   Created: 20-feb-2008
   Last modified: 27-dec-2008
   Author: Willian dos Santos Lima*/

/* ========================================================================= */
#ifndef GRAPHGEN
#define GRAPHGEN

/* ------------------------------------------------------------------------- */

#include "def.h"
#include "error.h"

/* ------------------------------------------------------------------------- */
/* information for a basic block*/
typedef struct BASICBLOCK{
   u4 idx_start;     /* address for instruction starting basic block*/
   u4 idx_end;       /* address for instruction ending basic block*/
   s4 *next_bb;      /* basic blocks that can be reached from this one*/
   u1 num_next;      /* number of basic blocks that can be reached*/
   boolean flag;     /* a flag to be used within a DFS-search or something else*/
   u4 parent;        /* the parent basic block of this one*/
}TBasicBlock;

/* information for a dependence type*/
typedef enum { INPUT_DEP, OUTPUT_DEP, ANTI_DEP } TDependenceType;

/* GRAPHS: all represented as adjacents list ------------------------------ */
/* information for a Control Flow Graph*/
typedef struct CFG{
   TBasicBlock *block;
   u4 length;
   s4 *basic_block_of;
}TCFG;

/* information for a Control Dependence Graph*/
typedef struct CDG{
   TBasicBlock *block;
   u4 length;
}TCDG;

/* information for a Data Flow Graph*/
typedef struct DFG{
   u4 *instruction;
   u4 *num_neighbors;    /* the number of neighbors to an instructon*/
   u4 **instr_neighbor;  /* for each instruction in the graph, its neighbors instructions*/
   u4 length;
}TDFG;

/* information for a Data Dependence Graph*/
typedef struct DDG{
   u4 *instruction;
   u4 *num_neighbors;    /* the number of neighbors to an instructon*/
   u4 **instr_neighbor;  /* for each instruction in the graph, its neighbors instructions*/
   TDependenceType **dt; /* for each instruction in the graph, the dependece type associate with its neighbor*/
   u4 length;
}TDDG;

/* ------------------------------------------------------------------------- */
/* graph generator variables*/

/* list for labelling target instructions from branches - must be initialized
   at CFG manager*/
extern s4 *from_instr;

/* ------------------------------------------------------------------------- */
/* graphs managers*/
#include "cfg.h"
#include "dfg.h"
#include "cdg.h"
#include "ddg.h"

/* ========================================================================= */
/* error codes for this phase:
   - 1, (CFG) unable to allocate memory for CFG
   - 2, (CFG) error when getting basic blocks
   - 3, (DFG) unable to allocate memory for DFG

/* ========================================================================= */

#endif
