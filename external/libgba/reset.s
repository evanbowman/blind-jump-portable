/*
	libgba bios reset routines
	Copyright 2003-2004 by Dave Murphy.
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
	.text
