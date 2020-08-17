#pragma once


#include "number/int.h"


#define REG_BASE 0x04000000
#define REG_DISPCNT *(u32*)0x4000000
#define	REG_DISPSTAT	*((volatile u16 *)(REG_BASE + 0x04))
#define MODE_0 0x0
#define OBJ_MAP_1D 0x40
#define OBJ_ENABLE 0x1000
#define REG_WAITCNT *(u16*)0x4000204

#define SE_PALBANK_MASK 0xF000
#define SE_PALBANK_SHIFT 12
#define SE_PALBANK(n) ((n) << SE_PALBANK_SHIFT)

#define ATTR2_PALBANK_MASK 0xF000
#define ATTR2_PALBANK_SHIFT 12
#define ATTR2_PALBANK(n) ((n) << ATTR2_PALBANK_SHIFT)

#define ATTR2_PRIO_SHIFT 10
#define ATTR2_PRIO(n) ((n) << ATTR2_PRIO_SHIFT)
#define ATTR2_PRIORITY(n) ATTR2_PRIO(n)

#define REG_MOSAIC *(volatile u16*)(0x04000000 + 0x4c)

#define BG_MOSAIC (1 << 6)

#define MOS_BH_MASK 0x000F
#define MOS_BH_SHIFT 0
#define MOS_BH(n) ((n) << MOS_BH_SHIFT)

#define MOS_BV_MASK 0x00F0
#define MOS_BV_SHIFT 4
#define MOS_BV(n) ((n) << MOS_BV_SHIFT)

#define MOS_OH_MASK 0x0F00
#define MOS_OH_SHIFT 8
#define MOS_OH(n) ((n) << MOS_OH_SHIFT)

#define MOS_OV_MASK 0xF000
#define MOS_OV_SHIFT 12
#define MOS_OV(n) ((n) << MOS_OV_SHIFT)

#define MOS_BUILD(bh, bv, oh, ov)                                              \
    ((((ov)&15) << 12) | (((oh)&15) << 8) | (((bv)&15) << 4) | ((bh)&15))

#define REG_TM3CNT_L *(volatile u16*)(REG_BASE + 0x10c)
#define REG_TM3CNT_H *(volatile u16*)(REG_BASE + 0x10e)
#define REG_TM2CNT *(volatile u32*)(0x04000000 + 0x108)
#define REG_TM2CNT_L *(volatile u16*)(REG_BASE + 0x108)
#define REG_TM2CNT_H *(volatile u16*)(REG_BASE + 0x10a)

typedef u32 HardwareTile[16];
typedef HardwareTile TileBlock[256];
typedef u16 ScreenBlock[1024];

#define EWRAM_DATA	__attribute__((section(".ewram")))
#define IWRAM_DATA	__attribute__((section(".iwram")))

#define MEM_TILE ((TileBlock*)0x6000000)
#define MEM_PALETTE ((u16*)(0x05000200))
#define MEM_BG_PALETTE ((u16*)(0x05000000))
#define MEM_SCREENBLOCKS ((ScreenBlock*)0x6000000)

#define OBJ_SHAPE(m) ((m) << 14)
#define ATTR0_COLOR_16 (0 << 13)
#define ATTR0_COLOR_256 (1 << 13)
#define ATTR0_MOSAIC (1 << 12)
#define ATTR0_SQUARE OBJ_SHAPE(0)
#define ATTR0_TALL OBJ_SHAPE(2)
#define ATTR0_WIDE OBJ_SHAPE(1)
#define ATTR0_BLEND 0x0400
#define ATTR1_SIZE_16 (1 << 14)
#define ATTR1_SIZE_32 (2 << 14)
#define ATTR1_SIZE_64 (3 << 14)
#define ATTR2_PALETTE(n) ((n) << 12)
#define OBJ_CHAR(m) ((m)&0x03ff)
#define BG0_ENABLE 0x100
#define BG1_ENABLE 0x200
#define BG2_ENABLE 0x400
#define BG3_ENABLE 0x800
#define WIN0_ENABLE (1 << 13)
#define WIN_BG0 0x0001 //!< Windowed bg 0
#define WIN_BG1 0x0002 //!< Windowed bg 1
#define WIN_BG2 0x0004 //!< Windowed bg 2
#define WIN_BG3 0x0008 //!< Windowed bg 3
#define WIN_OBJ 0x0010 //!< Windowed objects
#define WIN_ALL 0x001F //!< All layers in window.
#define WIN_BLD 0x0020 //!< Windowed blending
#define REG_WIN0H *((volatile u16*)(0x04000000 + 0x40))
#define REG_WIN1H *((volatile u16*)(0x04000000 + 0x42))
#define REG_WIN0V *((volatile u16*)(0x04000000 + 0x44))
#define REG_WIN1V *((volatile u16*)(0x04000000 + 0x46))
#define REG_WININ *((volatile u16*)(0x04000000 + 0x48))
#define REG_WINOUT *((volatile u16*)(0x04000000 + 0x4A))

#define BG_CBB_MASK 0x000C
#define BG_CBB_SHIFT 2
#define BG_CBB(n) ((n) << BG_CBB_SHIFT)

#define BG_SBB_MASK 0x1F00
#define BG_SBB_SHIFT 8
#define BG_SBB(n) ((n) << BG_SBB_SHIFT)

#define BG_SIZE_MASK 0xC000
#define BG_SIZE_SHIFT 14
#define BG_SIZE(n) ((n) << BG_SIZE_SHIFT)

#define BG_PRIORITY(m) ((m))

#define BG_REG_32x32 0      //!< reg bg, 32x32 (256x256 px)
#define BG_REG_64x32 0x4000 //!< reg bg, 64x32 (512x256 px)
#define BG_REG_32x64 0x8000 //!< reg bg, 32x64 (256x512 px)
#define BG_REG_64x64 0xC000 //!< reg bg, 64x64 (512x512 px)

//! \name Channel 1: Square wave with sweep
//\{
#define REG_SND1SWEEP *(volatile u16*)(0x04000000 + 0x0060) //!< Channel 1 Sweep
#define REG_SND1CNT *(volatile u16*)(0x04000000 + 0x0062)   //!< Channel 1 Control
#define REG_SND1FREQ *(volatile u16*)(0x04000000 + 0x0064)  //!< Channel 1 frequency
//\}

//! \name Channel 2: Simple square wave
//\{
#define REG_SND2CNT *(volatile u16*)(0x04000000 + 0x0068)  //!< Channel 2 control
#define REG_SND2FREQ *(volatile u16*)(0x04000000 + 0x006C) //!< Channel 2 frequency
//\}

//! \name Channel 3: Wave player
//\{
#define REG_SND3SEL *(volatile u16*)(0x04000000 + 0x0070)  //!< Channel 3 wave select
#define REG_SND3CNT *(volatile u16*)(0x04000000 + 0x0072)  //!< Channel 3 control
#define REG_SND3FREQ *(volatile u16*)(0x04000000 + 0x0074) //!< Channel 3 frequency
//\}

//! \name Channel 4: Noise generator
//\{
#define REG_SND4CNT *(volatile u16*)(0x04000000 + 0x0078)  //!< Channel 4 control
#define REG_SND4FREQ *(volatile u16*)(0x04000000 + 0x007C) //!< Channel 4 frequency
//\}

#define REG_SNDCNT *(volatile u32*)(0x04000000 + 0x0080) //!< Main sound control
#define REG_SNDDMGCNT                                                          \
    *(volatile u16*)(0x04000000 + 0x0080) //!< DMG channel control
#define REG_SNDDSCNT                                                           \
    *(volatile u16*)(0x04000000 + 0x0082) //!< Direct Sound control
#define REG_SNDSTAT *(volatile u16*)(0x04000000 + 0x0084) //!< Sound status
#define REG_SNDBIAS *(volatile u16*)(0x04000000 + 0x0088) //!< Sound bias


// --- REG_SND1SWEEP ---------------------------------------------------

/*!	\defgroup grpAudioSSW	Tone Generator, Sweep Flags
	\ingroup grpMemBits
	\brief	Bits for REG_SND1SWEEP (aka REG_SOUND1CNT_L)
*/
/*!	\{	*/

#define SSW_INC 0      //!< Increasing sweep rate
#define SSW_DEC 0x0008 //!< Decreasing sweep rate
#define SSW_OFF 0x0008 //!< Disable sweep altogether

#define SSW_SHIFT_MASK 0x0007
#define SSW_SHIFT_SHIFT 0
#define SSW_SHIFT(n) ((n) << SSW_SHIFT_SHIFT)

#define SSW_TIME_MASK 0x0070
#define SSW_TIME_SHIFT 4
#define SSW_TIME(n) ((n) << SSW_TIME_SHIFT)


#define SSW_BUILD(shift, dir, time)                                            \
    ((((time)&7) << 4) | ((dir) << 3) | ((shift)&7))

/*!	\}	/defgroup	*/

// --- REG_SND1CNT, REG_SND2CNT, REG_SND4CNT ---------------------------

/*!	\defgroup grpAudioSSQR	Tone Generator, Square Flags
	\ingroup grpMemBits
	\brief	Bits for REG_SND{1,2,4}CNT
	(aka REG_SOUND1CNT_H, REG_SOUND2CNT_L, REG_SOUND4CNT_L, respectively)
*/
/*!	\{	*/

#define SSQR_DUTY1_8 0      //!< 12.5% duty cycle (#-------)
#define SSQR_DUTY1_4 0x0040 //!< 25% duty cycle (##------)
#define SSQR_DUTY1_2 0x0080 //!< 50% duty cycle (####----)
#define SSQR_DUTY3_4 0x00C0 //!< 75% duty cycle (######--) Equivalent to 25%
#define SSQR_INC 0          //!< Increasing volume
#define SSQR_DEC 0x0800     //!< Decreasing volume

#define SSQR_LEN_MASK 0x003F
#define SSQR_LEN_SHIFT 0
#define SSQR_LEN(n) ((n) << SSQR_LEN_SHIFT)

#define SSQR_DUTY_MASK 0x00C0
#define SSQR_DUTY_SHIFT 6
#define SSQR_DUTY(n) ((n) << SSQR_DUTY_SHIFT)

#define SSQR_TIME_MASK 0x0700
#define SSQR_TIME_SHIFT 8
#define SSQR_TIME(n) ((n) << SSQR_TIME_SHIFT)

#define SSQR_IVOL_MASK 0xF000
#define SSQR_IVOL_SHIFT 12
#define SSQR_IVOL(n) ((n) << SSQR_IVOL_SHIFT)


#define SSQR_ENV_BUILD(ivol, dir, time)                                        \
    (((ivol) << 12) | ((dir) << 11) | (((time)&7) << 8))

#define SSQR_BUILD(_ivol, dir, step, duty, len)                                \
    (SSQR_ENV_BUILD(ivol, dir, step) | (((duty)&3) << 6) | ((len)&63))


/*!	\}	/defgroup	*/

// --- REG_SND1FREQ, REG_SND2FREQ, REG_SND3FREQ ------------------------

/*!	\defgroup grpAudioSFREQ	Tone Generator, Frequency Flags
	\ingroup grpMemBits
	\brief	Bits for REG_SND{1-3}FREQ
	(aka REG_SOUND1CNT_X, REG_SOUND2CNT_H, REG_SOUND3CNT_X)
*/
/*!	\{	*/

#define SFREQ_HOLD 0       //!< Continuous play
#define SFREQ_TIMED 0x4000 //!< Timed play
#define SFREQ_RESET 0x8000 //!< Reset sound

#define SFREQ_RATE_MASK 0x07FF
#define SFREQ_RATE_SHIFT 0
#define SFREQ_RATE(n) ((n) << SFREQ_RATE_SHIFT)

#define SFREQ_BUILD(rate, timed, reset)                                        \
    (((rate)&0x7FF) | ((timed) << 14) | ((reset) << 15))

/*!	\}	/defgroup	*/

// --- REG_SNDDMGCNT ---------------------------------------------------

/*!	\defgroup grpAudioSDMG	Tone Generator, Control Flags
	\ingroup grpMemBits
	\brief	Bits for REG_SNDDMGCNT (aka REG_SOUNDCNT_L)
*/
/*!	\{	*/


#define SDMG_LSQR1 0x0100  //!< Enable channel 1 on left
#define SDMG_LSQR2 0x0200  //!< Enable channel 2 on left
#define SDMG_LWAVE 0x0400  //!< Enable channel 3 on left
#define SDMG_LNOISE 0x0800 //!< Enable channel 4 on left
#define SDMG_RSQR1 0x1000  //!< Enable channel 1 on right
#define SDMG_RSQR2 0x2000  //!< Enable channel 2 on right
#define SDMG_RWAVE 0x4000  //!< Enable channel 3 on right
#define SDMG_RNOISE 0x8000 //!< Enable channel 4 on right

#define SDMG_LVOL_MASK 0x0007
#define SDMG_LVOL_SHIFT 0
#define SDMG_LVOL(n) ((n) << SDMG_LVOL_SHIFT)

#define SDMG_RVOL_MASK 0x0070
#define SDMG_RVOL_SHIFT 4
#define SDMG_RVOL(n) ((n) << SDMG_RVOL_SHIFT)


// Unshifted values
#define SDMG_SQR1 0x01
#define SDMG_SQR2 0x02
#define SDMG_WAVE 0x04
#define SDMG_NOISE 0x08


#define SDMG_BUILD(_lmode, _rmode, _lvol, _rvol)                               \
    (((_rmode) << 12) | ((_lmode) << 8) | (((_rvol)&7) << 4) | ((_lvol)&7))

#define SDMG_BUILD_LR(_mode, _vol) SDMG_BUILD(_mode, _mode, _vol, _vol)

/*!	\}	/defgroup	*/

// --- REG_SNDDSCNT ----------------------------------------------------

/*!	\defgroup grpAudioSDS	Direct Sound Flags
	\ingroup grpMemBits
	\brief	Bits for REG_SNDDSCNT (aka REG_SOUNDCNT_H)
*/
/*!	\{	*/

#define SDS_DMG25 0       //!< Tone generators at 25% volume
#define SDS_DMG50 0x0001  //!< Tone generators at 50% volume
#define SDS_DMG100 0x0002 //!< Tone generators at 100% volume
#define SDS_A50 0         //!< Direct Sound A at 50% volume
#define SDS_A100 0x0004   //!< Direct Sound A at 100% volume
#define SDS_B50 0         //!< Direct Sound B at 50% volume
#define SDS_B100 0x0008   //!< Direct Sound B at 100% volume
#define SDS_AR 0x0100     //!< Enable Direct Sound A on right
#define SDS_AL 0x0200     //!< Enable Direct Sound A on left
#define SDS_ATMR0 0       //!< Direct Sound A to use timer 0
#define SDS_ATMR1 0x0400  //!< Direct Sound A to use timer 1
#define SDS_ARESET 0x0800 //!< Reset FIFO of Direct Sound A
#define SDS_BR 0x1000     //!< Enable Direct Sound B on right
#define SDS_BL 0x2000     //!< Enable Direct Sound B on left
#define SDS_BTMR0 0       //!< Direct Sound B to use timer 0
#define SDS_BTMR1 0x4000  //!< Direct Sound B to use timer 1
#define SDS_BRESET 0x8000 //!< Reset FIFO of Direct Sound B

/*!	\}	/defgroup	*/

// --- REG_SNDSTAT -----------------------------------------------------

/*!	\defgroup grpAudioSSTAT	Sound Status Flags
	\ingroup grpMemBits
	\brief	Bits for REG_SNDSTAT (and REG_SOUNDCNT_X)
*/
/*!	\{	*/

#define SSTAT_SQR1 0x0001  //!< (R) Channel 1 status
#define SSTAT_SQR2 0x0002  //!< (R) Channel 2 status
#define SSTAT_WAVE 0x0004  //!< (R) Channel 3 status
#define SSTAT_NOISE 0x0008 //!< (R) Channel 4 status
#define SSTAT_DISABLE 0    //!< Disable sound
#define SSTAT_ENABLE                                                           \
    0x0080 //!< Enable sound. NOTE: enable before using any other sound regs

#define REG_SOUNDCNT_L                                                         \
    *(volatile u16*)0x4000080 //DMG sound control// #include "gba.h"

#define REG_SOUNDCNT_H *(volatile u16*)0x4000082 //Direct sound control
#define REG_SOUNDCNT_X *(volatile u16*)0x4000084 //Extended sound control
#define REG_DMA1SAD *(u32*)0x40000BC             //DMA1 Source Address
#define REG_DMA1DAD *(u32*)0x40000C0             //DMA1 Desination Address
#define REG_DMA1CNT_H *(u16*)0x40000C6           //DMA1 Control High Value
#define REG_TM1CNT_L *(u16*)0x4000104            //Timer 2 count value
#define REG_TM1CNT_H *(u16*)0x4000106            //Timer 2 control
#define REG_TM0CNT_L *(u16*)0x4000100            //Timer 0 count value
#define REG_TM0CNT_H *(u16*)0x4000102            //Timer 0 Control


//---------------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif
//---------------------------------------------------------------------------------


//---------------------------------------------------------------------------------
// SIO mode control bits used with REG_SIOCNT
//---------------------------------------------------------------------------------
#define SIO_8BIT		0x0000	// Normal 8-bit communication mode
#define SIO_32BIT		0x1000	// Normal 32-bit communication mode
#define SIO_MULTI		0x2000	// Multi-play communication mode
#define SIO_UART		0x3000	// UART communication mode
#define SIO_IRQ			0x4000	// Enable serial irq


//---------------------------------------------------------------------------------
// baud rate settings
//---------------------------------------------------------------------------------
#define SIO_9600		0x0000
#define SIO_38400		0x0001
#define SIO_57600		0x0002
#define SIO_115200		0x0003

#define SIO_CLK_INT		(1<<0)	// Select internal clock
#define SIO_2MHZ_CLK	(1<<1)	// Select 2MHz clock
#define SIO_RDY			(1<<2)	// Opponent SO state
#define SIO_SO_HIGH		(1<<3)	// Our SO state
#define SIO_START		(1<<7)


//---------------------------------------------------------------------------------
// SIO modes set with REG_RCNT
//---------------------------------------------------------------------------------
#define R_NORMAL		0x0000
#define R_MULTI			0x0000
#define R_UART			0x0000
#define R_GPIO			0x8000
#define R_JOYBUS		0xC000

//---------------------------------------------------------------------------------
// General purpose mode control bits used with REG_RCNT
//---------------------------------------------------------------------------------
#define	GPIO_SC			0x0001	// Data
#define	GPIO_SD			0x0002
#define	GPIO_SI			0x0004
#define	GPIO_SO			0x0008
#define	GPIO_SC_IO		0x0010	// Select I/O
#define	GPIO_SD_IO		0x0020
#define	GPIO_SI_IO		0x0040
#define	GPIO_SO_IO		0x0080
#define	GPIO_SC_INPUT	0x0000	// Input setting
#define GPIO_SD_INPUT	0x0000
#define	GPIO_SI_INPUT	0x0000
#define	GPIO_SO_INPUT	0x0000
#define	GPIO_SC_OUTPUT	0x0010	// Output setting
#define	GPIO_SD_OUTPUT	0x0020
#define	GPIO_SI_OUTPUT	0x0040
#define	GPIO_SO_OUTPUT	0x0080


#define REG_SIOCNT		*(volatile u16*)(REG_BASE + 0x128)	// Serial Communication Control
#define REG_SIODATA8	*(volatile u16*)(REG_BASE + 0x12a)	// 8bit Serial Communication Data
#define REG_SIODATA32	*(volatile u32*)(REG_BASE + 0x120)	// 32bit Serial Communication Data
#define REG_SIOMLT_SEND	*(volatile u16*)(REG_BASE + 0x12a)	// Multi-play SIO Send Data
#define REG_SIOMLT_RECV	*(volatile u16*)(REG_BASE + 0x120)	// Multi-play SIO Receive Data
#define REG_SIOMULTI0	*(volatile u16*)(REG_BASE + 0x120)	// Master data
#define REG_SIOMULTI1	*(volatile u16*)(REG_BASE + 0x122)	// Slave 1 data
#define REG_SIOMULTI2	*(volatile u16*)(REG_BASE + 0x124)	// Slave 2 data
#define REG_SIOMULTI3	*(volatile u16*)(REG_BASE + 0x126)	// Slave 3 data

#define REG_RCNT		*(volatile u16*)(REG_BASE + 0x134)	// SIO Mode Select/General Purpose Data

#define REG_HS_CTRL		*(volatile u16*)(REG_BASE + 0x140)	// SIO JOY Bus Control

#define REG_JOYRE		*(volatile u32*)(REG_BASE + 0x150)	// SIO JOY Bus Receive Data
#define REG_JOYRE_L		*(volatile u16*)(REG_BASE + 0x150)
#define REG_JOYRE_H		*(volatile u16*)(REG_BASE + 0x152)

#define REG_JOYTR		*(volatile u32*)(REG_BASE + 0x154)	// SIO JOY Bus Transmit Data
#define REG_JOYTR_L		*(volatile u16*)(REG_BASE + 0x154)
#define REG_JOYTR_H		*(volatile u16*)(REG_BASE + 0x156)

#define REG_JSTAT		*(volatile u16*)(REG_BASE + 0x158)	// SIO JOY Bus Receive Status


//---------------------------------------------------------------------------------
/*! \def SystemCall(Number)
    \brief helper macro to insert a bios call.
		\param Number swi number to call

		Inserts a swi of the correct format for arm or thumb code.
*/
#if	defined	( __thumb__ )
#define	SystemCall(Number)	 __asm ("SWI	  "#Number"\n" :::  "r0", "r1", "r2", "r3")
#else
#define	SystemCall(Number)	 __asm ("SWI	  "#Number"	<< 16\n" :::"r0", "r1", "r2", "r3")
#endif


enum LCDC_IRQ {
	LCDC_VBL_FLAG	=	(1<<0),
	LCDC_HBL_FLAG	=	(1<<1),
	LCDC_VCNT_FLAG	=	(1<<2),
	LCDC_VBL		=	(1<<3),
	LCDC_HBL		=	(1<<4),
	LCDC_VCNT		=	(1<<5)
};


/*! \var typedef void ( * IntFn)(void)
    \brief A type definition for an interrupt function pointer
*/
typedef void ( * IntFn)(void);

struct IntTable{IntFn handler; u32 mask;};

#define MAX_INTS	15

#define INT_VECTOR	*(IntFn *)(0x03007ffc)		// BIOS Interrupt vector
/*! \def REG_IME

    \brief Interrupt Master Enable Register.

	When bit 0 is clear, all interrupts are masked.  When it is 1,
	interrupts will occur if not masked out in REG_IE.

*/
#define	REG_IME		*(volatile u16 *)(REG_BASE + 0x208)	// Interrupt Master Enable
/*! \def REG_IE

    \brief Interrupt Enable Register.

	This is the activation mask for the internal interrupts.  Unless
	the corresponding bit is set, the IRQ will be masked out.
*/
#define	REG_IE		*(volatile u16 *)(REG_BASE + 0x200)	// Interrupt Enable
/*! \def REG_IF

    \brief Interrupt Flag Register.

	Since there is only one hardware interrupt vector, the IF register
	contains flags to indicate when a particular of interrupt has occured.
	To acknowledge processing interrupts, set IF to the value of the
	interrupt handled.

*/
#define	REG_IF		*(volatile u16 *)(REG_BASE + 0x202)	// Interrupt Request

//!  interrupt masks.
/*!
  These masks are used in conjuction with REG_IE to enable specific interrupts
  and with REG_IF to acknowledge interrupts have been serviced.
*/
typedef enum irqMASKS {
	IRQ_VBLANK	=	(1<<0),		/*!< vertical blank interrupt mask */
	IRQ_HBLANK	=	(1<<1),		/*!< horizontal blank interrupt mask */
	IRQ_VCOUNT	=	(1<<2),		/*!< vcount match interrupt mask */
	IRQ_TIMER0	=	(1<<3),		/*!< timer 0 interrupt mask */
	IRQ_TIMER1	=	(1<<4),		/*!< timer 1 interrupt mask */
	IRQ_TIMER2	=	(1<<5),		/*!< timer 2 interrupt mask */
	IRQ_TIMER3	=	(1<<6),		/*!< timer 3 interrupt mask */
	IRQ_SERIAL	=	(1<<7),		/*!< serial interrupt mask */
	IRQ_DMA0	=	(1<<8),		/*!< DMA 0 interrupt mask */
	IRQ_DMA1	=	(1<<9),		/*!< DMA 1 interrupt mask */
	IRQ_DMA2	=	(1<<10),	/*!< DMA 2 interrupt mask */
	IRQ_DMA3	=	(1<<11),	/*!< DMA 3 interrupt mask */
	IRQ_KEYPAD	=	(1<<12),	/*!< Keypad interrupt mask */
	IRQ_GAMEPAK	=	(1<<13)		/*!< horizontal blank interrupt mask */
} irqMASK;

extern struct IntTable IntrTable[];

/*! \fn void irqInit(void)
    \brief initialises the gba interrupt code.

*/
void InitInterrupt(void) __attribute__ ((deprecated));
void irqInit();

/*! \fn IntFn *irqSet(irqMASK mask, IntFn function)
    \brief sets the interrupt handler for a particular interrupt.

	\param mask
	\param function
*/
IntFn *SetInterrupt(irqMASK mask, IntFn function) __attribute__ ((deprecated));
IntFn *irqSet(irqMASK mask, IntFn function);
/*! \fn void irqEnable(int mask)
    \brief allows an interrupt to occur.

	\param mask
*/
void EnableInterrupt(irqMASK mask) __attribute__ ((deprecated));
void irqEnable(int mask);

/*! \fn void irqDisable(int mask)
    \brief prevents an interrupt occuring.

	\param mask
*/
void DisableInterrupt(irqMASK mask) __attribute__ ((deprecated));
void irqDisable(int mask);

void IntrMain();


//---------------------------------------------------------------------------------
// Reset Functions
//---------------------------------------------------------------------------------
/*! \enum RESTART_FLAG
	\brief flags for the SoftReset function
*/
typedef enum RESTART_FLAG {
	ROM_RESTART,	/*!< Restart from RAM entry point. */
	RAM_RESTART		/*!< restart from ROM entry point */
} RESTART_FLAG;


/*! \fn void SoftReset(RESTART_FLAG RestartFlag)
    \brief reset the GBA.
    \param RestartFlag flag
*/
void	SoftReset(RESTART_FLAG RestartFlag);

/*! \enum RESET_FLAG
	\brief flags controlling which parts of the system get reset
*/
enum RESET_FLAG {
	RESET_EWRAM		=	(1<<0),	/*!< Clear 256K on-board WRAM			*/
	RESET_IWRAM		=	(1<<1),	/*!< Clear 32K in-chip WRAM				*/
	RESET_PALETTE	=	(1<<2),	/*!< Clear Palette						*/
	RESET_VRAM		=	(1<<3),	/*!< Clear VRAM							*/
	RESET_OAM		=	(1<<4),	/*!< Clear OAM							*/
	RESET_SIO		=	(1<<5),	/*!< Switches to general purpose mode	*/
	RESET_SOUND		=	(1<<6),	/*!< Reset Sound registers				*/
	RESET_OTHER		=	(1<<7)	/*!< all other registers				*/
};

typedef enum RESET_FLAG RESET_FLAGS;

/*! \fn void RegisterRamReset(int ResetFlags)
    \brief reset the GBA registers and RAM.
    \param ResetFlags flags
*/
void RegisterRamReset(int ResetFlags);

//---------------------------------------------------------------------------------
// Interrupt functions
//---------------------------------------------------------------------------------

/*! \def static inline void Halt()
*/

static inline void Halt()	{ SystemCall(2); }
static inline void Stop()	{ SystemCall(3); }

//---------------------------------------------------------------------------------
static inline u32 BiosCheckSum() {
//---------------------------------------------------------------------------------
	u32 result;
	#if	defined	( __thumb__ )
		__asm ("SWI	0x0d\nmov %0,r0\n" :  "=r"(result) :: "r1", "r2", "r3");
	#else
		__asm ("SWI	0x0d<<16\nmov %0,r0\n" : "=r"(result) :: "r1", "r2", "r3");
	#endif
	return result;
}

//---------------------------------------------------------------------------------
// Math functions
//---------------------------------------------------------------------------------
/*! \fn s32 Div(s32 Number, s32 Divisor)
	\param Number
	\param Divisor
*/
s32	Div(s32 Number, s32 Divisor);
/*! \fn s32 DivMod(s32 Number, s32 Divisor)
	\param Number
	\param Divisor
*/
s32	DivMod(s32 Number, s32 Divisor);
/*! \fn u32 DivAbs(s32 Number, s32 Divisor)
	\param Number
	\param Divisor
*/
u32	DivAbs(s32 Number, s32 Divisor);
/*! \fn s32 DivArm(s32 Number, s32 Divisor)
	\param Number
	\param Divisor
*/
s32	DivArm(s32 Divisor, s32 Number);
/*! \fn s32 DivArmMod(s32 Number, s32 Divisor)
	\param Number
	\param Divisor
*/
s32	DivArmMod(s32 Divisor, s32 Number);
/*! \fn u32 DivArmAbs(s32 Number, s32 Divisor)
	\param Number
	\param Divisor
*/
u32	DivArmAbs(s32 Divisor, s32 Number);
/*! \fn u16 Sqrt(u32 X)
	\param X
*/
u16 Sqrt(u32 X);

/*! \fn s16 ArcTan(s16 Tan)
	\param Tan
*/
s16 ArcTan(s16 Tan);
/*! \fn s16 ArcTan2(s16 X, s16 Y)
	\param X
	\param Y
*/
u16 ArcTan2(s16 X, s16 Y);

/*! \fn CpuSet( const void *source, void *dest, u32 mode)
	\param source
	\param dest
	\param mode
*/
void CpuSet( const void *source, void *dest, u32 mode);
/*! \fn CpuFastSet( const void *source, void *dest, u32 mode)
	\param source
	\param dest
	\param mode
*/
void CpuFastSet( const void *source, void *dest, u32 mode);

/*! \fn void IntrWait(u32 ReturnFlag, u32 IntFlag)
    \brief waits for an interrupt to occur.
	\param ReturnFlag
	\param IntFlag
*/
void IntrWait(u32 ReturnFlag, u32 IntFlag);

/*! \fn void VBlankIntrWait()
    \brief waits for a vertical blank interrupt to occur.
*/
static inline
void VBlankIntrWait()	{ SystemCall(5); }


#define REG_KEYCNT		*(volatile u16*)(REG_BASE + 0x132)  // Key Control
/*! \enum KEYPAD_BITS

	\brief bit values for keypad buttons
*/
typedef enum KEYPAD_BITS {
	KEY_A		=	(1<<0),	/*!< keypad A button */
	KEY_B		=	(1<<1),	/*!< keypad B button */
	KEY_SELECT	=	(1<<2),	/*!< keypad SELECT button */
	KEY_START	=	(1<<3),	/*!< keypad START button */
	KEY_RIGHT	=	(1<<4),	/*!< dpad RIGHT */
	KEY_LEFT	=	(1<<5),	/*!< dpad LEFT */
	KEY_UP		=	(1<<6),	/*!< dpad UP */
	KEY_DOWN	=	(1<<7),	/*!< dpad DOWN */
	KEY_R		=	(1<<8),	/*!< Right shoulder button */
	KEY_L		=	(1<<9),	/*!< Left shoulder button */

	KEYIRQ_ENABLE	=	(1<<14),	/*!< Enable keypad interrupt */
	KEYIRQ_OR		=	(0<<15),	/*!< interrupt logical OR mode */
	KEYIRQ_AND		=	(1<<15),	/*!< interrupt logical AND mode */
	DPAD 		=	(KEY_UP | KEY_DOWN | KEY_LEFT | KEY_RIGHT) /*!< mask all dpad buttons */
} KEYPAD_BITS;


// --- RTC -------------------------------------------------------------
// For interfacing with the Seiko Instruments Inc. (SII) S-3511A real-time clock

#define S3511A_WR 0
#define S3511A_RD 1

#define S3511A_CMD(n) (0x60 | (n << 1))

#define S3511A_CMD_RESET    S3511A_CMD(0)
#define S3511A_CMD_STATUS   S3511A_CMD(1)
#define S3511A_CMD_DATETIME S3511A_CMD(2)
#define S3511A_CMD_TIME     S3511A_CMD(3)
#define S3511A_CMD_ALARM    S3511A_CMD(4)

#define S3511A_GPIO_PORT_DATA        (*(volatile u16 *)0x80000C4)
#define S3511A_GPIO_PORT_DIRECTION   (*(volatile u16 *)0x80000C6)
#define S3511A_GPIO_PORT_READ_ENABLE (*(volatile u16 *)0x80000C8)

#define S3511A_STATUS_INTFE  0x02 // frequency interrupt enable
#define S3511A_STATUS_INTME  0x08 // per-minute interrupt enable
#define S3511A_STATUS_INTAE  0x20 // alarm interrupt enable
#define S3511A_STATUS_24HOUR 0x40 // 0: 12-hour mode, 1: 24-hour mode
#define S3511A_STATUS_POWER  0x80 // power on or power failure occurred



#ifdef __cplusplus
}	   // extern "C"
#endif
