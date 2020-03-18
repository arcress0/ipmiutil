/*
 * ifru.h
 * common routines from ifru.c 
 */
#ifndef uchar
#define uchar unsigned char
#endif

int load_fru(uchar sa, uchar frudev, uchar frutype, uchar **pfrubuf);
int show_fru(uchar sa, uchar frudev, uchar frutype, uchar *pfrubuf);
void free_fru(uchar *pfrubuf);

/* end ifru.h */
