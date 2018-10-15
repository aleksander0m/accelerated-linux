/*
 * uncompress.h 
 *
 * Copyright (C) 2014, Greg Ungerer <greg.ungerer@accelerated.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#ifndef _ARCH_UNCOMPRESS_H_
#define _ARCH_UNCOMPRESS_H_

#ifdef CONFIG_DEBUG_UNCOMPRESS
extern void putc(int c);
#else
static inline void putc(int c) {}
#endif
static inline void flush(void) {}
static inline void arch_decomp_setup(void) {}

#if defined(CONFIG_MACH_8300) || defined(CONFIG_MACH_6300CX) || \
    defined(CONFIG_MACH_6300LX) || defined(CONFIG_MACH_6330MX) || \
    defined(CONFIG_MACH_6350SR)
#define ARCH_HAS_DECOMP_WDOG
static inline void arch_decomp_wdog(void)
{
	*((volatile u32 *)(0xd0018140)) ^= 0x00400000;
}
#endif /* CONFIG_MACH_8300 || CONFIG_MACH_6300CX || CONFIG_MACH_6300LX */

#if defined(CONFIG_MACH_ARMADA_38X)
#define ARCH_HAS_DECOMP_WDOG
static inline void arch_decomp_wdog(void)
{
	*((volatile u32 *)(0xf1018140)) ^= 0x00200000;
}
#endif /* CONFIG_MACH_ARMADA_38X */

#if defined(CONFIG_MACH_5400_RM_DT)
#define ARCH_HAS_DECOMP_WDOG
static inline void arch_decomp_wdog(void)
{
	*((volatile u32 *)(0xf1010140)) ^= 0x00000010;
}
#endif /* CONFIG_MACH_5400_RM_DT */

#ifndef ARCH_HAS_DECOMP_WDOG
static inline void arch_decomp_wdog(void) {}
#endif

#endif /* _ARCH_UNCOMPRESS_H_ */