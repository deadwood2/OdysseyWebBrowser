#ifndef COMPILERS_H
#define COMPILERS_H
/*
 * Compilers compatibility
 * -----------------------
 *
 * Version 1.1
 *
 * Currently supported compilers:
 *  - SAS/C (68k)
 *  - GCC (PPC. 68k is not supported yet)
 *
 * $Id: compilers.h,v 1.1 2012/05/04 21:14:10 bigfoot Exp $
 */

/*
 * Please use ONLY the following defines in your code:
 *
 * ASM
 * SAVEDS
 * __far (exception.. libjpeg forces us to do so)
 * __chip (exception as well)
 * STDARGS
 * __reg(__a0, LONG blah)
 *
 */


/*
 * GCC
 */
#ifdef __GNUC__

/* PPC (MorphOS) */
#ifdef __MORPHOS__

#ifndef ASM
#define ASM
#endif

#ifndef __far
#define __far
#endif

#ifndef SAVEDS
#define SAVEDS
#endif

#ifndef STDARGS
#define STDARGS
#endif

#ifndef __chip
#define __chip
#endif

#ifndef __reg
#define __reg(x,y) y
#endif

#else /* !__MORPHOS__ */

/* 68k */

#ifndef ASM
#define ASM
#endif

#ifndef __far
#define __far
#endif

#ifndef SAVEDS
#define SAVEDS
#endif

#ifndef STDARGS
#define STDARGS
#endif

#ifndef __chip
#define __chip
#endif

#ifndef __reg
#define __reg(x,y) y __asm__(#x)
#endif

#endif /* !__MORPHOS__ */

#endif /* __GNUC__ */


/*
 * SAS/C
 */
#ifdef __SASC

#ifndef ASM
#define ASM __asm
#endif

/* __far is already built in */

#ifndef SAVEDS
#define SAVEDS __saveds
#endif

#ifndef STDARGS
#define STDARGS __stdargs
#endif

#ifndef __reg
#define __reg(x,y) register __ ## x y
#endif

#endif /* __SASC */


#endif /* COMPILERS_H */
