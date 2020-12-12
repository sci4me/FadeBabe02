.import __RAM_START__
.segment "EXEHDR"
.addr __RAM_START__
.addr main


.include "macros.inc"
.include "navi65.inc"


.import __CSTACK_START__, __CSTACK_SIZE__
.importzp sp


.import zerobss
.import copydata

.import acia_on_recv
.import leds_set_value
.import _kmain


TIMER_INTERVAL = 10000


.zeropage
t1:             .res 1
rbst:           .res 1
rbrst:          .res 1
delay_timer:    .res 1


.code
.macro endisr
    ply
    plx
    pla
    rti
.endmacro

.proc isr
    pha
    phx
    phy

    lda IRQSRC
    ; NOTE: Not sure why this breaks things... it's not the flags.. so.. ???
    ; and #$07 ; NOTE: this is slightly dangerous; we just ignore the top five bits regardless of whether they were set or not
    tax
    jmp (isrs,x)
.endproc

.rodata
.align 256 ; Not needed? *shrugs* Not really sure how ld65's alignment is handled so.. ehh.
isrs:
.addr isr_invalid
.addr isr_invalid
.addr isr_invalid
.addr isr_invalid
.addr isr_invalid
.addr isr_via2
.addr isr_via1
.addr isr_acia


reboot_seq: .byte "$XBOOT$", 0
.code

.proc isr_acia
    lda #$80
    bit ACIA_STATUS
    beq @end

    lda ACIA_DATA
    jsr acia_on_recv

    ldx rbst
    lda reboot_seq,x
    cmp ACIA_DATA
    bne :+
    inx
    lda reboot_seq,x
    beq @rb
    stx rbst
    bra @end

:   stz rbst
@end:   endisr
@rb:    lda #$7F
        sta VIA1_IER
        sta VIA2_IER
        stz ACIA_STATUS
        jmp ($FFFC)
.endproc

.proc isr_via1
    ; TODO
    endisr
.endproc

.proc isr_via2
    ; TODO: Technically we should be checking IFR...


    lda rbst
    beq :++    
    lda rbrst
    inc a
    cmp #20
    bne :+
    stz rbrst
    stz rbst
    bra :++
:   sta rbrst
:   lda VIA2_T1C_L

    
    lda delay_timer
    beq :+
    dec delay_timer
:   endisr
.endproc

.proc isr_invalid
    ; TODO
    endisr
.endproc


.proc main
    sei
    cld
    ldx #$FE
    txs

    
    ldx #0
:   stz $00,x
    inx
    bne :-


    ld16 $02, isr ; NOTE: $02 is IRQVEC


    jsr zerobss
    jsr copydata ; NOTE: Not needed since we're loading into RAM


    ld16 sp, (__CSTACK_START__ + __CSTACK_SIZE__)


    lda #%01000000
    sta VIA2_ACR
    lda #%11000000
    sta VIA2_IER
    lda #<TIMER_INTERVAL
    sta VIA2_T1C_L
    lda #>TIMER_INTERVAL
    sta VIA2_T1C_H


    cli


    jsr _kmain


:   stp
    jmp :-
.endproc


.export panic
.export _panic=panic
.proc panic
    jsr leds_set_value
:   wai
    bra :-
.endproc


.export delay_ms
.export _delay_ms=delay_ms
.proc delay_ms
    sta delay_timer
:   lda delay_timer
    bne :-
    rts
.endproc