/*
 * hpifru.c
 *
 * Author:  Bill Barkley & Andy Cress
 * Copyright (c) 2003-2004 Intel Corporation.
 *
 * 04/18/03 
 * 06/09/03 - new CustomField parsing, including SystemGUID
 * 02/19/04 ARCress - generalized BMC tag parsing, created IsTagBmc()
 * 05/05/04 ARCress - added OpenHPI Mgt Ctlr tag to IsTagBmc()
 * 09/22/04 ARCress - invbuf size bigger, check null ptr in fixstr
 * 10/14/04 ARCress - added HPI_B logic, incomplete
 */
#ifdef HPI_A
#include "hpifrua.c"
#else
#include "hpifrub.c"
#endif
/* end hpifru.c */
