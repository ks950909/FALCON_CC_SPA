#ifndef __MAIN_H__
#define __MAIN_H__

#include <stdio.h>
#include "pqclean/crypto_sign/falcon-512/clean/inner.h"

// .c


// leak.c
void fft_leak(fpr *f, unsigned logn);
void set_fpradd_log(FILE *fp);

#endif //__MAIN_H__
