#ifndef ACIA_H
#define ACIA_H


#include "types.h"


extern void __fastcall__ acia_init(void* overflow_handler);
extern u8 __fastcall__ acia_available(void);
extern void __fastcall__ acia_putc(u8 x);
extern u8 __fastcall__ acia_getc(void);
extern void __fastcall__ acia_puts(char *s);
extern void __fastcall__ acia_putln(char *s);
extern void __fastcall__ acia_put_hex_byte(u8 x);


#endif