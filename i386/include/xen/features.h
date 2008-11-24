/******************************************************************************
 * features.h
 *
 * Query the features reported by Xen.
 *
 * Copyright (c) 2006, Ian Campbell
 *
 * $FreeBSD: src/sys/i386/include/xen/features.h,v 1.1 2008/08/12 19:41:11 kmacy Exp $
 */

#ifndef __ASM_XEN_FEATURES_H__
#define __ASM_XEN_FEATURES_H__

#include <xen/interface/version.h>

extern void setup_xen_features(void);

extern uint8_t xen_features[XENFEAT_NR_SUBMAPS * 32];

#define xen_feature(flag)	(xen_features[flag])

#endif /* __ASM_XEN_FEATURES_H__ */
