#ifndef TYPES_H
#define TYPES_H


typedef char s8;
typedef unsigned char u8;
typedef int s16;
typedef unsigned int u16;
typedef long s32;
typedef unsigned long u32;


#ifndef offsetof
#define offsetof(type, member) ((u16)(&((type*)0)->member))
#endif


#endif