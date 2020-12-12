.include "navi65.inc"


.code
.export switch1_read
.export _switch1_read=switch1_read
.proc switch1_read
	lda SWITCH1
	eor #$FF
	rts
.endproc

.export switch2_read
.export _switch2_read=switch1_read
.proc switch2_read
	lda SWITCH2
	eor #$FF
	rts
.endproc