/*

	libgba interrupt support routines

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

#include "gba.h"

//---------------------------------------------------------------------------------
struct IntTable IntrTable[MAX_INTS];
void dummy(void) {}

#define NULL 0

//---------------------------------------------------------------------------------
void InitInterrupt(void) {
//---------------------------------------------------------------------------------
	irqInit();
}

//---------------------------------------------------------------------------------
void irqInit() {
//---------------------------------------------------------------------------------
	int i;

	// Set all interrupts to dummy functions.
	for(i = 0; i < MAX_INTS; i ++)
	{
		IntrTable[i].handler = dummy;
		IntrTable[i].mask = 0;
	}

	INT_VECTOR = IntrMain;
}

//---------------------------------------------------------------------------------
IntFn* SetInterrupt(irqMASK mask, IntFn function) {
//---------------------------------------------------------------------------------
	return irqSet(mask,function);
}

//---------------------------------------------------------------------------------
IntFn* irqSet(irqMASK mask, IntFn function) {
//---------------------------------------------------------------------------------
	int i;

	for	(i=0;;i++) {
		if	(!IntrTable[i].mask || IntrTable[i].mask == mask) break;
	}

	if ( i >= MAX_INTS) return NULL;

	IntrTable[i].handler	= function;
	IntrTable[i].mask		= mask;

	return &IntrTable[i].handler;

}

//---------------------------------------------------------------------------------
void EnableInterrupt(irqMASK mask) {
//---------------------------------------------------------------------------------
	irqEnable(mask);
}

//---------------------------------------------------------------------------------
void irqEnable ( int mask ) {
//---------------------------------------------------------------------------------
	REG_IME	= 0;

	if (mask & IRQ_VBLANK) REG_DISPSTAT |= LCDC_VBL;
	if (mask & IRQ_HBLANK) REG_DISPSTAT |= LCDC_HBL;
	if (mask & IRQ_VCOUNT) REG_DISPSTAT |= LCDC_VCNT;
	REG_IE |= mask;
	REG_IME	= 1;
}

//---------------------------------------------------------------------------------
void DisableInterrupt(irqMASK mask) {
//---------------------------------------------------------------------------------
	irqDisable(mask);
}

//---------------------------------------------------------------------------------
void irqDisable(int mask) {
//---------------------------------------------------------------------------------
	REG_IME	= 0;

	if (mask & IRQ_VBLANK) REG_DISPSTAT &= ~LCDC_VBL;
	if (mask & IRQ_HBLANK) REG_DISPSTAT &= ~LCDC_HBL;
	if (mask & IRQ_VCOUNT) REG_DISPSTAT &= ~LCDC_VCNT;
	REG_IE &= ~mask;

	REG_IME	= 1;

}
