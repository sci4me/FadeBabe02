CLOCK_MHZ = 10


ACIA_DATA        = $7F00
ACIA_STATUS      = $7F01
ACIA_COMMAND     = $7F02
ACIA_CONTROL     = $7F03

VIA1_ORB         = $7F20
VIA1_ORA         = $7F21
VIA1_DDRB        = $7F22
VIA1_DDRA        = $7F23
VIA1_T1C_L       = $7F24
VIA1_T1C_H       = $7F25
VIA1_T1L_L       = $7F26
VIA1_T1L_H       = $7F27
VIA1_T2C_L       = $7F28
VIA1_T2C_H       = $7F29
VIA1_SR          = $7F2A
VIA1_ACR         = $7F2B
VIA1_PCR         = $7F2C
VIA1_IFR         = $7F2D
VIA1_IER         = $7F2E
VIA1_IRA         = $7F2F

VIA2_ORB         = $7F40
VIA2_ORA         = $7F41
VIA2_DDRB        = $7F42
VIA2_DDRA        = $7F43
VIA2_T1C_L       = $7F44
VIA2_T1C_H       = $7F45
VIA2_T1L_L       = $7F46
VIA2_T1L_H       = $7F47
VIA2_T2C_L       = $7F48
VIA2_T2C_H       = $7F49
VIA2_SR          = $7F4A
VIA2_ACR         = $7F4B
VIA2_PCR         = $7F4C
VIA2_IFR         = $7F4D
VIA2_IER         = $7F4E
VIA2_IRA         = $7F4F

IRQSRC 			 = $7FE0

LEDS1			 = $7FE2
LEDS2			 = $7FE1

SWITCH1 		 = $7FE3
SWITCH2   	 	 = $7FE4


.struct ACIA
	.org $7F00
	data 	 	.byte
	status  	.byte
	command 	.byte
	control 	.byte
.endstruct


.struct VIA1
	.org $7F20
	prb 		.byte
	pra 		.byte
	ddrb 		.byte
	ddra 		.byte
	t1c_l		.byte
	t1c_h 		.byte
	t1l_l 		.byte
	t1l_h 		.byte
	t2c_l 		.byte
	t2c_h 		.byte
	sr 			.byte
	acr 		.byte
	pcr 		.byte
	ifr 		.byte
	ier 		.byte
	pra2 		.byte
.endstruct


.struct VIA2
	.org $7F40
	prb 		.byte
	pra 		.byte
	ddrb 		.byte
	ddra 		.byte
	t1c_l		.byte
	t1c_h 		.byte
	t1l_l 		.byte
	t1l_h 		.byte
	t2c_l 		.byte
	t2c_h 		.byte
	sr 			.byte
	acr 		.byte
	pcr 		.byte
	ifr 		.byte
	ier 		.byte
	pra2 		.byte
.endstruct