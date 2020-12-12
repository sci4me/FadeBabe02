.macro stz16 addr
                        stz addr
                        stz addr+1
.endmacro

.macro inc16 addr
.local @skip
			inc addr
			bne @skip
			inc addr+1
@skip:	
.endmacro

.macro dec16 addr
.local @done
                        lda addr
                        bne @done
                        dec addr+1
@done:                  dec addr
.endmacro

.macro ld16 addr, value
			lda #<value
			sta addr
			lda #>value
			sta addr+1
.endmacro

.macro push16 addr
                        lda addr
                        pha
                        lda addr+1
                        pha
.endmacro

.macro pop16 addr
                        pla
                        sta addr+1
                        pla
                        sta addr
.endmacro

.macro addl16 addr, value
                        clc
                        lda addr
                        adc #<value
                        sta addr
                        lda addr+1
                        adc #>value
                        sta addr+1
.endmacro