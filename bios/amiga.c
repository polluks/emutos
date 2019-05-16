/*
 * amiga.c - Amiga specific functions
 *
 * Copyright (C) 2013-2017 The EmuTOS development team
 *
 * Authors:
 *  VRI   Vincent Rivière
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 */

/* #define ENABLE_KDEBUG */

#include "config.h"
#include "portab.h"
#include "kprint.h"
#include "amiga.h"
#include "vectors.h"
#include "tosvars.h"
#include "processor.h"
#include "gemerror.h"
#include "ikbd.h"               /* for call_mousevec() */
#include "screen.h"
#include "videl.h"
#include "delay.h"
#include "asm.h"
#include "string.h"
#include "disk.h"
#include "biosmem.h"
#include "bootparams.h"
#include "machine.h"

#if CONF_WITH_AROS
#include "aros.h"
#endif

extern long xmaddalt(UBYTE *start, long size); /* found in bdos/mem.h */

#ifdef MACHINE_AMIGA

/* Custom registers */
#define CUSTOM_BASE ((void *)0xdff000)
#define JOY0DAT *(volatile UWORD*)0xdff00a
#define JOY1DAT *(volatile UWORD*)0xdff00c
#define ADKCONR *(volatile UWORD*)0xdff010
#define POTGOR  *(volatile UWORD*)0xdff016 /* = POTINP */
#define SERDATR *(volatile UWORD*)0xdff018
#define DSKBYTR *(volatile UWORD*)0xdff01a
#define INTENAR *(volatile UWORD*)0xdff01c
#define INTREQR *(volatile UWORD*)0xdff01e
#define DSKPTH  *(void* volatile*)0xdff020
#define DSKLEN  *(volatile UWORD*)0xdff024
#define SERDAT  *(volatile UWORD*)0xdff030
#define SERPER  *(volatile UWORD*)0xdff032
#define POTGO   *(volatile UWORD*)0xdff034
#define DSKSYNC *(volatile UWORD*)0xdff07e
#define COP1LCH *(UWORD* volatile*)0xdff080
#define COPJMP1 *(volatile UWORD*)0xdff088
#define DIWSTRT *(volatile UWORD*)0xdff08e
#define DIWSTOP *(volatile UWORD*)0xdff090
#define DDFSTRT *(volatile UWORD*)0xdff092
#define DDFSTOP *(volatile UWORD*)0xdff094
#define DMACON  *(volatile UWORD*)0xdff096
#define INTENA  *(volatile UWORD*)0xdff09a
#define INTREQ  *(volatile UWORD*)0xdff09c
#define ADKCON  *(volatile UWORD*)0xdff09e
#define BPLCON0 *(volatile UWORD*)0xdff100
#define BPLCON1 *(volatile UWORD*)0xdff102
#define BPL1MOD *(volatile UWORD*)0xdff108
#define COLOR00 *(volatile UWORD*)0xdff180
#define COLOR01 *(volatile UWORD*)0xdff182

/* CIA A registers */
#define CIAAPRA    *(volatile UBYTE*)0xbfe001
#define CIAAPRB    *(volatile UBYTE*)0xbfe101
#define CIAADDRA   *(volatile UBYTE*)0xbfe201
#define CIAADDRB   *(volatile UBYTE*)0xbfe301
#define CIAATALO   *(volatile UBYTE*)0xbfe401
#define CIAATAHI   *(volatile UBYTE*)0xbfe501
#define CIAATBLO   *(volatile UBYTE*)0xbfe601
#define CIAATBHI   *(volatile UBYTE*)0xbfe701
#define CIAATODLO  *(volatile UBYTE*)0xbfe801
#define CIAATODMID *(volatile UBYTE*)0xbfe901
#define CIAATODHI  *(volatile UBYTE*)0xbfea01
#define CIAASDR    *(volatile UBYTE*)0xbfec01
#define CIAAICR    *(volatile UBYTE*)0xbfed01
#define CIAACRA    *(volatile UBYTE*)0xbfee01
#define CIAACRB    *(volatile UBYTE*)0xbfef01

/* CIA B registers */
#define CIABPRA    *(volatile UBYTE*)0xbfd000
#define CIABPRB    *(volatile UBYTE*)0xbfd100
#define CIABDDRA   *(volatile UBYTE*)0xbfd200
#define CIABDDRB   *(volatile UBYTE*)0xbfd300
#define CIABTALO   *(volatile UBYTE*)0xbfd400
#define CIABTAHI   *(volatile UBYTE*)0xbfd500
#define CIABTBLO   *(volatile UBYTE*)0xbfd600
#define CIABTBHI   *(volatile UBYTE*)0xbfd700
#define CIABTODLO  *(volatile UBYTE*)0xbfd800
#define CIABTODMID *(volatile UBYTE*)0xbfd900
#define CIABTODHI  *(volatile UBYTE*)0xbfda00
#define CIABSDR    *(volatile UBYTE*)0xbfdc00
#define CIABICR    *(volatile UBYTE*)0xbfdd00
#define CIABCRA    *(volatile UBYTE*)0xbfde00
#define CIABCRB    *(volatile UBYTE*)0xbfdf00

/* Gayle registers */
#define GAYLE_BASE ((void *)0xde1000)
#define GAYLE_ID *(volatile UBYTE*)0xde1000
#define FAT_GARY_TIMEOUT *(volatile UBYTE*)0xde0000

/* Generic Set/Clear bit */
#define SETBITS  (1U << 15)
#define CLRBITS  (0U << 15)

/* INTREQ / INTENA flags */
#define INTEN    (1U << 14)
#define EXTER    (1U << 13)
#define DSKSYN   (1U << 12)
#define RBF      (1U << 11)
#define AUD3     (1U << 10)
#define AUD2     (1U << 9)
#define AUD1     (1U << 8)
#define AUD0     (1U << 7)
#define BLIT     (1U << 6)
#define VERTB    (1U << 5)
#define PORTS    (1U << 3)
#define SOFT     (1U << 2)
#define DSKBLK   (1U << 1)
#define TBE      (1U << 0)

/* DMACON bits */
#define BBUSY    (1U << 14)
#define BZERO    (1U << 13)
#define BLTPRI   (1U << 10)
#define DMAEN    (1U << 9)
#define BPLEN    (1U << 8)
#define COPEN    (1U << 7)
#define BLTEN    (1U << 6)
#define SPREN    (1U << 5)
#define DSKEN    (1U << 4)
#define AUD3EN   (1U << 3)
#define AUD2EN   (1U << 2)
#define AUD1EN   (1U << 1)
#define AUD0EN   (1U << 0)

/* ADKCON bits */
#define PRECOMP1 (1U << 14)
#define PRECOMP0 (1U << 13)
#define MFMPREC  (1U << 12)
#define WORDSYNC (1U << 10)
#define MSBSYNC  (1U << 9)
#define FAST     (1U << 8)

/* DSKLEN bits */
#define DK_DMAEN (1U << 15)
#define DK_WRITE (1U << 14)

#if CONF_WITH_APOLLO_68080

/* Board_id register for Vampire machine detection */
#define VREG_BOARD          *(volatile UWORD*)0xdff3fc
#define VREG_BOARD_Unknown  0x00
#define VREG_BOARD_V600     0x01
#define VREG_BOARD_V500     0x02
#define VREG_BOARD_V4       0x03
#define VREG_BOARD_V666     0x04
#define VREG_BOARD_V4SA     0x05
#define VREG_BOARD_V1200    0x06
#define VREG_BOARD_V4000    0x07
#define VREG_BOARD_VCD32    0x08
#define VREG_BOARD_Future   0x09

#endif /* CONF_WITH_APOLLO_68080 */

/******************************************************************************/
/* Machine detection                                                          */
/******************************************************************************/

/* This deals with casts. No worry, this is inlined at compile time. */
static volatile UWORD *get_custom_register_mirror_address(volatile UWORD *preg, void *mirror_base)
{
    UBYTE* pbyte_base = (UBYTE *)CUSTOM_BASE;
    UBYTE* pbyte_reg = (UBYTE *)preg;
    ULONG offset = pbyte_reg - pbyte_base;
    UBYTE* pbyte_mirror_base = (UBYTE*)mirror_base;
    return (volatile UWORD *)(pbyte_mirror_base + offset);
}

/* Determine if an address range is a mirror of official custom chips */
static BOOL is_custom_chips_mirror(void *mirror_base)
{
    BOOL mirror = FALSE;
    volatile UWORD *pintena_mirror;
    UWORD save_intena;

    /* We will detect an eventual mirror through INTENA register */
    pintena_mirror = get_custom_register_mirror_address(&INTENA, mirror_base);

    /* Save interrupts */
    save_intena = INTENAR;

    /* Disable all interrupts using mirror */
    *pintena_mirror = ~SETBITS;
    if (INTENAR == 0)
    {
        /* All interrupts are disabled.
         * Either interrupts were previously disabled,
         * or this is a mirror of INTENA. */

        /* Enable TBE interrupt using mirror */
        *pintena_mirror = SETBITS | TBE;
        if (INTENAR == TBE)
        {
            /* Interrupt has been enabled. This is a mirror of INTENA. */
            mirror = TRUE;
            KDEBUG(("Custom chips mirror detected at %p.\n", mirror_base));
        }
    }

    /* Restore interrupts */
    INTENA = ~SETBITS;
    INTENA = SETBITS | save_intena;

    return mirror;
}

int has_gayle;

/* Detect A600 / A1200 Gayle chip.
 * Freely inspired from AROS ReadGayle().
 */
static void detect_gayle(void)
{
    UBYTE gayle_id;
    int i;

    has_gayle = 0;

    /* There may be a custom chips mirror instead of Gayle */
    if (is_custom_chips_mirror(GAYLE_BASE))
        return;

    /* On A300, we must clear the Fat Gary Timeout register
     * to avoid reading a bogus 0x80 Gayle ID */
    UNUSED(FAT_GARY_TIMEOUT);

    gayle_id = 0;
    GAYLE_ID = 0; /* Reset ID register */
    for (i = 0; i < 8; i++)
    {
        gayle_id <<= 1;
        gayle_id |= (GAYLE_ID & 0x80) ? 1 : 0;
    }

    if (gayle_id == 0 || gayle_id == 0xff)
    {
        /* Bogus ID */
        return;
    }

    /* Gayle ID should be 0xd0 on ECS, or 0xd1 on AGA */
    KDEBUG(("gayle_id = 0x%02x\n", gayle_id));

    has_gayle = 1;
}

void amiga_machine_detect(void)
{
    detect_gayle();
    KDEBUG(("has_gayle = %d\n", has_gayle));
}

const char *amiga_machine_name(void)
{
#if CONF_WITH_APOLLO_68080
    if (is_apollo_68080)
    {
        /* Detect Vampire core using Board_ID register */
        UBYTE boardid = HIBYTE(VREG_BOARD);
        switch(boardid)
        {
        case VREG_BOARD_V4SA:
            return "Vampire V4 - Standalone";
        case VREG_BOARD_V500:
            return "Amiga 500 with Vampire V2";
        case VREG_BOARD_V600:
            return "Amiga 600 with Vampire V2";
        case VREG_BOARD_V1200:
            return "Amiga 1200 with Vampire V1200";
        }
    }
#endif

    return "Amiga";
}

#if CONF_WITH_ALT_RAM

/******************************************************************************/
/* Alternate RAM                                                              */
/******************************************************************************/

static BOOL write_read_equals(void *start, ULONG testval)
{
    volatile ULONG *p = (volatile ULONG *)start;

    p[0] = testval; /* Write test value */
    p[1] = 0; /* Put something else on the data bus */

    /* Memory detection should always be performed with data cache disabled.
     * But just in case it is enabled, we do proper cache management here.
     * This also has a positive side effect with WinUAE JIT (see below). */
    flush_data_cache(start, sizeof(ULONG)*2);
    invalidate_data_cache(start, sizeof(ULONG)*2);

    /* Note for WinUAE: there is an issue with the JIT compiler.
     * http://eab.abime.net/showthread.php?t=97234
     * Even when JIT is enabled in the WinUAE settings, it is disabled by
     * default. It is actually enabled when the instruction cache is enabled
     * through CACR, which happens very early in EmuTOS initialization.
     * And code is only compiled after 5(?) calls.
     * Unfortunately, the simple algorithm used here is defeated by the JIT.
     * I guess that JIT assumes that when a value is written to some memory
     * address, then it can be cached in a register and reused when read back.
     * Fortunately, a single call to an external function is enough to defeat
     * that caching. So even if the above cache management is useless when the
     * data cache is disabled, those *calls* (whatever is called) are still
     * useful as "JIT barrier" to prevent data caching.
     * Typical testcase is A4000 + JIT + 1.5 MB of Slow RAM.
     * It is incorrectly detected as 1.75 MB without JIT barrier. */

    return p[0] == testval; /* Should be TRUE if there is RAM here */
}

static BOOL is_valid_ram(void *p)
{
    BOOL valid = FALSE;
    const ULONG testval = 0x55aa33cc;

    /* Test with 2 different patterns */
    if (write_read_equals(p, testval)
        && write_read_equals(p, ~testval))
    {
        valid = TRUE;
    }

    KDEBUG(("is_valid_ram(%p) == %d\n", p, valid));

    return valid;
}

/* Normal RAM detection */
ULONG amiga_detect_ram(void *start, void *end, ULONG step)
{
    UBYTE *pbyte_start = (UBYTE *)start;
    UBYTE *pbyte_end = (UBYTE *)end;
    UBYTE *p = pbyte_start;

    while (p < pbyte_end && is_valid_ram(p))
        p += step;

    return p - pbyte_start;
}

/* Reverse RAM detection, when actual RAM is located at the end of the range. */
static ULONG amiga_detect_ram_reverse(void *start, void *end, ULONG step)
{
    UBYTE *pbyte_start = (UBYTE *)start;
    UBYTE *pbyte_end = (UBYTE *)end;
    UBYTE *p = pbyte_end - step;

    while (p >= pbyte_start && is_valid_ram(p))
        p -= step;

    return pbyte_end - (p + step);
}

/* See Amiga memory map there:
 * https://www.amigacoding.com/index.php/Amiga_memory_map */

/* Detect Slow RAM, a.k.a A500 trapdoor RAM, a.k.a pseudo-fast RAM.
 * Max = 1.5 MB (standard)
 *   or 1.75 MB (requires Gary adapter, incompatible with Gayle IDE). */
static void add_slow_ram(void)
{
    UBYTE *start = (UBYTE *)0x00c00000;
    UBYTE *end;
    ULONG size;

    /* There may be a custom chips mirror instead of Slow RAM */
    if (is_custom_chips_mirror(start))
        return;

    if (has_gayle)
    {
        /* Slow RAM area is supposed to stop here */
        end = (UBYTE *)0x00d80000;
    }
    else
    {
        /* But if there is no Gayle IDE, Slow RAM may extend farther */
        end = (UBYTE *)0x00dc0000;
    }

    size = amiga_detect_ram(start, end, 256*1024UL);
    if (size == 0)
        return;

    KDEBUG(("Slow RAM detected at %p, size=%lu\n", start, size));
    xmaddalt(start, size);
}

/* Detect A3000/A4000 Processor Slot Fast RAM, a.k.a. Ramsey High MBRAM.
 * Max = 128 MB, theoretical max = 1904 MB? */
static void add_processor_slot_fast_ram(void)
{
    UBYTE *start = (UBYTE *)0x08000000;
    UBYTE *end = (UBYTE *)0x7f000000; /* Maximum positive address? */
    ULONG size;

    if (!IS_BUS32)
        return;

    size = amiga_detect_ram(start, end, 1*1024*1024UL);
    if (size == 0)
        return;

    KDEBUG(("Processor Slot Fast RAM detected at %p, size=%lu\n", start, size));
    xmaddalt(start, size);
}

/* Detect A3000/A4000 Motherboard Fast RAM, a.k.a. Ramsey Low MBRAM.
 * Max = 64 MB, theoretical max = 112 MB? */
static void add_motherboard_fast_ram(void)
{
    UBYTE *start = (UBYTE *)0x01000000; /* Start of 32-bit space */
    UBYTE *end = (UBYTE *)0x08000000;
    ULONG size;

    if (!IS_BUS32)
        return;

    /* This RAM is located at the end of the address range */
    size = amiga_detect_ram_reverse(start, end, 1*1024*1024UL);
    if (size == 0)
        return;

    KDEBUG(("Motherboard Fast RAM detected at %p, size=%lu\n", end - size, size));
    xmaddalt(end - size, size);
}

/* AUTOCONFIG is implemented lower */
static void add_expansion_ram(void);

void amiga_add_alt_ram(void)
{
#if EMUTOS_LIVES_IN_RAM
    int i;

    /* The list of Alt-RAM regions has been provided by the ramtos loader */
    for (i = 0; i < MAX_ALTRAM_REGIONS && altram_regions[i].address; i++)
    {
        UBYTE *address = altram_regions[i].address;
        ULONG size = altram_regions[i].size;
        KDEBUG(("xmaddalt(%p, %lu)\n", address, size));
        xmaddalt(address, size);
    }
#else
    /* Add the slowest RAM first to put it at the end of the Alt-RAM pool */
    add_slow_ram();
    add_processor_slot_fast_ram();
    add_motherboard_fast_ram();
#if CONF_WITH_AROS
    aros_add_alt_ram();
#else
    add_expansion_ram();
#endif
#endif /* EMUTOS_LIVES_IN_RAM */
}

#endif /* CONF_WITH_ALT_RAM */

/******************************************************************************/
/* Screen                                                                     */
/******************************************************************************/

UWORD amiga_screen_width;
UWORD amiga_screen_width_in_bytes;
UWORD amiga_screen_height;
const UBYTE *amiga_screenbase;
UWORD *copper_list;

ULONG amiga_initial_vram_size(void)
{
    return 640UL * 512 / 8;
}

static void amiga_set_videomode(UWORD width, UWORD height)
{
    UWORD lowres_height = height;
    UWORD bplcon0 = 0x1200; /* 1 bit-plane, COLOR ON */
    UWORD bpl1mod = 0; /* Modulo */
    UWORD ddfstrt = 0x0038; /* Data-fetch start for low resolution */
    UWORD ddfstop = 0x00d0; /* Data-fetch stop for low resolution */
    UWORD hstart = 0x81; /* Display window horizontal start */
    UWORD hstop = 0xc1; /* Display window horizontal stop */
    UWORD vstart; /* Display window vertical start */
    UWORD vstop; /* Display window vertical stop */
    UWORD diwstrt; /* Display window start */
    UWORD diwstop; /* Display window stop */

    amiga_screen_width = width;
    amiga_screen_width_in_bytes = width / 8;
    amiga_screen_height = height;

    if (width >= 640)
    {
        bplcon0 |= 0x8000; /* HIRES */
        ddfstrt = 0x003c;
        ddfstop = 0x00d4;
    }

    if (height >= 400)
    {
        bplcon0 |= 0x0004; /* LACE */
        bpl1mod = amiga_screen_width_in_bytes;
        lowres_height = height / 2;
    }

    vstart = 44 + (256 / 2) - (lowres_height / 2);
    vstop = vstart + lowres_height;
    vstop = (UBYTE)(((WORD)vstop) - 0x100); /* Normalize value */

    diwstrt = MAKE_UWORD(vstart, hstart);
    diwstop = MAKE_UWORD(vstop, hstop);

    KDEBUG(("BPLCON0 = 0x%04x\n", bplcon0));
    KDEBUG(("BPL1MOD = 0x%04x\n", bpl1mod));
    KDEBUG(("DDFSTRT = 0x%04x\n", ddfstrt));
    KDEBUG(("DDFSTOP = 0x%04x\n", ddfstop));
    KDEBUG(("DIWSTRT = 0x%04x\n", diwstrt));
    KDEBUG(("DIWSTOP = 0x%04x\n", diwstop));

    BPLCON0 = bplcon0; /* Bit Plane Control */
    BPLCON1 = 0;       /* Horizontal scroll value 0 */
    BPL1MOD = bpl1mod; /* Modulo = line width in interlaced mode */
    DDFSTRT = ddfstrt; /* Data-fetch start */
    DDFSTOP = ddfstop; /* Data-fetch stop */
    DIWSTRT = diwstrt; /* Set display window start */
    DIWSTOP = diwstop; /* Set display window stop */

    /* Set up color registers */
    COLOR00 = 0x0fff; /* Background color = white */
    COLOR01 = 0x0000; /* Foreground color = black */

    if (width == 640 && height == 400)
        sshiftmod = ST_HIGH;
    else
        sshiftmod = FALCON_REZ;
}

WORD amiga_check_moderez(WORD moderez)
{
    WORD current_mode, return_mode;

    if (moderez == 0xff02) /* ST High */
        moderez = VIDEL_COMPAT|VIDEL_1BPP|VIDEL_80COL|VIDEL_VERTICAL;

    if (moderez < 0)                /* ignore other ST video modes */
        return 0;

    current_mode = amiga_vgetmode();
    return_mode = moderez;          /* assume always valid */
    return (return_mode==current_mode)?0:return_mode;
}

void amiga_get_current_mode_info(UWORD *planes, UWORD *hz_rez, UWORD *vt_rez)
{
    *planes = 1;
    *hz_rez = amiga_screen_width;
    *vt_rez = amiga_screen_height;
}

void amiga_screen_init(void)
{
    amiga_set_videomode(640, 400);

    /* The VBL will update the Copper list below with any new value
     * of amiga_screenbase, eventually adjusted for interlace.
     * It is *mandatory* to reset BPL1PTH/BPL1PTL on each VBL.
     * It could have been done manually in the VBL interrupt handler, but in
     * that case the display would be wrong when interrupts are turned off.
     * On the other hand, with a Copper list, the bitplane pointer is always
     * reset correctly, even if the CPU interrupts are turned off.
     * The only remaining issue is that the interlaced fields are not
     * properly switched when interrupts are turned off. It is a minor issue,
     * as this should normally not happen for a long time. And even in that
     * case, displayed texts are still readable, even if a bit distorted.
     * The Copper list waits a few lines at the top to give the VBL interrupt
     * handler enough time to update it.
     */

    /* Set up the Copper list (must be in ST-RAM) */
    copper_list = (UWORD *)balloc_stram(sizeof(UWORD) * 8, FALSE);
    copper_list[0] = 0x0a01; /* Wait line 10 to give time to the VBL routine */
    copper_list[1] = 0xff00; /* Vertical wait only */
    copper_list[2] = 0x0e0; /* BPL1PTH */
    copper_list[3] = HIWORD(amiga_screenbase);
    copper_list[4] = 0x0e2; /* BPL1PTL */
    copper_list[5] = LOWORD(amiga_screenbase);
    copper_list[6] = 0xffff; /* End of      */
    copper_list[7] = 0xfffe; /* Copper list */

    /* Initialize the Copper */
    COP1LCH = copper_list;
    COPJMP1 = 0;

    /* Enable VBL interrupt */
    VEC_LEVEL3 = amiga_vbl;
    INTENA = SETBITS | INTEN | VERTB;

    /* Start the DMA, with bit plane and Copper */
    DMACON = SETBITS | COPEN | BPLEN | DMAEN;
}

void amiga_setphys(const UBYTE *addr)
{
    KDEBUG(("amiga_setphys(%p)\n", addr));
    amiga_screenbase = addr;
}

const UBYTE *amiga_physbase(void)
{
    return amiga_screenbase;
}

WORD amiga_setcolor(WORD colorNum, WORD color)
{
    KDEBUG(("amiga_setcolor(%d, 0x%04x)\n", colorNum, color));

    if (colorNum == 0)
        return 0x777;
    else
        return 0x000;
}

void amiga_setrez(WORD rez, WORD videlmode)
{
    UWORD width, height;

    /* Currently, we only support monochrome video modes */
    if ((videlmode & VIDEL_BPPMASK) != VIDEL_1BPP)
        return;

    width = (videlmode & VIDEL_80COL) ? 640 : 320;

    if (videlmode & VIDEL_VGA)
        height = (videlmode & VIDEL_VERTICAL) ? 240 : 480;
    else if (videlmode & VIDEL_PAL)
        height = (videlmode & VIDEL_VERTICAL) ? 512 : 256;
    else
        height = (videlmode & VIDEL_VERTICAL) ? 400 : 200;

    amiga_set_videomode(width, height);
}

WORD amiga_vgetmode(void)
{
    WORD mode = VIDEL_1BPP;

    if (amiga_screen_width >= 640)
        mode |= VIDEL_80COL;

    if (amiga_screen_height == 240)
        mode |= VIDEL_VGA | VIDEL_VERTICAL;
    else if (amiga_screen_height == 480)
        mode |= VIDEL_VGA;
    else if (amiga_screen_height == 512)
        mode |= VIDEL_PAL | VIDEL_VERTICAL;
    else if (amiga_screen_height == 256)
        mode |= VIDEL_PAL;
    else if (amiga_screen_height == 400)
        mode |= VIDEL_VERTICAL;

    if (amiga_screen_width == 640 && amiga_screen_height == 400)
        mode |= VIDEL_COMPAT;

    return mode;
}

/******************************************************************************/
/* IKBD                                                                       */
/* Documentation: https://www.kernel.org/doc/Documentation/input/atarikbd.txt */
/* FIXME: Algorithms implemented here are incomplete and approximative.       */
/******************************************************************************/

static BOOL mouse_events_disabled; /* Negative name due to lack of DATA segment */
static BOOL joysticks_events_disabled; /* Negative name due to lack of DATA segment */
static BOOL port0_joystick_mode;

void amiga_kbd_init(void)
{
    /* Set mouse/joystick button 1 as input */
    CIAADDRA &= ~(0x80 | 0x40);

    /* Set mouse/joystick button 2 as input */
    POTGO = 0xff00;
}

#if !CONF_SERIAL_IKBD

static void amiga_ikbd_simulate_writeb(UBYTE b)
{
    static UBYTE buffer[6];
    static UBYTE *p;
    static UWORD remaining;
    static UWORD load;

    KDEBUG(("amiga_ikbd_writeb 0x%02x\n", b));

    /* Special command MEMORY LOAD in progress */
    if (load > 0)
    {
        /* Not implemented: just skip data bytes */
        load--;

        if (load == 0)
            KDEBUG(("IKBD command MEMORY LOAD done.\n"));

        return;
    }

    /* As we have no DATA segment, initialize variables now */
    if (p == NULL)
        p = buffer;

    /* Store byte in command buffer */
    *p++ = b;

    if (remaining == 0)
    {
        /* First byte of command */
        switch (b)
        {
        case 0x80: /* RESET */
        case 0x07: /* SET MOUSE BUTTON ACTION */
        case 0x17: /* SET JOYSTICK MONITORING */
            remaining = 1;
        break;

        case 0x0a: /* SET MOUSE KEYCODE MOSE */
        case 0x0b: /* SET MOUSE THRESHOLD */
        case 0x0c: /* SET MOUSE SCALE */
        case 0x21: /* MEMORY READ */
        case 0x22: /* CONTROLLER EXECUTE */
            remaining = 2;
        break;

        case 0x20: /* MEMORY LOAD */
            remaining = 3;
        break;

        case 0x09: /* SET ABSOLUTE MOUSE POSITIONING */
            remaining = 4;
        break;

        case 0x0e: /* LOAD MOUSE POSITION */
            remaining = 5;
        break;

        case 0x19: /* SET JOYSTICK KEYCODE MODE */
        case 0x1b: /* TIME-OF-DAY CLOCK SET */
            remaining = 6;
        break;
        }
    }
    else
    {
        /* Additional byte of command */
        remaining--;

        /* Special command MEMORY LOAD */
        if (remaining == 0 && buffer[0] == 0x20)
        {
            UWORD adr = MAKE_UWORD(buffer[1], buffer[2]);
            UBYTE n = buffer[3];

            MAYBE_UNUSED(adr);
            KDEBUG(("IKBD command MEMORY LOAD %u bytes to 0x%04x...\n", n, adr));
            load = n;

            return;
        }
    }

    /* If the command requires additional bytes, wait for them */
    if (remaining > 0)
        return;

    KDEBUG(("IKBD command 0x%02x completely received\n", buffer[0]));

    /* Now the command is complete */
    p = buffer; /* For next command */

    /* Special command RESET */
    if (buffer[0] == 0x80 && buffer[1] == 0x01)
    {
        KDEBUG(("IKBD command RESET\n"));
        mouse_events_disabled = FALSE;
        joysticks_events_disabled = FALSE;
        port0_joystick_mode = FALSE;
        return;
    }

    /* Determine function of port 0 */
    switch (buffer[0])
    {
    /* Any mouse command disables joystick 0 */
    case 0x07: /* SET MOUSE BUTTON ACTION */
    case 0x08: /* SET RELATIVE MOUSE POSITION REPORTING */
    case 0x09: /* SET ABSOLUTE MOUSE POSITIONING */
    case 0x0a: /* SET MOUSE KEYCODE MODE */
    case 0x0b: /* SET MOUSE THRESHOLD */
    case 0x0c: /* SET MOUSE SCALE */
    case 0x0d: /* INTERROGATE MOUSE POSITION */
    case 0x0e: /* LOAD MOUSE POSITION */
    case 0x0f: /* SET Y=0 AT BOTTOM */
    case 0x10: /* SET Y=0 AT TOP */
    case 0x12: /* DISABLE MOUSE */
        port0_joystick_mode = FALSE;
    break;

    /* Any joystick command enables joystick 0 */
    case 0x14: /* SET JOYSTICK EVENT REPORTING */
    case 0x15: /* SET JOYSTICK INTERROGATION MODE */
    case 0x16: /* JOYSTICK INTERROGATE */
    case 0x17: /* SET JOYSTICK MONITORING */
    case 0x18: /* SET FIRE BUTTON MONITORING */
    case 0x19: /* SET JOYSTICK KEYCODE MODE */
    case 0x1a: /* DISABLE JOYSTICKS */
        port0_joystick_mode = TRUE;
    break;
    }

    /* Enable/Disable mouse and joysticks events */
    switch (buffer[0])
    {
    case 0x12: /* DISABLE MOUSE */
        mouse_events_disabled = TRUE;
    break;

    case 0x08: /* SET RELATIVE MOUSE POSITION REPORTING */
    case 0x09: /* SET ABSOLUTE MOUSE POSITIONING */
    case 0x0a: /* SET MOUSE KEYCODE MODE */
        mouse_events_disabled = FALSE;
    break;

    case 0x1a: /* DISABLE JOYSTICKS */
        joysticks_events_disabled = TRUE;
    break;

    case 0x14: /* SET JOYSTICK EVENT REPORTING */
    case 0x15: /* SET JOYSTICK INTERROGATION MODE */
    case 0x17: /* SET JOYSTICK MONITORING */
    case 0x18: /* SET FIRE BUTTON MONITORING */
    case 0x19: /* SET JOYSTICK KEYCODE MODE */
        joysticks_events_disabled = FALSE;
    break;
    }

    /* FIXME: Implement all commands */
}

#endif /* !CONF_SERIAL_IKBD */

void amiga_ikbd_writeb(UBYTE b)
{
#if CONF_SERIAL_IKBD
    amiga_rs232_writeb(b);
#else
    amiga_ikbd_simulate_writeb(b);
#endif
}

/******************************************************************************/
/* Keyboard                                                                   */
/******************************************************************************/

/* This scancode table will be used by amiga_int_ciaa_serial */
const UBYTE scancode_atari_from_amiga[128] =
{
    0x5b, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
    0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x29, 0x00, 0x70,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
    0x18, 0x19, 0x1a, 0x1b, 0x00, 0x6d, 0x6e, 0x6f,
    0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25,
    0x26, 0x27, 0x28, 0x2b, 0x00, 0x6a, 0x6b, 0x6c,
    0x60, 0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x31, 0x32,
    0x33, 0x34, 0x35, 0x00, 0x71, 0x67, 0x68, 0x69,
    0x39, 0x0e, 0x0f, 0x72, 0x1c, 0x01, 0x53, 0x00,
    0x00, 0x00, 0x4a, 0x00, 0x48, 0x50, 0x4d, 0x4b,
    0x3b, 0x3c, 0x3d, 0x3e, 0x3f, 0x40, 0x41, 0x42,
    0x43, 0x44, 0x63, 0x64, 0x65, 0x66, 0x4e, 0x62,
    0x2a, 0x36, 0x3a, 0x1d, 0x38, 0x38, 0x56, 0x57,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

/******************************************************************************/
/* Mouse                                                                      */
/******************************************************************************/

static UBYTE oldMouseX;
static UBYTE oldMouseY;
static BOOL oldButton1;
static BOOL oldButton2;
static BOOL oldMouseSet;

static void amiga_mouse_vbl(void)
{
    UWORD data;
    UBYTE mouseX, mouseY;
    BOOL button1, button2;

    if (mouse_events_disabled || port0_joystick_mode)
        return;

    data = JOY0DAT;
    mouseX = LOBYTE(data);
    mouseY = HIBYTE(data);
    button1 = (CIAAPRA & 0x40) == 0;
    button2 = (POTGOR & 0x0400) == 0;

    if (oldMouseSet &&
        (mouseX != oldMouseX
       || mouseY != oldMouseY
       || button1 != oldButton1
       || button2 != oldButton2))
    {
        UBYTE packet[3];
        packet[0] = 0xf8;

        if (button1)
            packet[0] |= 0x02;

        if (button2)
            packet[0] |= 0x01;

        packet[1] = mouseX - oldMouseX;
        packet[2] = mouseY - oldMouseY;

        call_mousevec(packet);
    }

    oldMouseX = mouseX;
    oldMouseY = mouseY;
    oldButton1 = button1;
    oldButton2 = button2;
    oldMouseSet = TRUE;
}

/******************************************************************************/
/* Joysticks                                                                  */
/******************************************************************************/

#define JOY_X0 0x0001
#define JOY_X1 0x0002
#define JOY_Y0 0x0100
#define JOY_Y1 0x0200

static UBYTE amiga_read_joystick(int joynum)
{
    UWORD dat;
    BOOL x0, x1, y0, y1;
    BOOL button1, right, left, down, up;
    UBYTE ikbdval;

    /* Read raw Amiga data */
    if (joynum == 1)
    {
        button1 = !(CIAAPRA & 0x80);
        dat = JOY1DAT;
    }
    else
    {
        button1 = !(CIAAPRA & 0x40);
        dat = JOY0DAT;
    }

    x0 = !!(dat & JOY_X0);
    x1 = !!(dat & JOY_X1);
    y0 = !!(dat & JOY_Y0);
    y1 = !!(dat & JOY_Y1);

    /* Interpret Amiga data */
    up = y1 != y0;
    left = y1;
    down = x1 != x0;
    right = x1;

    /* Convert to IKBD value */
    ikbdval =
        (button1 ? 0x80 : 0)
      | (right ? 0x08 : 0)
      | (left ? 0x04 : 0)
      | (down ? 0x02 : 0)
      | (up ? 0x01 : 0);

    return ikbdval;
}

static UBYTE oldJoy1;
static UBYTE oldJoy0;

static void amiga_joystick_vbl(void)
{
    UBYTE joy1, joy0;
    UBYTE packet[3];

    if (joysticks_events_disabled)
        return;

    joy1 = amiga_read_joystick(1);
    if (joy1 != oldJoy1)
    {
        KDEBUG(("joy1 = 0x%02x\n", joy1));
        packet[0] = 0xff;
        packet[1] = oldJoy0;
        packet[2] = joy1;
        call_joyvec(packet);
        oldJoy1 = joy1;
    }

    if (port0_joystick_mode)
    {
        joy0 = amiga_read_joystick(0);
        if (joy0 != oldJoy0)
        {
            KDEBUG(("joy0 = 0x%02x\n", joy0));
            packet[0] = 0xfe;
            packet[1] = joy0;
            packet[2] = oldJoy1;
            call_joyvec(packet);
            oldJoy0 = joy0;
        }
    }
}

/******************************************************************************/
/* Extra VBL                                                                  */
/******************************************************************************/

void amiga_extra_vbl(void)
{
    amiga_mouse_vbl();
    amiga_joystick_vbl();
}

/******************************************************************************/
/* Clock                                                                      */
/******************************************************************************/

#define BATTCLOCK ((volatile UBYTE*)0x00dc0000)

#define AMIGA_CLOCK_NONE 0
#define AMIGA_CLOCK_MSM6242B 1
#define AMIGA_CLOCK_RF5C01A 2

static int amiga_clock_type;

static UBYTE read_clock_reg(int reg)
{
    return BATTCLOCK[reg * 4 + 3] & 0x0f;
}

static void write_clock_reg(UBYTE reg, UBYTE val)
{
    BATTCLOCK[reg * 4 + 3] = val;
}

static UBYTE read_clock_bcd(int reg)
{
    return read_clock_reg(reg + 1) * 10 + read_clock_reg(reg);
}

void amiga_clock_init(void)
{
    KDEBUG(("d = %d, e = %d, f = %d\n", read_clock_reg(0xd), read_clock_reg(0xe), read_clock_reg(0xf)));

    if (read_clock_reg(0xf) == 4)
    {
        amiga_clock_type = AMIGA_CLOCK_MSM6242B;
    }
    else
    {
        UBYTE before, test, after;

        before = read_clock_reg(0xd);

        /* Write a value */
        test = before | 0x1;
        write_clock_reg(0xd, test);

        /* If the value is still here, there is a register */
        after = read_clock_reg(0xd);
        if (after == test)
            amiga_clock_type = AMIGA_CLOCK_RF5C01A;
        else
            amiga_clock_type = AMIGA_CLOCK_NONE;

        write_clock_reg(0xd, before);
    }

    KDEBUG(("amiga_clock_type = %d\n", amiga_clock_type));
}

static UWORD amiga_dogettime(void)
{
    UWORD seconds = read_clock_bcd(0);
    UWORD minutes = read_clock_bcd(2);
    UWORD hours = read_clock_bcd(4);
    UWORD time;

    KDEBUG(("amiga_dogettime() %02d:%02d:%02d\n", hours, minutes, seconds));

    time = (seconds >> 1)
         | (minutes << 5)
         | (hours << 11);

    return time;
}

static UWORD amiga_dogetdate(void)
{
    UWORD offset = (amiga_clock_type == AMIGA_CLOCK_RF5C01A) ? 1 : 0;
    UWORD days = read_clock_bcd(offset + 6);
    UWORD months = read_clock_bcd(offset + 8);
    UWORD years = read_clock_bcd(offset + 10);
    UWORD date;

    KDEBUG(("amiga_dogetdate() %02d/%02d/%02d\n", years, months, days));

    /* Y2K fix */
    if (years >= 78) /* AmigaOS epoch */
        years += 1900;
    else
        years += 2000;

    /* On A500+ with bad battery, the date is set to 1978/01/01 */

    if (years < 1980)
    {
        /* This date can't be represented in BDOS format. */
        return DEFAULT_DATETIME >> 16;
    }

    date = (days & 0x1F)
        | ((months & 0xF) << 5)
        | ((years - 1980) << 9);

    return date;
}

ULONG amiga_getdt(void)
{
    if (amiga_clock_type == AMIGA_CLOCK_NONE)
        return DEFAULT_DATETIME;

    return MAKE_ULONG(amiga_dogetdate(), amiga_dogettime());
}

#if CONF_WITH_UAE

/******************************************************************************/
/* uaelib: special functions provided by the UAE emulator                     */
/******************************************************************************/

/* Location of the UAE Boot ROM, a.k.a. RTAREA */
#define RTAREA_DEFAULT 0x00f00000
#define RTAREA_BACKUP  0x00ef0000

/* uaelib_demux() is the entry point */
#define UAELIB_DEMUX_OFFSET 0xFF60
uaelib_demux_t* uaelib_demux = NULL;

static ULONG uaelib_GetVersion(void);

#define IS_TRAP(x)(((*(ULONG*)(x)) & 0xf000ffff) == 0xa0004e75)

void amiga_uaelib_init(void)
{
    MAYBE_UNUSED(uaelib_GetVersion);

    if (IS_TRAP(RTAREA_DEFAULT + UAELIB_DEMUX_OFFSET))
        uaelib_demux = (uaelib_demux_t*)(RTAREA_DEFAULT + UAELIB_DEMUX_OFFSET);
    else if (IS_TRAP(RTAREA_BACKUP + UAELIB_DEMUX_OFFSET))
        uaelib_demux = (uaelib_demux_t*)(RTAREA_BACKUP + UAELIB_DEMUX_OFFSET);

#ifdef ENABLE_KDEBUG
    if (has_uaelib)
    {
        ULONG version = uaelib_GetVersion();
        KDEBUG(("EmuTOS running on UAE version %d.%d.%d\n",
            (int)((version & 0xff000000) >> 24),
            (int)((version & 0x00ff0000) >> 16),
            (int)(version & 0x0000ffff)));
    }
#endif
}

/* Get UAE version */
static ULONG uaelib_GetVersion(void)
{
    return uaelib_demux(0);
}

/* Write a string prefixed by DBG: to the UAE debug log
 * The final \n will automatically be appended */
static ULONG uaelib_DbgPuts(const char* str)
{
    return uaelib_demux(86, str);
}

/* Exit UAE */
static ULONG uaelib_ExitEmu(void)
{
    return uaelib_demux(13);
}

/******************************************************************************/
/* kprintf()                                                                  */
/******************************************************************************/

#define UAE_MAX_DEBUG_LENGTH 255

static char uae_debug_string[UAE_MAX_DEBUG_LENGTH + 1];

/* The only available output function is uaelib_DbgPuts(),
 * so we have to buffer the string until until \n */
void kprintf_outc_uae(int c)
{
    if (c == '\n')
    {
        /* Output the current string then clear the buffer */
        uaelib_DbgPuts(uae_debug_string);
        uae_debug_string[0] = '\0';
    }
    else
    {
        char* p;

        /* Append the character to the buffer */
        for (p = uae_debug_string; *p; ++p);
        if ((p - uae_debug_string) < UAE_MAX_DEBUG_LENGTH)
        {
            *p++ = (char)c;
            *p = '\0';
        }
    }
}

#endif /* CONF_WITH_UAE */

/******************************************************************************/
/* Shutdown                                                                   */
/******************************************************************************/

void amiga_shutdown(void)
{
#if CONF_WITH_UAE
    if (!has_uaelib)
        return;

    uaelib_ExitEmu();
#endif
}

BOOL amiga_can_shutdown(void)
{
#if CONF_WITH_UAE
    return has_uaelib;
#else
    return FALSE;
#endif
}

/******************************************************************************/
/* Floppy                                                                     */
/******************************************************************************/

/* Parts of this code were inspired by AROS trackdisk device */

static ULONG delay3ms;
static ULONG delay15ms;
static ULONG delay18ms;

#define DELAY_STEP() delay_loop(delay3ms)
#define DELAY_SETTLE() delay_loop(delay15ms)
#define DELAY_SET_STEP_DIRECTION() delay_loop(delay18ms)

#define TIMEOUT_MOTORON 500 /* milliseconds */
#define TIMEOUT_DSKBLK 1000 /* milliseconds */

#define SECTOR_SIZE 512 /* Size of a sector, in bytes */
#define MFM_TRACK_SIZE 13630UL /* Size of an MFM encoded track, in bytes */
#define MAGIC_MFM_SYNC_MARK 0x4489 /* MFM value for bit synchronization */
#define MAX_TRACKS 80 /* Typical value for most floppies */
#define MAX_SECTORS 11 /* Typical values on ST are 9, 10, or even 11 */

static WORD curdev; /* Currently selected floppy drive */
static WORD curtrack[2]; /* Current track, for each drive */
static WORD curside[2]; /* Current side, for each drive */
static UWORD *mfm_track; /* MFM-encoded track buffer */
static UBYTE sectors[MAX_SECTORS][SECTOR_SIZE]; /* Decoded sector data */
static BOOL sectors_decoded = FALSE; /* TRUE if sectors[][] is valid */
static UWORD crc_ccitt_table[256]; /* Precomputed CRC table */

static void make_crc_ccitt_table(void);

/* Initialize floppy driver */
void amiga_floppy_init(void)
{
    delay3ms = loopcount_1_msec * 3;
    delay15ms = loopcount_1_msec * 15;
    delay18ms = loopcount_1_msec * 18;

    /* The track buffer will be used by DMA so it requires Chip RAM */
    mfm_track = (UWORD *)balloc_stram(MFM_TRACK_SIZE, FALSE);
    KDEBUG(("mfm_track = %p, size = %lu\n", mfm_track, MFM_TRACK_SIZE));

    make_crc_ccitt_table(); /* Will be used to check track consistency */

    /* Set /RDY /TK0 /WPRO /CHNG pins as input */
    CIAADDRA &= ~(0x20 | 0x10 | 0x08 | 0x04);

    /* Set /MTR /SEL3 /SEL2 /SEL1 /SEL0 /SIDE DIR /STEP as output */
    CIABDDRB = 0xff; /* Set all pins as output */
    CIABPRB = 0xff; /* And disable all */

    /* Disable precompensation and MSBSYNC */
    ADKCON = CLRBITS | PRECOMP1 | PRECOMP0 | MSBSYNC;

    /* Enable MFM precompensation, DSKSYNC, MFM microseconds */
    ADKCON = SETBITS | MFMPREC | WORDSYNC | FAST;

    /* Synchronize DMA on this word */
    DSKSYNC = MAGIC_MFM_SYNC_MARK;

    /* Enable disk DMA */
    DMACON = SETBITS | DSKEN;
}

/* Select a single floppy drive for further operation */
static void amiga_floppy_select(WORD dev)
{
    if (dev != curdev)
    {
        /* Selecting other drive invalidates current track cache */
        sectors_decoded = FALSE;
    }

    /* Set Motor On flag. It will be taken in account on next selection. */
    CIABPRB &= ~0x80;

    /* Select the drive. This also turns motor on. */
    CIABPRB &= ~((dev == 0) ? 0x08 : 0x10);

    /* Remember the current drive */
    curdev = dev;
}

/* Deselect all floppy drives */
static void amiga_floppy_deselect(void)
{
    /* Deselect all drives */
    CIABPRB |= 0x08 | 0x10 | 0x20 | 0x40;

    /* Clear Motor On flag. It will be taken in account on next selection. */
    CIABPRB |= 0x80;

    /* Select the drive. This also turns motor off. */
    CIABPRB &= ~((curdev == 0) ? 0x08 : 0x10);

    /* Deselect all drives */
    CIABPRB |= 0x08 | 0x10 | 0x20 | 0x40;
}

/* Select a floppy side */
static void amiga_floppy_set_side(WORD side)
{
    if (side == 0)
        CIABPRB |= 0x04;
    else
        CIABPRB &= ~0x04;

    /* Remember the current side */
    curside[curdev] = side;
}

/* Determine if the head is on track 0 */
static BOOL amiga_floppy_is_track_zero(void)
{
    return !(CIAAPRA & 0x10);
}

/* Determine if the floppy has been removed or changed */
static BOOL amiga_floppy_has_disk_changed(void)
{
    return !(CIAAPRA & 0x04); /* DSKCHANGE */
}

/* Set step direction: 0 = forward, 1 = backward */
static void amiga_floppy_set_step_direction(WORD dir)
{
    WORD prevdir = (CIABPRB & 0x02);

    if (!prevdir && dir)
    {
        /* Backward */
        CIABPRB |= 0x02;
        DELAY_SET_STEP_DIRECTION();
    }
    else if (prevdir && !dir)
    {
        /* Forward */
        CIABPRB &= ~0x02;
        DELAY_SET_STEP_DIRECTION();
    }
}

/* Move the head to next track in current direction */
static void amiga_floppy_step(void)
{
    /* Pulse DSKSTEP bit to initiate step */
    CIABPRB &= ~0x01;
    CIABPRB |= 0x01;

    /* This delay is supposed to be enough for the step to succeed */
    DELAY_STEP();

    /* Adjust current track number depending on current step direction */
    curtrack[curdev] += (CIABPRB & 0x02) ? -1 : 1;
}

/* Recalibrate drive to resynchronize track counter */
static BOOL amiga_floppy_recalibrate(void)
{
    int steps = 0;

    KDEBUG(("amiga_floppy_recalibrate()\n"));

    /* Invalidate current track cache */
    sectors_decoded = FALSE;

    /* If the drive is already on track 0, step forward */
    if (amiga_floppy_is_track_zero())
    {
        amiga_floppy_set_step_direction(0);
        amiga_floppy_step();
    }

    /* Step backward until track 0 */
    amiga_floppy_set_step_direction(1);
    while (!amiga_floppy_is_track_zero())
    {
        amiga_floppy_step();
        steps++;

        if (steps >= MAX_TRACKS + 15)
        {
            /* Step does not work, there is no drive */
            return FALSE;
        }
    }

    /* We are on track 0 */
    curtrack[curdev] = 0;

    /* Wait for the head to stabilize */
    DELAY_SETTLE();

    /* The drive works fine */
    return TRUE;
}

/* Detect a floppy drive */
BOOL amiga_flop_detect_drive(WORD dev)
{
    BOOL ret;

    /* If recalibrate succeeds, we can assume that a drive is present */
    amiga_floppy_select(dev);
    ret = amiga_floppy_recalibrate();
    amiga_floppy_deselect();

    return ret;
}

/* Seek to an arbitrary track */
static WORD amiga_floppy_seek(WORD track)
{
    WORD offset = track - curtrack[curdev];

    if (offset == 0)
        return E_OK;

    amiga_floppy_set_step_direction(offset < 0);

    /* Step until expected track */
    while (curtrack[curdev] != track)
        amiga_floppy_step();

    /* Wait for the head to stabilize */
    DELAY_SETTLE();

    return E_OK;
}

/* Return TRUE if the motor timed out during switch on */
static BOOL timeout_motoron(void)
{
    LONG next = hz_200 + (TIMEOUT_MOTORON / 5);

    while (hz_200 < next)
    {
        if (!(CIAAPRA & 0x20)) /* Motor on? */
            return FALSE;
    }

    return TRUE;
}

/* Return TRUE if the disk DMA timed out */
static BOOL timeout_dskblk(void)
{
    LONG next = hz_200 + (TIMEOUT_DSKBLK / 5);

    while (hz_200 < next)
    {
        if (INTREQR & DSKBLK)
            return FALSE;
    }

    return TRUE;
}

/*
 * Read the current raw track into the MFM track cache
 * Output: mfm_track
 */
static WORD amiga_floppy_read_raw_track(void)
{
    UWORD dsklen;

    /* The motor should have been switched on when the drive was selected */
    if (timeout_motoron())
        return EDRVNR;

    KDEBUG(("DMA start...\n"));

    /* Set DMA address */
    DSKPTH = mfm_track;

    /* Start DMA read */
    dsklen = DK_DMAEN | (MFM_TRACK_SIZE / 2); /* Read + size in words */
    DSKLEN = dsklen;
    DSKLEN = dsklen; /* Twice for actual start */

    /* Wait for DSKBLK interrupt */
    if (timeout_dskblk())
    {
        KDEBUG(("error: DMA timed out\n"));
        DSKLEN = 0; /* Stop DMA */
        return EREADF;
    }

    /* Stop DMA */
    DSKLEN = 0;

    /* Clear DSKBLK interrupt */
    INTREQ = CLRBITS | DSKBLK;

    KDEBUG(("DMA end\n"));

    /* This is important if the data cache is activated in Chip RAM */
    invalidate_data_cache(mfm_track, MFM_TRACK_SIZE);

    return E_OK;
}

/*
 * Accurate documentation about CRC-CCITT can be found there:
 * http://jlgconsult.pagesperso-orange.fr/Atari/diskette/diskette_en.htm#FDC_CRC_Computation
 * http://www.atari-forum.com/viewtopic.php?p=9497#p9497
 */

/* Precompute a CRC-CCITT table */
static void make_crc_ccitt_table(void)
{
    UWORD w;
    int i, j;

    for (i = 0; i < 256; i++)
    {
        w = i << 8;

        for (j = 0; j < 8; j++)
            w = (w << 1) ^ ((w & 0x8000) ? 0x1021 : 0);

        crc_ccitt_table[i] = w;
    }
}

/* Get the CRC-CCITT of a buffer, with initial value */
static UWORD get_crc_ccitt_next(const void *buffer, UWORD length, UWORD crc)
{
    const UBYTE *p = (const UBYTE*)buffer;

    while (length-- > 0)
        crc = (crc << 8) ^ crc_ccitt_table[(crc >> 8) ^ *p++];

    return crc;
}

/* Get the CRC-CCITT of a buffer */
static UWORD get_crc_ccitt(const void *buffer, UWORD length)
{
    return get_crc_ccitt_next(buffer, length, 0xffff);
}

/*
 * Decode a single MFM byte, and increment the pointer.
 * Rules:
 * - each bit is encoded as 2 bits
 * - 1 is encoded as 01
 * - 0 is encoded as 10 if following a 0
 * - 0 is encoded as 00 if following a 1
 * Basically, first bit is a useless filler, second bit is the data bit.
 *
 * Documentation:
 * http://jlgconsult.pagesperso-orange.fr/Atari/diskette/diskette_en.htm#MFM_Address_Marks
 * https://en.wikipedia.org/wiki/Modified_Frequency_Modulation
 */
static UBYTE decode_mfm(const UWORD **ppmfm)
{
    UWORD mfm = *(*ppmfm)++; /* MFM encoded byte, as word */
    WORD counter; /* DBF counter */
    UBYTE out; /* Decoded byte */

    __asm__
    (
        "moveq   #7,%1\n"
        "1:\n\t"
        "add.w   %2,%2\n\t"
        "addx.w  %2,%2\n\t"
        "addx.b  %0,%0\n\t"
        "dbf     %1,1b"
    : "=&d"(out), "=&d"(counter)
    : "d"(mfm)
    :
    );

    return out;
}

/* Decode 2 MFM bytes as a single big endian word */
static UWORD decode_mfm_word(const UWORD **ppmfm)
{
    UBYTE hi = decode_mfm(ppmfm);
    UBYTE lo = decode_mfm(ppmfm);
    return MAKE_UWORD(hi, lo);
}

/*
 * Decode a whole MFM-encoded ST/PC track.
 * Input: mfm_track
 * Output: sectors, sectors_decoded
 *
 * Documentation:
 * http://jlgconsult.pagesperso-orange.fr/Atari/diskette/diskette_en.htm#Atari_Double_Density_Diskette_Format
 * http://bitsavers.trailing-edge.com/pdf/ibm/floppy/GA21-9182-4_Diskette_General_Information_Manual_Aug79.pdf
 */
static WORD amiga_floppy_decode_track(void)
{
    const UWORD *raw = mfm_track;
    const UWORD *rawend = (UWORD *)((ULONG)mfm_track + MFM_TRACK_SIZE);
    const int spt = 9; /* Sectors per track. FIXME: should be read from BPB */
    int sector = -1; /* Current sector */
    ULONG sectorbits = 0; /* Bit field for each sector read */
    UBYTE tmp[8]; /* Temporary buffer to compute CRC */
    UWORD datacrc; /* Partial CRC of Data Field */
#ifdef ENABLE_KDEBUG
    ULONG hz_start = hz_200;
#endif

    /* Pre-compute the CRC of Data Field header */
    tmp[0] = tmp[1] = tmp[2] = 0xa1; /* 3 sync bytes */
    tmp[3] = 0xfb; /* AM2 */
    datacrc = get_crc_ccitt(tmp, 4);

    /* While all sector data has not been read */
    while (sectorbits != (1 << spt) - 1)
    {
        UBYTE address_mark;
        UWORD crc, expected_crc;

        /* Find next sync mark */
        while (raw < rawend && *raw != MAGIC_MFM_SYNC_MARK)
            raw++;

        /* Skip multiple sync marks */
        while (raw < rawend && *raw == MAGIC_MFM_SYNC_MARK)
            raw++;

        if (raw >= rawend)
        {
            KDEBUG(("error: next sync mark not found\n"));
            return ESECNF;
        }

        /* Decode the address mark */
        address_mark = decode_mfm(&raw);

        if (address_mark == 0xfe) /* AM1 */
        {
            /* This is an ID Field (sector header) */
            UBYTE cylinder, head, size;

            /* Ensure that the ID Field has been fully read */
            if (raw >= rawend - 6)
            {
                KDEBUG(("error: truncated ID Field\n"));
                return ESECNF;
            }

            cylinder = decode_mfm(&raw);
            head = decode_mfm(&raw);
            sector = decode_mfm(&raw);
            size = decode_mfm(&raw);
            crc = decode_mfm_word(&raw);

            /* Compute the CRC of the ID Field */
            tmp[0] = tmp[1] = tmp[2] = 0xa1; /* 3 sync bytes */
            tmp[3] = address_mark;
            tmp[4] = cylinder;
            tmp[5] = head;
            tmp[6] = sector;
            tmp[7] = size;
            expected_crc = get_crc_ccitt(tmp, 8);

            /* Check ID Field integrity */
            if (crc != expected_crc)
            {
                KDEBUG(("error: sector %d: invalid header CRC\n", sector));
                return E_CRC;
            }

            /* Check sector metadata */
            if (cylinder != curtrack[curdev]
                || head != curside[curdev]
                || size != 2
                || sector < 1
                || sector > spt)
            {
                KDEBUG(("error: invalid header: cylinder=%d head=%d sector=%d size=%d\n",
                    cylinder, head, sector, size));
                return ESECNF;
            }
        }
        else if (address_mark == 0xfb) /* AM2 */
        {
            /* This is a Data Field (sector data) */
            UBYTE *data;
            int i;

            /* If no ID Field has been read, just ignore the Data Field.
             * It will be repeated at the end of the track, anyway. */
            if (sector < 0)
                continue;

            /* Ensure that the Data Field has been fully read */
            if (raw >= rawend - (SECTOR_SIZE + 2))
            {
                KDEBUG(("error: truncated Data Field\n"));
                return ESECNF;
            }

            /* Decode sector data */
            data = sectors[sector - 1];
            for (i = 0; i < SECTOR_SIZE; i++)
                data[i] = decode_mfm(&raw);
            crc = decode_mfm_word(&raw);

            /* Check data integrity */
            expected_crc = get_crc_ccitt_next(data, SECTOR_SIZE, datacrc);
            if (crc != expected_crc)
            {
                KDEBUG(("error: sector %d: invalid data CRC\n", sector));
                return E_CRC;
            }

            KDEBUG(("sector %d read OK\n", sector));

            /* Mark sector as read */
            sectorbits |= 1 << (sector - 1);
            sector = -1;
        }
        else
        {
            /* Unknown address mark */
            KDEBUG(("error: unknown address mark: 0x%02x\n", address_mark));
            return ESECNF;
        }
    }

    sectors_decoded = TRUE;

    KDEBUG(("track decode time = %lu ms\n", (hz_200 - hz_start) * 5));

    return E_OK;
}

/*
 * Read a whole track into track cache.
 * Input: mfm_track
 * Output: mfm_track, sectors, sectors_decoded
 */
static WORD amiga_floppy_read_track(WORD dev, WORD track, WORD side)
{
    WORD ret;

    KDEBUG(("amiga_floppy_read_track() dev=%d track=%d side=%d\n",
        dev, track, side));

    /* Invalidate current track cache */
    sectors_decoded = FALSE;

    /* Select side */
    amiga_floppy_set_side(side);

    /* Seek to requested track */
    KDEBUG(("amiga_floppy_seek(%d)...\n", track));
    ret = amiga_floppy_seek(track);
    KDEBUG(("amiga_floppy_seek(%d) ret=%d\n", track, ret));
    if (ret != E_OK)
        return ret;

    /* Read raw track data */
    KDEBUG(("amiga_floppy_read_raw_track()...\n"));
    ret = amiga_floppy_read_raw_track();
    KDEBUG(("amiga_floppy_read_raw_track() ret=%d\n", ret));
    if (ret != E_OK)
        return ret;

    /* Decode all sectors of the track */
    ret = amiga_floppy_decode_track();
    KDEBUG(("amiga_floppy_decode_track() ret=%d\n", ret));

    return ret;
}

/* Read some sectors from a track */
static WORD amiga_floppy_read(UBYTE *buf, WORD dev, WORD track, WORD side, WORD sect, WORD count)
{
    WORD ret;

    KDEBUG(("amiga_floppy_read() buf=%p dev=%d track=%d side=%d sect=%d count=%d\n",
        buf, dev, track, side, sect, count));

    /* Amiga hardware needs to read a whole track */
    if (sectors_decoded && dev == curdev && track == curtrack[curdev] && side == curside[curdev])
    {
        KDEBUG(("use cached track data dev=%d track=%d side=%d\n",
            dev, track, side));
    }
    else
    {
        ret = amiga_floppy_read_track(dev, track, side);
        if (ret != E_OK)
            return ret;
    }

    /* Copy sector data to user-supplied buffer */
    memcpy(buf, &sectors[sect - 1], SECTOR_SIZE * count);

    return E_OK;
}

/* Amiga implementation of floprw() */
WORD amiga_floprw(UBYTE *buf, WORD rw, WORD dev, WORD sect, WORD track, WORD side, WORD count)
{
    WORD ret;

    amiga_floppy_select(dev);

    if (amiga_floppy_has_disk_changed())
    {
        /* Recalibrate the drive */
        if (!amiga_floppy_recalibrate())
        {
            ret = EDRVNR;
            goto exit;
        }
    }

    if (rw & 1)
        ret = EWRPRO; /* Write not supported */
    else
        ret = amiga_floppy_read(buf, dev, track, side, sect, count);

exit:
    amiga_floppy_deselect();
    return ret;
}

/* Amiga implementation of flop_mediach() */
LONG amiga_flop_mediach(WORD dev)
{
    LONG ret;

    amiga_floppy_select(dev);
    ret = amiga_floppy_has_disk_changed() ? MEDIACHANGE : MEDIANOCHANGE;
    amiga_floppy_deselect();

    return ret;
}

/******************************************************************************/
/* RS-232                                                                     */
/******************************************************************************/

#define SERPER_8BIT 0x0000
#define SERPER_9BIT 0x8000

#define SERPER_DIVIDEND_NTSC 3579545UL
#define SERPER_DIVIDEND_PAL  3546895UL

#define SERPER_BAUD(baud) ((SERPER_DIVIDEND_PAL / (baud)) - 1)

#define SERDAT_TBE 0x2000 /* Transmit Buffer Empty */

void amiga_rs232_init(void)
{
#if CONF_SERIAL_IKBD
    SERPER = SERPER_8BIT | SERPER_BAUD(IKBD_BAUD);
    VEC_LEVEL5 = amiga_int_5;
    INTENA = SETBITS | RBF; /* Enable RBF interrupt */
#endif
}

BOOL amiga_rs232_can_write(void)
{
    return SERDATR & SERDAT_TBE;
}

void amiga_rs232_writeb(UBYTE b)
{
    while (!amiga_rs232_can_write())
    {
        /* Wait */
    }

    /* Send the byte */
    SERDAT = 0x0100 | b;
}

void amiga_rs232_rbf_interrupt(void)
{
#if CONF_SERIAL_IKBD
    UWORD serdat = SERDATR;
    UBYTE ikbdbyte = LOBYTE(serdat);
    call_ikbdraw(ikbdbyte);
#endif
}

/******************************************************************************/
/* AmigaOS-like types, defines and functions.                                 */
/* They are necessary for the AUTOCONFIG / WinUAE glue.                       */
/* Declarations come from the Commodore documentation,                        */
/* implementation is written by the EmuTOS development team.                  */
/******************************************************************************/

/* http://amigadev.elowar.com/read/ADCD_2.1/Includes_and_Autodocs_3._guide/node0654.html */
typedef void *APTR; /* Generic, untyped pointer */

/*
 * AmigaOS lists are doubly linked lists, with a few particularities.
 * - A Head pseudo-node is always present before the first data node.
 * - A Tail pseudo-node is always present after the first data node.
 * - If the list is empty, Head and Tail are linked together.
 * - Head and Tail pseudo-nodes are embedded in the List structure, and they
 *   partially overlap. In order to save space, Head.ln_Pred and Tail.ln_Succ
 *   occupy the same memory location, as both will always stay NULL.
 *   Because of that overlap, Head and Tail can't be formally represented in C.
 *   - Head is located at (struct Node *)&list->lh_Head
 *   - Tail is located at (struct Node *)&list->lh_Tail
 * - Names of List member are confusing:
 *   - List.lh_Head *overlaps* the Head pseudo-node (*not* a pointer to Head),
 *     so the value of List.lh_Head is actually the value of Head.ln_Succ,
 *     it other words it is a pointer to the first data node (or &Tail if empty).
 *   - List.lh_Tail *overlaps* the Tail pseudo-node (*not* a pointer to Tail),
 *     so the value of List.lh_Tail is actually the value of Tail.ln_Succ,
 *     which is, by definition, always NULL.
 *   - List.lh_TailPred is not confusing. As it overlaps Tail.ln_Pred,
 *     it is actually a pointer to the last data node (or &Head if empty).
 * - Because of those overlaps, care must be taken for GCC Strict Aliasing.
 * More information: https://wiki.amigaos.net/wiki/Exec_Lists_and_Queues
 */

/* http://amigadev.elowar.com/read/ADCD_2.1/Includes_and_Autodocs_3._guide/node062F.html */
struct Node MAY_ALIAS;
struct Node
{
    struct Node *ln_Succ; /* Successor, next data node, or &Tail if none */
    struct Node *ln_Pred; /* Predecessor, previous data node, or &Head if none */
    UBYTE       ln_Type;
    SBYTE       ln_Pri;
    char        *ln_Name;
};

/* http://amigadev.elowar.com/read/ADCD_2.1/Includes_and_Autodocs_3._guide/node0628.html */
struct List MAY_ALIAS;
struct List
{
    struct Node *lh_Head; /* ln_Succ of Head pseudo-node, pointer to first data node, or &Tail if empty */
    struct Node *lh_Tail; /* ln_Pred of Head pseudo-node, and also ln_Succ of Tail pseudo-node: always NULL */
    struct Node *lh_TailPred; /* ln_Pred of Tail pseudo-node, pointer to last data node, or &Head if empty */
    UBYTE       lh_Type;
    UBYTE       l_pad;
};

/* http://amigadev.elowar.com/read/ADCD_2.1/Includes_and_Autodocs_3._guide/node0047.html */
static void NewList(struct List *list)
{
    list->lh_Head = (struct Node *)&list->lh_Tail; /* Head.ln_Succ = &Tail; */
    list->lh_Tail = NULL; /* Head.ln_Pred = NULL; Tail.ln_Succ = NULL; */
    list->lh_TailPred = (struct Node *)&list->lh_Head; /* Tail.ln_Pred = &Head; */
}

/* http://amigadev.elowar.com/read/ADCD_2.1/Includes_and_Autodocs_3._guide/node01E1.html */
static void AddTail(struct List *list, struct Node *node)
{
    /* Fill the node */
    node->ln_Succ = (struct Node *)&list->lh_Tail; /* Tail pseudo-node */
    node->ln_Pred = list->lh_TailPred; /* Previous last node */

    /* Link previous last node to new node */
    list->lh_TailPred->ln_Succ = node;

    /* Link list to new last node */
    list->lh_TailPred = node;
}

/******************************************************************************/
/* AUTOCONFIG - Initialization of Zorro II/III expansion boards               */
/******************************************************************************/

/* http://amigadev.elowar.com/read/ADCD_2.1/Includes_and_Autodocs_3._guide/node05FE.html */
/* http://amigadev.elowar.com/read/ADCD_2.1/Hardware_Manual_guide/node02C8.html */

/* Bus base address */
#define E_EXPANSIONBASE   0x00e80000 /* Zorro II */
#define EZ3_EXPANSIONBASE 0xff000000 /* Zorro III */

/* Zorro II memory range */
#define E_MEMORYBASE 0x00200000
#define E_MEMORYSIZE 0x00800000 /* 8 MB */

/* Misc. defines */
#define E_SLOTSIZE 0x10000
#define EZ3_SIZEGRANULARITY 0x00080000

/* Each expansion board starts with ExpansionRom read-only data
 * followed by ExpansionControl write registers.
 * Special routines are required to read/write them on hardware.
 * As soon as the board is configured, those areas disappear.
 * This is why a copy of ExpansionRom is stored in the ConfigDev structure. */

struct ExpansionRom
{
    UBYTE er_Type;
    UBYTE er_Product;
    UBYTE er_Flags;
    UBYTE er_Reserved03;
    UWORD er_Manufacturer;
    ULONG er_SerialNumber;
    UWORD er_InitDiagVec;
    UBYTE er_Reserved0c;
    UBYTE er_Reserved0d;
    UBYTE er_Reserved0e;
    UBYTE er_Reserved0f;
};

struct ExpansionControl
{
    UBYTE ec_Interrupt;
    UBYTE ec_Z3_HighBase;
    UBYTE ec_BaseAddress;
    UBYTE ec_Shutup;
    UBYTE ec_Reserved14;
    UBYTE ec_Reserved15;
    UBYTE ec_Reserved16;
    UBYTE ec_Reserved17;
    UBYTE ec_Reserved18;
    UBYTE ec_Reserved19;
    UBYTE ec_Reserved1a;
    UBYTE ec_Reserved1b;
    UBYTE ec_Reserved1c;
    UBYTE ec_Reserved1d;
    UBYTE ec_Reserved1e;
    UBYTE ec_Reserved1f;
};

/* er_Type bits */
#define ERT_TYPEMASK 0xc0
#define ERT_NEWBOARD 0xc0
#define ERT_ZORROII  ERT_NEWBOARD
#define ERT_ZORROIII 0x80
#define ERTF_MEMLIST (1<<5) /* RAM board */
#define ERT_MEMMASK  0x07

/* er_Flags bits */
#define ERFF_NOSHUTUP (1<<6) /* Board can't be shut up */
#define ERFF_EXTENDED (1<<5) /* Interpret ERT_MEMMASK bits differently */
#define ERT_Z3_SSMASK 0x0f /* Zorro III board sub-size */

/* AmigaOS stores board information in ConfigDev structure. So we do. */
/* http://amigadev.elowar.com/read/ADCD_2.1/Includes_and_Autodocs_3._guide/node05F1.html */
struct ConfigDev MAY_ALIAS;
struct ConfigDev
{
    struct Node         cd_Node;
    UBYTE               cd_Flags;
    UBYTE               cd_Pad;
    struct ExpansionRom cd_Rom; /* Copy of hardware ROM data */
    APTR                cd_BoardAddr;
    ULONG               cd_BoardSize;
    UWORD               cd_SlotAddr;
    UWORD               cd_SlotSize;
    APTR                cd_Driver;
    struct ConfigDev    *cd_NextCD;
    ULONG               cd_Unused[4];
};

/* cd_Flags bits */
#define CDF_SHUTUP    0x01
#define CDF_CONFIGME  0x02
#define CDF_PROCESSED 0x08

/* Expansion ROM data is encoded using a very special scheme.
 * The 2 nybbles (4 bits) of each byte are stored independently.
 * Each nybble resides in the high bits of a WORD.
 * So each data byte actually occupies 2 WORDs (4 bytes)
 * - High nybble is located at data offset * 4
 * - Offset of the low nybble depends of the board type
 * http://amigadev.elowar.com/read/ADCD_2.1/Hardware_Manual_guide/node02C7.html
 */

static void get_nybble_offsets(APTR board, ULONG offset, volatile UBYTE **pphigh, volatile UBYTE **pplow)
{
    volatile UBYTE *p = (UBYTE *)board;
    ULONG low_nybble_offset;

    /* High (or Low) nybbles are always spaced out to 4 bytes */
    offset *= 4;

    /* Low nybble offset depends on bus type */
    if (IS_32BIT_POINTER(board))
        low_nybble_offset = 0x100; /* Zorro III */
    else
        low_nybble_offset = 0x002; /* Zorro II */

    /* Return pointers */
    *pphigh = &p[offset];
    *pplow = &p[offset + low_nybble_offset];
}

/* http://amigadev.elowar.com/read/ADCD_2.1/Includes_and_Autodocs_3._guide/node0268.html */
static UBYTE ReadExpansionByte(APTR board, ULONG offset)
{
    volatile UBYTE *phigh, *plow;
    UBYTE byte;

    get_nybble_offsets(board, offset, &phigh, &plow);

    /* Read low nybble */
    byte = (*plow & 0xf0) >> 4;

    /* Add high nybble */
    byte |= *phigh & 0xf0;

    return byte;
}

/* http://amigadev.elowar.com/read/ADCD_2.1/Includes_and_Autodocs_3._guide/node026D.html */
static void WriteExpansionByte(APTR board, ULONG offset, ULONG byte)
{
    volatile UBYTE *phigh, *plow;

    get_nybble_offsets(board, offset, &phigh, &plow);

    /* Write low nybble */
    *plow = byte << 4;

    /* Write high nybble */
    *phigh = byte;
}

/* Read the ExpansionRom structure from a board */
static void read_board_rom(APTR board, struct ExpansionRom *rom)
{
    ULONG offset;
    UBYTE *p = (UBYTE *)rom;

    for (offset = 0; offset < sizeof(struct ExpansionRom); offset++)
    {
        /* ROM bits are inverted, so NOT() them all */
        *p++ = ~ReadExpansionByte(board, offset);
    }

    /* Exception: this field was not inverted, so invert it again. */
    rom->er_Type = ~rom->er_Type;
}

/* http://amigadev.elowar.com/read/ADCD_2.1/Includes_and_Autodocs_3._guide/node0269.html */
static BOOL ReadExpansionRom(APTR board, struct ConfigDev *configDev)
{
    struct ExpansionRom *rom = &configDev->cd_Rom;
    UBYTE type;
    UBYTE mem;
    ULONG size;

    /* Read ExpansionRom structure from the board hardware */
    read_board_rom(board, rom);

    /* Sanity check on manufacturer */
    if (rom->er_Manufacturer == 0 || rom->er_Manufacturer == 0xffff)
        return FALSE;

    /* Sanity check on board type */
    type = rom->er_Type & ERT_TYPEMASK;
    if (!(type == ERT_ZORROII || type == ERT_ZORROIII))
        return FALSE;

    /* Determine board size */
    mem = rom->er_Type & ERT_MEMMASK;
    if (type == ERT_ZORROIII && (rom->er_Flags & ERFF_EXTENDED))
    {
        /* Extended size is interpreted differently */
        size = (16*1024*1024UL) << mem;
    }
    else
    {
        /* Standard size */
        if (mem == 0)
            size = 8*1024*1024UL;
        else
            size = (64*1024UL) << (mem - 1);
    }

    /* Store board size into ConfigDev */
    configDev->cd_BoardSize = size;

    KDEBUG(("*** ReadExpansionRom(): Found board at %p: Type=0x%02x Flags=0x%02x Manufacturer=0x%04x Product=0x%02x SerialNumber=0x%08lx InitDiagVec=0x%04x, size=0x%08lx\n",
        board, rom->er_Type, rom->er_Flags, rom->er_Manufacturer, rom->er_Product, rom->er_SerialNumber, rom->er_InitDiagVec, size));

    return TRUE;
}

/* http://amigadev.elowar.com/read/ADCD_2.1/Includes_and_Autodocs_3._guide/node025F.html */
static struct ConfigDev *AllocConfigDev(void)
{
    size_t size = sizeof(struct ConfigDev);
    struct ConfigDev *configDev;

    configDev = (struct ConfigDev *)balloc_stram(size, FALSE);
    if (!configDev)
        return NULL;

    bzero(configDev, size);

    return configDev;
}

/* http://amigadev.elowar.com/read/ADCD_2.1/Includes_and_Autodocs_3._guide/node0263.html */
static void FreeConfigDev(struct ConfigDev *configDev)
{
    // FIXME: bfree()?
}

/* List of all ConfigDev's found by AUTOCONFIG. */
static struct List boardList; /* Needs to be initialized! */

/* http://amigadev.elowar.com/read/ADCD_2.1/Includes_and_Autodocs_3._guide/node025D.html */
static void AddConfigDev(struct ConfigDev *configDev)
{
    KDEBUG(("AddConfigDev %p\n", configDev));
    AddTail(&boardList, (struct Node *)configDev);
}

/* http://aros.sourceforge.net/de/documentation/developers/autodocs/expansion.php#writeexpansionword
 * WriteExpansionWord() is used to configure Zorro III boards
 * by writing to ExpansionControl ec_Z3_HighBase and ec_BaseAddress.
 * It makes ExpansionRom/ExpansionControl disappear from "board" address,
 * then the real board appears at its final address.
 * Here, we add an extra parameter "configDev" specially for UAE hacks.
 * Normally, the AUTOCONFIG protocol maps Zorro III boards to 0x40000000.
 * But during WriteExpansionWord, UAE may forcibly remap the board to 0x10000000.
 * This happens with WinUAE when the option below is selected:
 * Settings > Hardware > RAM > Z3 mapping mode > UAE (0x10000000)
 * In that case, UAE expects to find the configDev pointer in A3 register.
 * This is why we need to implement this routine in assembly language.
 * UAE may override configDev->cd_BoardAddr and configDev->cd_SlotAddr
 * with the forced location.
 * See WinUAE source below for details, in function expamemz3_map():
 * https://github.com/tonioni/WinUAE/blob/master/expansion.cpp */
static void WriteExpansionWord_UAE(APTR board, ULONG offset, ULONG word, struct ConfigDev *configDev)
{
    register struct ConfigDev *regConfigDev __asm__("a3") = configDev; /* configDev must be in A3 for UAE */
    UBYTE *adr = (UBYTE *)board + (offset * 4); /* Registers are spaced out to 4 bytes */

    KDEBUG(("Before WriteExpansionWord_UAE() cd_BoardAddr=%p cd_SlotAddr=0x%04x\n",
        configDev->cd_BoardAddr, configDev->cd_SlotAddr));

    __asm__ volatile
    (
        "move.b  %2,4(%1)\n\t"  /* Write Low byte in next register */
        "move.w  %2,(%1)"       /* Write High and Low bytes as single WORD */
    : /* outputs */
    : "a"(regConfigDev), "a"(adr), "d"(word) /* inputs */
    : CLOBBER_MEMORY /* clobbered */
    );

    KDEBUG(("After  WriteExpansionWord_UAE() cd_BoardAddr=%p cd_SlotAddr=0x%04x\n",
        configDev->cd_BoardAddr, configDev->cd_SlotAddr));
}

/* Auto-configure all Zorro II/III expansion boards.
 * This implements the AUTOCONFIG protocol.
 * https://wiki.amigaos.net/wiki/Expansion_Library
 * For each board, a ConfigDev structure is allocated and kept in boardList.
 * That list will be scanned later to add actual RAM to the OS pool.
 */
void amiga_autoconfig(void)
{
    KDEBUG(("**************** AUTOCONFIG !!!! ****************\n"));
    KDEBUG(("IS_BUS32=%d\n", IS_BUS32));

    /* ConfigDev's are supposed to be chained, so we do. */
    NewList(&boardList);

    for(;;)
    {
        APTR base; /* Bus base address, either Zorro II or Zorro III */
        struct ConfigDev *configDev = AllocConfigDev();
        BOOL found = FALSE;

        /* First, look for the board on Zorro III bus. */
        if (IS_BUS32)
        {
            base = (APTR)EZ3_EXPANSIONBASE;
            found = ReadExpansionRom(base, configDev);
        }

        /* If not found, look for the board on Zorro II bus. */
        if (!found)
        {
            base = (APTR)E_EXPANSIONBASE;
            found = ReadExpansionRom(base, configDev);
        }

        /* If still not found, there are no more boards. */
        if (!found)
        {
            FreeConfigDev(configDev);
            break;
        }

        /* Board found. Configure it. */
        if (config_board(base, configDev))
            AddConfigDev(configDev);
        else
            FreeConfigDev(configDev);
    }

    KDEBUG(("**************** AUTOCONFIG DONE ****************\n"));
}

/******************************************************************************/
/* Expansion RAM                                                              */
/******************************************************************************/

/* Detect the amount of RAM present on a board */
static ULONG detect_board_ram_size(struct ConfigDev *configDev)
{
    UBYTE subsize = configDev->cd_Rom.er_Flags & ERT_Z3_SSMASK;

    if (!IS_32BIT_POINTER(configDev->cd_BoardAddr) || subsize == 0)
    {
        /* RAM is available in the whole range */
        return configDev->cd_BoardSize;
    }
    else if (subsize == 1)
    {
        /* Probe the whole range for actual RAM */
        UBYTE *start = (UBYTE *)configDev->cd_BoardAddr;
        ULONG maxsize = configDev->cd_BoardSize;
        return amiga_detect_ram(start, start + maxsize, EZ3_SIZEGRANULARITY);
    }
    else if (subsize <= 8)
    {
        /* subsize == 2 means 64 KB, next ones double the size */
        return (64*1024UL) << (subsize - 2);
    }
    else if (subsize <= 13)
    {
        /* subsize == 9 means 6 MB, next ones add 2 MB */
        return (6*1024*1024UL) + ((2*1024*1024UL) * (subsize - 9));
    }
    else
    {
        /* Invalid subsize */
        return 0;
    }
}

/* Detect RAM from a single board, and if found, add it to the OS */
static void add_ram_from_board(struct ConfigDev *configDev)
{
    APTR start;
    ULONG size;

    /* Consider only RAM boards */
    if (!(configDev->cd_Rom.er_Type & ERTF_MEMLIST))
        return;

    /* Skip already processed boards */
    if (configDev->cd_Flags & (CDF_SHUTUP | CDF_PROCESSED))
        return;

    /* Determine RAM address range */
    start = configDev->cd_BoardAddr;
    size = detect_board_ram_size(configDev);

    /* Sanity check */
    if (size == 0 || size > configDev->cd_BoardSize)
    {
        KDEBUG(("*** Skip RAM board due to invalid size: configDev=%p cd_BoardAddr=%p cd_BoardSize=%lu subsize=%d size=%lu\n",
            configDev, configDev->cd_BoardAddr, configDev->cd_BoardSize,
            configDev->cd_Rom.er_Flags & ERT_Z3_SSMASK, size));
        return;
    }

    KDEBUG(("*** Expansion RAM found: configDev=%p cd_BoardAddr=%p cd_BoardSize=%lu subsize=%d size=%lu\n",
        configDev, configDev->cd_BoardAddr, configDev->cd_BoardSize,
        configDev->cd_Rom.er_Flags & ERT_Z3_SSMASK, size));

    /* Register this Alt-RAM to the OS */
    xmaddalt(start, size);

    /* This board has been processed */
    configDev->cd_Flags |= CDF_PROCESSED;
}

/* Look for RAM on Expansion boards. This must be done after AUTOCONFIG. */
static void add_expansion_ram(void)
{
    struct Node *node;

    /* Scan the list of Expansion boards */
    for (node = boardList.lh_Head; node->ln_Succ; node = node->ln_Succ)
    {
        struct ConfigDev *configDev = (struct ConfigDev *)node;
        add_ram_from_board(configDev);
    }
}

#endif /* MACHINE_AMIGA */
