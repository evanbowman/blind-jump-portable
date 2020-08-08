
	.text
	.code 16

	.global	RegisterRamReset
	.thumb_func

RegisterRamReset:
	swi		1
	bx		lr

	.global	SoftReset
	.thumb_func

SoftReset:
	ldr		r3, =0x03007FFA		@ restart flag
	strb	r0,[r3, #0]
	ldr		r3, =0x04000208		@ REG_IME
	mov		r2, #0
	strb	r2, [r3, #0]
	ldr		r1, =0x03007f00
	mov		sp, r1
	swi		1
	swi		0

	.pool
