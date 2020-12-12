#ifndef FADEBABE_H
#define FADEBABE_H


#include "types.h"


enum {
    OP_NOP,
    OP_PUSHNIL,
    OP_PUSHTRUE,
    OP_PUSHFALSE,
    OP_PUSHINT,
    OP_PUSHSYM,
    OP_PUSHVAL,
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_MOD,
    OP_EQ,
    OP_LT,
    OP_GT,
    OP_NOT,
    OP_AND,
    OP_OR,
    OP_LOAD,
    OP_STORE,
    OP_STACKSIZE,
    OP_DUP,
    OP_DROP,
    OP_SWAP,
    OP_NEWOBJ,
    OP_GETPROP,
    OP_SETPROP,
    OP_EXEC,
    OP_CONDEXEC,
    OP_WHILE,
    N_OPS
};


typedef struct Object {
    u16 count;
} Object;


typedef struct Lambda {
    u16 code_size;
    u8 code[];
} Lambda;


#define V_NIL       0
#define V_BOOL      1
#define V_OBJECT    2
#define V_STRING    3
#define V_INT       4
#define V_LAMBDA    5
#define V_SYM       6
#define V_FORWARD   7
#define V_NATIVE    8
#define V_REF       9

#define V_MASK_TYPE 0x0F
#define V_MASK_MARK 0x80


typedef struct GCObj {
    u8 tag;
    struct GCObj *next;
    union {
        Object *_obj;
        char *_str;
        Lambda *_lam;
    };
} GCObj;


typedef void (*vmnative)(void);


typedef struct Value {
    u8 tag;
    union {
        GCObj gc;
        u16 _int;
        struct Value *fwd;
        vmnative native;
    };
} Value;


void fadebabe_run(void);


#endif