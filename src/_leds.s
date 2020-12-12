.include "navi65.inc"


.code
.export leds_init
.export _leds_init=leds_init
.proc leds_init
	stz LEDS1
	stz LEDS2
	rts
.endproc


.export leds_set_value
.export _leds_set_value=leds_set_value
.proc leds_set_value
	phx
	pha
	pha

	and #$0F
	tax
	lda s_7seg_lut,x
	sta LEDS2

	pla
	and #$F0
	lsr
	lsr
	lsr
	lsr
	tax
	lda s_7seg_lut,x
	sta LEDS1

	pla
	plx
	rts
.endproc


.rodata
s_7seg_lut:
	.byte %00111111
	.byte %00000110
	.byte %01011011
	.byte %01001111
	.byte %01100110
	.byte %01101101
	.byte %01111101
	.byte %00000111
	.byte %01111111
	.byte %01101111
	.byte %01110111
	.byte %01111100
	.byte %00111001
	.byte %01011110
	.byte %01111001
	.byte %01110001