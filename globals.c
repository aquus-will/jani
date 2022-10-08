#include "def.h"

/* global variables' declaration*/
u2 err_, warn_;           /* errors and warnings counters*/
char output_[U1_MAX + 1]; /* output file name (executable)*/
u1 gravar_info_;          /* whether informations of ClassFile must be available*/

/* whether the I-TYPE instruction is a branch one*/
const char is_branch[64] = {1, 0, 0, 0, 0, 0, 1, 0,  /*0x00 - 0x07*/
                            0, 0, 0, 0, 0, 0, 1, 0,  /*0x08 - 0x0F*/
                            0, 0, 0, 0, 0, 0, 1, 0,  /*0x10 - 0x17*/
                            0, 0, 0, 0, 0, 0, 1, 0,  /*0x18 - 0x1F*/
                            0, 0, 0, 0, 0, 0, 1, 0,  /*0x20 - 0x27*/
                            0, 0, 0, 0, 0, 0, 1, 0,  /*0x28 - Ox2F*/
                            0, 0, 0, 0, 0, 0, 1, 0,  /*0x30 - 0x37*/
                            0, 0, 0, 0, 0, 0, 0, 0}; /*0x38 - 0x3F*/

/* whether the I-TYPE instruction defines a value*/
const char is_idefine[64] = {0, 0, 0, 1, 1, 0, 0, 1,  /*0x00 - 0x07*/
                             1, 0, 0, 1, 1, 0, 0, 1,  /*0x08 - 0x0F*/
                             1, 0, 0, 0, 1, 0, 0, 1,  /*0x10 - 0x17*/
                             1, 0, 0, 0, 1, 0, 0, 0,  /*0x18 - 0x1F*/
                             1, 0, 0, 1, 1, 0, 0, 1,  /*0x20 - 0x27*/
                             1, 0, 0, 1, 1, 0, 0, 1,  /*0x28 - Ox2F*/
                             1, 0, 0, 0, 1, 0, 0, 1,  /*0x30 - 0x37*/
                             0, 0, 0, 0, 1, 0, 0, 0}; /*0x38 - 0x3F*/

/* whether the R-TYPE instruction defines a value*/
const char is_rdefine[64] = {0, 0, 1, 1, 0, 0, 1, 1,  /*0x00 - 0x07*/
                             1, 0, 0, 1, 0, 0, 1, 0,  /*0x08 - 0x0F*/
                             1, 0, 1, 1, 0, 0, 1, 1,  /*0x10 - 0x17*/
                             1, 0, 1, 1, 0, 0, 1, 1,  /*0x18 - 0x1F*/
                             1, 0, 0, 0, 1, 1, 1, 1,  /*0x20 - 0x27*/
                             1, 0, 0, 1, 0, 0, 0, 0,  /*0x28 - Ox2F*/
                             1, 1, 0, 0, 0, 0, 0, 0,  /*0x30 - 0x37*/
                             0, 1, 1, 0, 0, 0, 0, 0}; /*0x38 - 0x3F*/

/* from OPCODE to NAME ......................................................*/
/* mapping 'op' and 'opx' from 0x00 to 0x3F:*/
const char *op_name[64] = {"CALL", "", "", "LDBU", "ADDI", "STB", "BR", "LDB",             /*0x00 - 0x07*/
                           "CMPGEI", "", "", "LDHU", "ANDI", "STH", "BGE", "LDH",          /*0x08 - 0x0F*/
                           "CMPLTI", "", "", "", "ORI", "STW", "BLT", "LDW",               /*0x10 - 0x17*/
                           "CMPNEI", "", "", "FLUSHDA", "XORI", "", "BNE", "",             /*0x18 - 0x1F*/
                           "CMPEQI", "", "", "LDBUIO", "MULI", "STBIO", "BEQ", "LDBIO",    /*0x20 - 0x27*/
                           "CMPGEUI", "", "", "LDHUIO", "ANDHI", "STHIO", "BGEU", "LDHIO", /*0x28 - Ox2F*/
                           "CMPLTUI", "", "", "INITD", "ORHI", "STWIO", "BLTU", "LDWIO",   /*0x30 - 0x37*/
                           "", "", "", "FLUSHD", "XORHI", "", "", ""};                     /*0x38 - 0x3F*/

const char *opx_name[64] = {"", "ERET", "ROLI", "ROL", "FLUSHP", "RET", "NOR", "MULXUU",    /*0x00 - 0x07*/
                            "CMPGE", "BRET", "", "ROR", "FLUSHI", "JMP", "AND", "",         /*0x08 - 0x0F*/
                            "CMPLT", "", "SLLI", "SLL", "", "", "OR", "MULXSU",             /*0x10 - 0x17*/
                            "CMPNE", "", "SRLI", "SRL", "NEXTPC", "CALLR", "XOR", "MULXSS", /*0x18 - 0x1F*/
                            "CMPEQ", "", "", "", "DIVU", "DIV", "RDCTL", "MUL",             /*0x20 - 0x27*/
                            "CMPGEU", "INITI", "", "SRA", "", "TRAP", "WRCTL", "",          /*0x28 - Ox2F*/
                            "CMPLTU", "ADD", "", "", "BREAK", "", "SYNC", "",               /*0x30 - 0x37*/
                            "", "SUB", "SRAI", "", "", "", "", ""};                         /*0x38 - 0x3F*/

/* list of bytecodes that are implemented here*/
const char is_implemented[256] = {
   1, 1, 1, 1, 1,
   1, 1, 1, 1, 0,
   0, 0, 0, 0, 0,
   0, 1, 1, 0, 0,
   0, 1, 0, 0, 0,
   1, 1, 1, 1, 1,
   0, 0, 0, 0, 0,
   0, 0, 0, 0, 0,
   0, 0, 0, 0, 0,
   0, 1, 0, 0, 0,
   0, 0, 0, 0, 1,
   0, 0, 0, 1, 1,
   1, 1, 1, 0, 0,
   0, 0, 0, 0, 0,
   0, 0, 0, 0, 0,
   0, 0, 0, 0, 1,
   0, 0, 0, 0, 0,
   0, 0, 1, 1, 1,
   0, 0, 1, 0, 0,
   1, 1, 0, 0, 0,
   1, 0, 0, 0, 1,
   0, 0, 0, 1, 0,
   0, 0, 1, 0, 0,
   0, 1, 0, 0, 0,
   1, 0, 1, 0, 0,
   0, 1, 0, 1, 0,
   1, 0, 1, 0, 0,
   0, 0, 0, 0, 0,
   0, 0, 0, 0, 0,
   0, 0, 1, 0, 0,    /*147 (0x93) -> INT2SHORT!!*/
   0, 0, 0, 1, 1,
   1, 1, 1, 1, 1,
   1, 1, 1, 1, 1,
   0, 0, 1, 0, 0,
   0, 0, 0, 0, 0,
   0, 0, 1, 0, 0,
   0, 0, 0, 0, 0,
   0, 0, 0, 1, 0,
   0, 0, 0, 0, 0,
   0, 0, 0, 0, 0,
   0, 0, 0, 0, 0,
   0, 0, 0, 0, 0,
   0, 0, 0, 0, 0,
   0, 0, 0, 0, 0,
   0, 0, 0, 0, 0,
   0, 0, 0, 0, 0,
   0, 0, 0, 0, 0,
   0, 0, 0, 0, 0,
   0, 0, 0, 0, 0,
   0, 0, 0, 0, 0
};

/* bytecodes lengths*/
const int bytecode_length[256] = {
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,	/*   0 */
	2, 3, 2, 3, 3, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, /*  16 */
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /*  32 */
	1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, /*  48 */
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /*  64 */
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /*  80 */
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /*  96 */
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* 112 */
	1, 1, 1, 1, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* 128 */
	1, 1, 1, 1, 1, 1, 1, 1, 1, 3, 3, 3, 3, 3, 3, 3, /* 144 */
	3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 0, 0, 1, 1, 1, 1, /* 160 - attention to 0xAA e 0xAB*/
	1, 1, 3, 3, 3, 3, 3, 3, 3, 5, 1, 3, 2, 3, 1, 1, /* 176 */
	3, 3, 1, 1, 1, 4, 3, 3, 5, 5, 1, 1, 1, 1, 1, 1, /* 192 */
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 3, /* 208 */
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, /* 224 */
	3, 3, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 3, 3	/* 240 */
};

/* from OPCODE to NAME ......................................................*/
const char *bytecode_name[256] = {
    "NOP", "ACONST_NULL", "ICONST_M1", "ICONST_0", "ICONST_1",
    "ICONST_2", "ICONST_3", "ICONST_4", "ICONST_5", "LCONST_0",
    "LCONST_1", "FCONST_0", "FCONST_1", "FCONST_2", "DCONST_0",
    "DCONST_1", "BIPUSH", "SIPUSH", "LDC1", "LDC2",
    "LDC2W", "ILOAD", "LLOAD", "FLOAD", "DLOAD",
    "ALOAD", "ILOAD_0", "ILOAD_1", "ILOAD_2", "ILOAD_3",
    "LLOAD_0", "LLOAD_1", "LLOAD_2", "LLOAD_3", "FLOAD_0",
    "FLOAD_1", "FLOAD_2", "FLOAD_3", "DLOAD_0", "DLOAD_1",
    "DLOAD_2", "DLOAD_3", "ALOAD_0", "ALOAD_1", "ALOAD_2",
    "ALOAD_3", "IALOAD", "LALOAD", "FALOAD", "DALOAD",
    "AALOAD", "BALOAD", "CALOAD", "SALOAD", "ISTORE",
    "LSTORE", "FSTORE", "DSTORE", "ASTORE", "ISTORE_0",
    "ISTORE_1", "ISTORE_2", "ISTORE_3", "LSTORE_0", "LSTORE_1",
    "LSTORE_2", "LSTORE_3", "FSTORE_0", "FSTORE_1", "FSTORE_2",
    "FSTORE_3", "DSTORE_0", "DSTORE_1", "DSTORE_2", "DSTORE_3",
    "ASTORE_0", "ASTORE_1", "ASTORE_2", "ASTORE_3", "IASTORE",
    "LASTORE", "FASTORE", "DASTORE", "AASTORE", "BASTORE",
    "CASTORE", "SASTORE", "POP", "POP2", "DUP",
    "DUP_X1", "DUP_X2", "DUP2", "DUP2_X1", "DUP2_X2",
    "SWAP", "IADD", "LADD", "FADD", "DADD",
    "ISUB", "LSUB", "FSUB", "DSUB", "IMUL",
    "LMUL", "FMUL", "DMUL", "IDIV", "LDIV",
    "FDIV", "DDIV", "IREM", "LREM", "FREM",
    "DREM", "INEG", "LNEG", "FNEG", "DNEG",
    "ISHL", "LSHL", "ISHR", "LSHR", "IUSHR",
    "LUSHR", "IAND", "LAND", "IOR", "LOR",
    "IXOR", "LXOR", "IINC", "I2L", "I2F",
    "I2D", "L2I", "L2F", "L2D", "F2I",
    "F2L", "F2D", "D2I", "D2L", "D2F",
    "INT2BYTE", "INT2CHAR", "INT2SHORT", "LCMP", "FCMPL",
    "FCMPG", "DCMPL", "DCMPG", "IFEQ", "IFNE",
    "IFLT", "IFGE", "IFGT", "IFLE", "IF_ICMPEQ",
    "IF_ICMPNE", "IF_ICMPLT", "IF_ICMPGE", "IF_ICMPGT", "IF_ICMPLE",
    "IF_ACMPEQ", "IF_ACMPNE", "GOTO", "JSR", "RET",
    "TABLESWITCH", "LOOKUPSWITCH", "IRETURN", "LRETURN", "FRETURN",
    "DRETURN", "ARETURN", "RETURN", "GETSTATIC", "PUTSTATIC",
    "GETFIELD", "PUTFIELD", "INVOKEVIRTUAL", "INVOKESPECIAL", "INVOKESTATIC",
    "INVOKEINTERFACE", "", "NEW", "NEWARRAY", "ANEWARRAY",
    "ARRAYLENGTH", "ATHROW", "CHECKCAST", "INSTANCEOF", "MONITORENTER",
    "MONITOREXIT", "WIDE", "MULTIANEWARRAY", "IFNULL", "IFNONNULL",
    "GOTO_W", "JSR_W", "BREAKPOINT", "", ""
};

/* ------------------------------------------------------------------------- */
/* graph generator variables*/

/* list for labelling target instructions from branches - must be initialized
   at CFG manager*/
s4 *from_instr;
