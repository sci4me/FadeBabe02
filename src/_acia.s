.include "navi65.inc"
.include "macros.inc"


.zeropage
p1: .res 2
head: .res 1
tail: .res 1
count: .res 1


.data
.align 256
buffer: .res 128


bufovfl_tramp: jmp $0000
bufovfl_handler = bufovfl_tramp+1


.code
.export acia_on_recv
.proc acia_on_recv
    phx
    ldx count
    bmi @bufovfl
    ldx tail
    sta buffer,x
    inc tail
    rmb7 tail
    inc count
    plx
    rts
@bufovfl:
    jsr bufovfl_tramp
    plx
    rts
.endproc


.export acia_init
.export _acia_init=acia_init
.proc acia_init
    pha
    
    stz head
    stz tail
    stz count
    sta bufovfl_handler
    stx bufovfl_handler+1

    sei
	lda #%00001001
    sta ACIA_COMMAND
    lda #%00011111
    sta ACIA_CONTROL
    cli

    pla
    rts
.endproc


.export acia_available
.export _acia_available=acia_available
.proc acia_available
    lda count
    rts
.endproc


.export acia_putc
.export _acia_putc=acia_putc
.proc acia_putc
	phx
    phy
	ldx #$68
    ldy #CLOCK_MHZ
:	dex
	bne :-
    dey
    bne :-
	ply
    plx
	sta ACIA_DATA
	rts
.endproc


.export acia_getc
.export _acia_getc=acia_getc
.proc acia_getc
:   lda count
    beq :-
    phx
    ldx head
    lda buffer,x
    inc head
    rmb7 head
    dec count
    plx
	rts
.endproc


.export acia_puts
.export _acia_puts=acia_puts
.proc acia_puts
	pha
    phx
    sta p1
    stx p1+1
:	lda (p1)
	beq :+
	jsr acia_putc
	inc16 p1
	bra :-
:	plx
    pla
	rts
.endproc


.export acia_putln
.export _acia_putln=acia_putln
.proc acia_putln
    jsr acia_puts
    pha
    lda #10
    jsr acia_putc
    pla
    rts
.endproc


.export acia_put_hex_byte
.export _acia_put_hex_byte=acia_put_hex_byte
.proc acia_put_hex_byte
    phx
    pha
    pha
    lsr
    lsr
    lsr
    lsr
    and #$0F
    tax
    lda s_hex_lut,x
    jsr acia_putc
    pla
    and #$0F
    tax
    lda s_hex_lut,x
    jsr acia_putc
    pla
    plx
    rts
.endproc


.rodata
s_hex_lut: .byte "0123456789ABCDEF"