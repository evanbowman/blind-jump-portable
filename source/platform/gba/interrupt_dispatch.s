/*

	libgba interrupt dispatcher routines

	Copyright 2003-2007 by Dave Murphy.

	This library is free software; you can redistribute it and/or
	modify it under the terms of the GNU Library General Public
	License as published by the Free Software Foundation; either
	version 2 of the License, or (at your option) any later version.

	This library is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
	Library General Public License for more details.

	You should have received a copy of the GNU Library General Public
	License along with this library; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
	USA.

	Please report all bugs and problems through the bug tracker at
	"http://sourceforge.net/tracker/?group_id=114505&atid=668551".


*/

	.section	.iwram,"ax",%progbits
	.extern	IntrTable
	.code 32

	.global	IntrMain

IntrMain:
	mov	r3, #0x4000000
	ldr	r2, [r3,#0x200]

	ldr	r1, [r3, #0x208]
	str	r3, [r3, #0x208]
	mrs	r0, spsr
	stmfd	sp!, {r0-r1,r3,lr}

	and	r1, r2,	r2, lsr #16

	ldrh	r2, [r3, #-8]
	orr	r2, r2, r1
	strh	r2, [r3, #-8]

	ldr	r2,=IntrTable
	add	r3,r3,#0x200


findIRQ:
	ldr	r0, [r2, #4]
	cmp	r0,#0
	beq	no_handler
	ands	r0, r0, r1
	bne	jump_intr
	add	r2, r2, #8
	b	findIRQ

no_handler:
	strh	r1, [r3, #0x02]
	ldmfd	sp!, {r0-r1,r3,lr}
	str	r1, [r3, #0x208]
	mov	pc,lr

jump_intr:
	ldr	r2, [r2]
	cmp	r2, #0
	beq	no_handler

got_handler:
	mrs	r1, cpsr
	bic	r1, r1, #0xdf
	orr	r1, r1, #0x1f
	msr	cpsr,r1

	strh	r0, [r3, #0x02]

	push	{lr}
	adr	lr, IntrRet
	bx	r2

IntrRet:
	pop	{lr}
	mov	r3, #0x4000000
	str	r3, [r3, #0x208]

	mrs	r3, cpsr
	bic	r3, r3, #0xdf
	orr	r3, r3, #0x92
	msr	cpsr, r3

	ldmfd   sp!, {r0-r1,r3,lr}
	str	r1, [r3, #0x208]
	msr	spsr, r0
	mov	pc,lr

	.pool
	.end
