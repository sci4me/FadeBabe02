#include "io.h"
#include "acia.h"


void put_hex_word(u16 x) {
	acia_put_hex_byte((x >> 8) & 0xFF);
	acia_put_hex_byte(x & 0xFF);
}