#ifndef NAVI65_H
#define NAVI65_H


#include "types.h"


typedef struct {
	volatile u8 data;
	volatile u8 status;
	volatile u8 command;
	volatile u8 control;
} __ACIA;

#define ACIA (*(__ACIA*)0x7F00)


typedef struct {
	volatile u8 prb;
	volatile u8 pra;
	volatile u8 ddrb;
	volatile u8 ddra;
	volatile u8 t1cl;
	volatile u8 t1ch;
	volatile u8 t1ll;
	volatile u8 t1lh;
	volatile u8 t2cl;
	volatile u8 t2ch;
	volatile u8 sr;
	volatile u8 acr;
	volatile u8 pcr;
	volatile u8 ifr;
	volatile u8 ier;
	volatile u8 _pra;
} __VIA;

#define VIA1 (*(__VIA*)0x7F20)
#define VIA2 (*(__VIA*)0x7F40)


#define IRQSRC (*(u8*)0x7FE0)


#define LEDS1 (*(u8*)0x7FE2)
#define LEDS2 (*(u8*)0x7FE1)


#define SWITCH1 (*(u8*)0x7FE3)
#define SWITCH2 (*(u8*)0x7FE4)


extern void __fastcall__ panic(u8 x);
extern void __fastcall__ delay_ms(u8 x);


#endif