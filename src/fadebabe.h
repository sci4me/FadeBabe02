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
    OP_NEWARR,
    OP_GET,
    OP_SET,
    OP_EXEC,
    OP_CONDEXEC,
    OP_WHILE,
    N_OPS
};


typedef struct Object {
    u16 count;
} Object;


typedef struct Array {
    u16 count;
    u16 size;
    struct Value data[];
} Array;


typedef struct Lambda {
    u16 code_size;
    u8 code[];
} Lambda;


#define V_NIL       0
#define V_BOOL      1
#define V_OBJECT    2
#define V_ARRAY     3
#define V_STRING    4
#define V_INT       5
#define V_LAMBDA    6
#define V_SYM       7
#define V_FORWARD   8
#define V_NATIVE    9
#define V_REF       10

#define V_MASK_TYPE 0x0F
#define V_MASK_MARK 0x80


#define v2a(v) (v->gc._arr)


#define ARR_DEFAULT_SIZE 4


typedef struct GCObj {
    struct GCObj *next;
    union {
        Object *_obj;
        Array *_arr;
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