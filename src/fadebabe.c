#include "navi65.h"
#include "leds.h"
#include "switches.h"
#include "acia.h"
#include "io.h"
#include "fadebabe.h"
#include "fbsrc.h"


u8* memset(u8 *p, u8 c, u16 n);
u8* memcpy(u8 *dst, u8 *src, u16 n);
u8 memcmp(u8 *a, u8 *b, u16 n);
u16 strlen(char *s);


static u8 is_whitespace(char c) {
    return c == ' ' || c == '\r' || c == '\t' || c == '\n';
}

static u8 is_digit(char c) {
    return c >= '0' && c <= '9';
}

static u8 is_alpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

static u8 is_hex(char c) {
    return (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}


typedef struct Intern {
    u8 len;
    char str[];
} Intern;

static u8 intern_data[0x200];

// TODO: overflow
static char *intern(char *s, u8 n) {
    Intern *x = (Intern*)intern_data;
    
    while(x->len) {
        if(n == x->len) {
            if(memcmp(s, x->str, n) == 0)
                return x->str;
        }

        x = (Intern*)((u8*)x + sizeof(Intern) + x->len + 1);
    }

    x->len = n;
    memcpy(x->str, s, n);
    x->str[n] = 0;

    return x->str;
}

static u8 symidx(char *s, u8 *r) {
    Intern *x = (Intern*)intern_data;
    u8 i = 0;

    while(x->len) {
        if(s == x->str) {
            *r = i;
            return 1;
        }

        ++i;
        x = (Intern*)((u8*)x + sizeof(Intern) + x->len + 1);
    }

    return 0;
}


static u8 comp_buf[0x400];
static u16 comp_buf_next = 0;


#define MEM_END         0x7F00 // beginning of MMIO page

#define HEAP_FROM_START 0x5000
#define HEAP_TO_START   0x6780

static u8 *heap = (u8*)HEAP_FROM_START;
static GCObj *heap_last = 0;

static u8* gc_alloc(u16 n) {
    // TODO: check if out of memory and do gc
    u8 *x = heap;
    heap += n;
    return x;
}


static Value globals[0x40];
static Value stack[0x40];
static u8 sp = 0;


static Value* resolve(Value *v) {
    if(v->tag == V_REF || v->tag == V_FORWARD) {
        return resolve(v->fwd);
    } else {
        return v;
    }
}


static void __print(void) {
    Value *v = &stack[--sp];
    
    if(v->tag == V_REF) {
        v = v->fwd;
    }

    switch(v->tag) {
        case V_NIL:
            acia_puts("nil");
            break;
        case V_BOOL:
            acia_puts(v->_int ? "true" : "false");
            break;
        case V_OBJECT:
            acia_puts("<object>");
            break;
        case V_ARRAY:
            acia_puts("<array>");
            break;
        case V_STRING:
            acia_puts(v->gc._str);
            break;
        case V_INT: {
            u8 y;
            char buf[6];
            u8 z = 0;
            u16 x = v->_int;
            while(x > 10) {
                y = x % 10;
                x /= 10;
                buf[z] = (y + '0');
                ++z;
            }
            acia_putc((x % 10) + '0');
            while(z--) {
                acia_putc(buf[z]);
            }            
            break;
        }
        case V_LAMBDA:
            acia_puts("<lambda>");
            break;
        case V_SYM:
            acia_puts("<sym>");
            break;
        case V_FORWARD:
            // TODO
            break;
        case V_NATIVE:
            acia_puts("<native>");
            break;
        default:
            acia_put_hex_byte(v->tag);
            panic(0x99);
            break;
    }
}

static void __println(void) {
    __print();
    acia_putc('\n');
}

static void __printhex(void) {
    --sp;
    if(stack[sp].tag != V_INT) panic(0xD3);
    put_hex_word(stack[sp]._int);
}

static void __putchar(void) {
    --sp;
    if(stack[sp].tag != V_INT) panic(0x59);
    acia_putc((u8)(stack[sp]._int & 0xFF));
}

static void __getchar(void) {
    stack[sp].tag = V_INT;
    stack[sp]._int = acia_getc();
    ++sp;
}

static void __setleds(void) {
    --sp;
    if(stack[sp].tag != V_INT) panic(0x5C);
    leds_set_value((u8)(stack[sp]._int & 0xFF));
}

static void __delayms(void) {
    --sp;
    if(stack[sp].tag != V_INT) panic(0x5D);
    delay_ms((u8)(stack[sp]._int & 0xFF));
}

static void __switch1_read(void) {
    stack[sp].tag = V_INT;
    stack[sp]._int = switch1_read();
    ++sp;
}

static void __switch2_read(void) {
    stack[sp].tag = V_INT;
    stack[sp]._int = switch2_read();
    ++sp;
}

static void __apnd(void) {
    Value *av;
    Array *a;
    Value *v;
    u16 nsize;
    --sp;
    av = resolve(&stack[sp]);
    if(av->tag == V_ARRAY) {
        a = av->gc._arr;
        if(a->count == a->size) {
            nsize = a->size * 2;
            v = (Value*)gc_alloc(sizeof(Value) + sizeof(Value) * nsize);
            v->tag = V_ARRAY;
            v->gc.next = heap_last;
            heap_last = &v->gc;
            v->gc._arr = (Array*)(((u8*)v) + sizeof(Value));
            v->gc._arr->count = a->count;
            v->gc._arr->size = nsize;
            memcpy(v->gc._arr->data, a->data, sizeof(Value) * a->size);
            stack[sp].fwd->tag = V_FORWARD;
            stack[sp].fwd->fwd = v;
            a = v->gc._arr;
        }

        memcpy(&a->data[a->count++], &stack[sp-1], sizeof(Value));
    } else {
        panic(0x3F);
    }
    --sp;
}

static void __cnt(void) {
    Value *a = resolve(&stack[sp-1]);
    if(a->tag == V_ARRAY) {
        stack[sp-1].tag = V_INT;
        stack[sp-1]._int = v2a(a)->count;
    } else {
        panic(0x3E);
    }
}

static void init_natives(void) {
    #define addnative(name) do { intern(#name, sizeof(#name)-1); globals[i].tag = V_NATIVE; globals[i].native = __##name; ++i; } while(0)
    u8 i = 0;

    addnative(print);
    addnative(println);
    addnative(printhex);
    addnative(putchar);
    addnative(getchar);
    addnative(setleds);
    addnative(delayms);
    addnative(switch1_read);
    addnative(switch2_read);
    addnative(apnd);
    addnative(cnt);

    #undef addnative
}


static void interpret(Value *fn) {    
    #define TRACE 0

    u8 op;
    u8 i;
    u16 j;
    char *s;
    Value v;
    Value *vp1;
    Value *vp2;
    Array *a;
    u16 pc = 0;
    u16 code_size = fn->gc._lam->code_size;
    u8 *code = fn->gc._lam->code;
    
    static const void* const dispatch_table[] = {
        &&T_NOP,
        &&T_PUSHNIL,
        &&T_PUSHTRUE,
        &&T_PUSHFALSE,
        &&T_PUSHINT,
        &&T_PUSHSYM,
        &&T_PUSHVAL,
        &&T_ADD,
        &&T_SUB,
        &&T_MUL,
        &&T_DIV,
        &&T_MOD,
        &&T_EQ,
        &&T_LT,
        &&T_GT,
        &&T_NOT,
        &&T_AND,
        &&T_OR,
        &&T_LOAD,
        &&T_STORE,
        &&T_STACKSIZE,
        &&T_DUP,
        &&T_DROP,
        &&T_SWAP,
        &&T_NEWOBJ,
        &&T_NEWARR,
        &&T_GET,
        &&T_SET,
        &&T_EXEC,
        &&T_CONDEXEC,
        &&T_WHILE
    };

    #if TRACE
    put_hex_word(code_size);
    acia_putc('\n');
    #endif

    #define fetch8() code[pc++]
    #define fetch16() (((u16)fetch8() << 8) | ((u16)fetch8()))

    if(fn->tag != V_LAMBDA) panic(0x31);

    // TODO: check if GC moved the lambda! or perhaps 
    // have a flag the interpreter polls and clears? meh

dispatch:
    if(pc >= code_size) goto _done;
    #if TRACE
    put_hex_word(pc);
    acia_puts(": ");
    #endif
    op = fetch8();
    if(op >= N_OPS) {
        put_hex_word(pc);
        acia_puts(": ");
        acia_put_hex_byte(op);
        acia_putc('\n');
        panic(0xF6);
    }
    #if TRACE
    acia_put_hex_byte(op);
    acia_putc('\n');
    #endif
    goto *dispatch_table[op];

    #define vmcase(op) T_##op:
    #define vmbreak goto dispatch

    vmcase(NOP) {
        vmbreak;
    }
    vmcase(PUSHNIL) {
        stack[sp].tag = V_NIL;
        ++sp;
        vmbreak;
    }
    vmcase(PUSHTRUE) {
        stack[sp].tag = V_BOOL;
        stack[sp]._int = 1;
        ++sp;
        vmbreak;
    }
    vmcase(PUSHFALSE) {
        stack[sp].tag = V_BOOL;
        stack[sp]._int = 0;
        ++sp;
        vmbreak;
    }
    vmcase(PUSHINT) {
        stack[sp].tag = V_INT;
        stack[sp]._int = fetch16();
        ++sp;
        vmbreak;
    }
    vmcase(PUSHSYM) {
        s = (char*)fetch16();
        if(symidx(s, &i)) {
            stack[sp].tag = V_SYM;
            stack[sp]._int = i;
            ++sp;
        } else {
            goto T_PUSHNIL;
        }
        vmbreak;
    }
    vmcase(PUSHVAL) {
        stack[sp].tag = V_REF;
        stack[sp].fwd = (Value*)fetch16();
        ++sp;
        vmbreak;
    }
    vmcase(ADD) {
        --sp;
        stack[sp-1]._int += stack[sp]._int;
        vmbreak;
    }
    vmcase(SUB) {
        --sp;
        stack[sp-1]._int -= stack[sp]._int;
        vmbreak;
    }
    vmcase(MUL) {
        --sp;
        stack[sp-1]._int *= stack[sp]._int;
        vmbreak;
    }
    vmcase(DIV) {
        --sp;
        stack[sp-1]._int /= stack[sp]._int;
        vmbreak;
    }
    vmcase(MOD) {
        --sp;
        stack[sp-1]._int %= stack[sp]._int;
        vmbreak;
    }
    vmcase(EQ) {
        --sp;
        if(stack[sp].tag == V_INT && stack[sp-1].tag == V_INT) {
            stack[sp-1].tag = V_BOOL;
            stack[sp-1]._int = stack[sp-1]._int == stack[sp]._int;
        } else {
            panic(0x91);
        }
        vmbreak;
    }
    vmcase(LT) {
        --sp;
        if(stack[sp].tag == V_INT && stack[sp-1].tag == V_INT) {
            stack[sp-1].tag = V_BOOL;
            stack[sp-1]._int = stack[sp-1]._int < stack[sp]._int;
        } else {
            panic(0x92);
        }
        vmbreak;
    }
    vmcase(GT) {
        --sp;
        if(stack[sp].tag == V_INT && stack[sp-1].tag == V_INT) {
            stack[sp-1].tag = V_BOOL;
            stack[sp-1]._int = stack[sp-1]._int > stack[sp]._int;
        } else {
            panic(0x93);
        }
        vmbreak;
    }
    vmcase(NOT) {
        if(stack[sp-1].tag == V_BOOL) {
            if(stack[sp-1]._int) stack[sp-1]._int = 0;
            else stack[sp-1]._int = 1;
        } else if(stack[sp-1].tag == V_INT) {
            stack[sp-1]._int = ~stack[sp-1]._int;
        } else {
            panic(0x38);
        }
        vmbreak;
    }
    vmcase(AND) {
        --sp;
        if(stack[sp].tag == V_BOOL && stack[sp-1].tag == V_BOOL) {
            stack[sp-1]._int = stack[sp]._int && stack[sp-1]._int;
        } else if(stack[sp].tag == V_INT && stack[sp-1].tag == V_INT) {
            stack[sp-1]._int = stack[sp]._int & stack[sp-1]._int;
        } else {
            panic(0x49);
        }
        vmbreak;
    }
    vmcase(OR) {
        --sp;
        if(stack[sp].tag == V_BOOL && stack[sp-1].tag == V_BOOL) {
            stack[sp-1]._int = stack[sp]._int || stack[sp-1]._int;
        } else if(stack[sp].tag == V_INT && stack[sp-1].tag == V_INT) {
            stack[sp-1]._int = stack[sp]._int | stack[sp-1]._int;
        } else {
            panic(0x4A);
        }
        vmbreak;
    }
    vmcase(LOAD) {
        i = stack[sp-1]._int;
        memcpy(&stack[sp-1], &globals[i], sizeof(Value));
        vmbreak;
    }
    vmcase(STORE) {
        i = stack[sp-1]._int;
        memcpy(&globals[i], &stack[sp-2], sizeof(Value));
        sp -= 2;
        vmbreak;
    }
    vmcase(STACKSIZE) {
        stack[sp].tag = V_INT;
        stack[sp]._int = sp;
        ++sp;
        vmbreak;
    }
    vmcase(DUP) {
        memcpy(&stack[sp], &stack[sp-1], sizeof(Value));
        ++sp;
        vmbreak;
    }
    vmcase(DROP) {
        --sp;
        vmbreak;
    }
    vmcase(SWAP) {
        memcpy(&v, &stack[sp-1], sizeof(Value));
        memcpy(&stack[sp-1], &stack[sp-2], sizeof(Value));
        memcpy(&stack[sp-2], &v, sizeof(Value));
        vmbreak;
    }
    vmcase(NEWOBJ) {
        panic(0x82);
        vmbreak;
    }
    vmcase(NEWARR) {
        vp1 = (Value*)gc_alloc(sizeof(Value) + sizeof(Value) * ARR_DEFAULT_SIZE);
        vp1->tag = V_ARRAY;
        vp1->gc.next = heap_last;
        heap_last = &vp1->gc;
        vp1->gc._arr = (Array*)(((u8*)vp1) + sizeof(Value));
        vp1->gc._arr->count = 0;
        vp1->gc._arr->size = ARR_DEFAULT_SIZE;
        stack[sp].tag = V_REF;
        stack[sp].fwd = vp1;
        ++sp;
        vmbreak;
    }
    vmcase(GET) {
        --sp;
        vp1 = resolve(&stack[sp]);
        if(vp1->tag == V_ARRAY && stack[sp-1].tag == V_INT) {
            j = stack[sp-1]._int;
            if(j < vp1->gc._arr->count) {
                memcpy(&stack[sp-1], &vp1->gc._arr->data[j], sizeof(Value));
            } else {
                panic(0x21);
            }
        } else {
            acia_put_hex_byte(vp1->tag);
            acia_put_hex_byte(stack[sp-1].tag);
            acia_putc('\n');
            panic(0x11);
        }
        vmbreak;
    }
    vmcase(SET) {
        --sp;
        vp1 = resolve(&stack[sp]);
        if(vp1->tag == V_ARRAY && stack[sp-1].tag == V_INT) {
            j = stack[sp-1]._int;
            a = v2a(vp1);
            if(i < a->count) {
                memcpy(&a->data[j], &stack[sp-2], sizeof(Value));
            } else {
                panic(0x22);
            }
        } else {
            panic(0x12);
        }
        --sp;
        vmbreak;
    }
    vmcase(EXEC) {
        --sp;
        i = stack[sp].tag;
        if(i == V_REF) {
            interpret(stack[sp].fwd);
        } else if(i == V_NATIVE) {
            stack[sp].native();
        } else {
            acia_put_hex_byte(i);
            panic(0x69);
        }
        vmbreak;
    }
    vmcase(CONDEXEC) {
        vp1 = stack[--sp].fwd;
        if(stack[--sp]._int) {
            interpret(vp1);
        }
        vmbreak;
    }
    vmcase(WHILE) {
        vp1 = stack[--sp].fwd;
        vp2 = stack[--sp].fwd;
        for(;;) {
            interpret(vp2);
            if(!stack[--sp]._int) break;
            interpret(vp1);
        }
        vmbreak;
    }
_done:

    #undef vmcase
    #undef vmbreak

    #undef fetch8
    #undef fetch16
    
    #undef TRACE

    return;
}


typedef struct LexState {
    char *p;
} LexState;

static Value* compile(LexState *l) {
    u16 code_size = 0;
    u16 mark = comp_buf_next;
    Value *result;
    Value *v;
    char c;
    char *xp;
    u8 level, n, r, s;
    u16 x, m;

    // TODO: overflow
    #define emit(x) do { comp_buf[comp_buf_next++] = x; code_size++; } while(0)

    while(*l->p) {
        while(*l->p && is_whitespace(*l->p)) l->p++;

        c = *l->p++;
        switch(c) {
            case 0:
                goto _end;
            case '{': {
                level = 1;
                while(level) {
                    c = *l->p++;
                    if(c == 0) {
                        break;
                    } else if(c == '{') {
                        ++level;
                    } else if(c == '}') {
                        --level;
                    }
                }
                if(level) {
                    // TODO
                    panic(0x01);
                }
                break;
            }
            case '"':
                xp = l->p;
                while((c = *l->p) && c != '"') ++l->p;
                if(*l->p++ != '"') panic(0x59);
                m = l->p - xp - 1;

                v = (Value*)gc_alloc(sizeof(Value) + m + 1);
                v->tag = V_STRING;
                v->gc.next = heap_last;
                heap_last = &v->gc;
                v->gc._str = (char*)((u8*)v) + sizeof(Value);
                memcpy(v->gc._str, xp, m);
                v->gc._str[m] = 0;

                goto _1;
                break;
            case '[':
                v = compile(l);
_1:             x = (u16)v;
                emit(OP_PUSHVAL);
                emit((u8)((x >> 8) & 0xFF));
                emit((u8)(x & 0xFF));
                break;
            case ']':
                // TODO: is this always safe w/o any kind of checking?
                //       should be right? since we recurse on [
                goto _end;
            case '(':
                // TODO
                break;
            case '+':
                emit(OP_ADD);
                break;
            case '-':
                emit(OP_SUB);
                break;
            case '*':
                emit(OP_MUL);
                break;
            case '/':
                emit(OP_DIV);
                break;
            case '%':
                emit(OP_MOD);
                break;
            case '=':
                emit(OP_EQ);
                break;
            case '<':
                emit(OP_LT);
                break;
            case '>':
                emit(OP_GT);
                break;
            case '~':
                emit(OP_NOT);
                break;
            case '&':
                emit(OP_AND);
                break;
            case '|':
                emit(OP_OR);
                break;
            case ';':
                emit(OP_LOAD);
                break;
            case ':':
                emit(OP_STORE);
                break;
            case '`':
                emit(OP_STACKSIZE);
                break;
            case '$':
                emit(OP_DUP);
                break;
            case '@':
                emit(OP_DROP);
                break;
            case '^':
                emit(OP_SWAP);
                break;
            case '\\':
                emit(OP_NEWOBJ);
                break;
            case '_':
                emit(OP_NEWARR);
                break;
            case ',':
                emit(OP_GET);
                break;
            case '.':
                emit(OP_SET);
                break;
            case '!':
                emit(OP_EXEC);
                break;
            case '?':
                emit(OP_CONDEXEC);
                break;
            case '#':
                emit(OP_WHILE);
                break;
            default:
                if(is_digit(c)) {
                    // TODO: hex and binary numbers
                    x = 0;
                    m = 1;

                    if(c == '0' && *l->p == 'x') {
                        r = 16;
                        s = 4;
                        n = 0;
                        ++l->p;
                    } else if(c == '0' && *l->p == 'b') {
                        r = 2;
                        s = 16;
                        n = 0;
                        ++l->p;
                    } else {
                        r = 10;
                        s = 5;
                        n = 1;
                    }

                    while(c = *l->p) {
                        if(r == 16 && !(is_digit(c) || is_hex(c))) break;
                        else if(r == 10 && !is_digit(c)) break;
                        else if(r == 2 && !(c == '0' || c == '1')) break;

                        // TODO: overflow
                        if(n == s) panic(0x03);

                        ++l->p;
                        ++n;
                    }

                    if(r != 10 && n == 0) {
                        --l->p;
                        emit(OP_PUSHINT);
                        emit(0);
                        emit(0);
                        break;
                    }

                    xp = l->p - 1;
                    for(; n > 0; n--) {
                        u8 d = *xp;
                        --xp;

                        if((d == 'x' && r == 16) || (d == 'b' && r == 2)) continue;

                        if(d >= '0' && d <= '9') d -= '0';
                        else if(d >= 'a' && d <= 'f') d = d - 'a' + 10;
                        else if(d >= 'A' && d <= 'F') d = d - 'A' + 10;
                        else panic(0x76);

                        x += d * m;
                        m *= r;
                    }

                    emit(OP_PUSHINT);
                    emit((u8)((x >> 8) & 0xFF));
                    emit((u8)(x & 0xFF));
                } else if(is_alpha(c)) {
                    n = 1;
                    xp = l->p - 1;

                    while(*l->p && is_alpha(*l->p)) {
                       // TODO: overflow
                       if(n == 255) panic(0x04);

                       l->p++;
                       n++;
                    }

                    if(n == 4 && memcmp(xp, "true", 4) == 0) {
                        emit(OP_PUSHTRUE);
                    } else if(n == 5 && memcmp(xp, "false", 5) == 0) {
                        emit(OP_PUSHFALSE);
                    } else if(n == 3 && memcmp(xp, "nil", 3) == 0) {
                        emit(OP_PUSHNIL);
                    } else {
                        x = (u16)intern(xp, n);
                        emit(OP_PUSHSYM);
                        emit((u8)((x >> 8) & 0xFF));
                        emit((u8)(x & 0xFF));
                    }
                } else {
                    // TODO: cry
                    acia_put_hex_byte(c);
                    panic(0x05);
                }
                break;
        }
    }
_end:

    #undef emit

    result = (Value*)gc_alloc(sizeof(Value) + sizeof(Lambda) + code_size);
    result->tag = V_LAMBDA;
    result->gc.next = heap_last;
    heap_last = &result->gc;
    result->gc._lam = (Lambda*)(((u8*)result) + sizeof(Value));
    result->gc._lam->code_size = code_size;
    memcpy(result->gc._lam->code, &comp_buf[mark], code_size);

    comp_buf_next = mark;

    return result;
}


void fadebabe_run(void) {
    Value *main;
    LexState l;

    init_natives();

    //memset(intern_data, 0, sizeof(intern_data));
    //memset(globals, 0, sizeof(globals));
    //memset(stack, 0, sizeof(stack));

    l.p = fbsrc;
    main = compile(&l);

    interpret(main);
}