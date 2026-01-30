#include "../include/main.h"

static FILE *g_fpradd_log = NULL;

void set_fpradd_log(FILE *fp)
{
    g_fpradd_log = fp;
}

static inline void log_fpr_pair(fpr a, fpr b)
{
    if (g_fpradd_log == NULL) {
        return;
    }
    fprintf(g_fpradd_log, "%016llX %016llX\n",
            (unsigned long long)a, (unsigned long long)b);
}

static inline void log_fpr_pair_sub(fpr a, fpr b)
{
    if (g_fpradd_log == NULL) {
        return;
    }
    b ^= (fpr)0x8000000000000000ULL;
    fprintf(g_fpradd_log, "%016llX %016llX\n",
            (unsigned long long)a, (unsigned long long)b);
}

#define FPR_ADD_LOG(a, b) (log_fpr_pair((a), (b)), fpr_add((a), (b)))
#define FPR_SUB_LOG(a, b) (log_fpr_pair_sub((a), (b)), fpr_sub((a), (b)))

#define FPC_ADD(d_re, d_im, a_re, a_im, b_re, b_im)   do { \
        fpr fpct_re, fpct_im; \
        fpct_re = FPR_ADD_LOG(a_re, b_re); \
        fpct_im = FPR_ADD_LOG(a_im, b_im); \
        (d_re) = fpct_re; \
        (d_im) = fpct_im; \
    } while (0)

#define FPC_SUB(d_re, d_im, a_re, a_im, b_re, b_im)   do { \
        fpr fpct_re, fpct_im; \
        fpct_re = FPR_SUB_LOG(a_re, b_re); \
        fpct_im = FPR_SUB_LOG(a_im, b_im); \
        (d_re) = fpct_re; \
        (d_im) = fpct_im; \
    } while (0)

#define FPC_MUL(d_re, d_im, a_re, a_im, b_re, b_im)   do { \
        fpr fpct_a_re, fpct_a_im; \
        fpr fpct_b_re, fpct_b_im; \
        fpr fpct_d_re, fpct_d_im; \
        fpct_a_re = (a_re); \
        fpct_a_im = (a_im); \
        fpct_b_re = (b_re); \
        fpct_b_im = (b_im); \
        fpct_d_re = FPR_SUB_LOG( \
                             fpr_mul(fpct_a_re, fpct_b_re), \
                             fpr_mul(fpct_a_im, fpct_b_im)); \
        fpct_d_im = FPR_ADD_LOG( \
                             fpr_mul(fpct_a_re, fpct_b_im), \
                             fpr_mul(fpct_a_im, fpct_b_re)); \
        (d_re) = fpct_d_re; \
        (d_im) = fpct_d_im; \
    } while (0)


void fft_leak(fpr *f, unsigned logn)
{
    unsigned u;
    size_t t, n, hn, m;

    n = (size_t)1 << logn;
    hn = n >> 1;
    t = hn;
    for (u = 1, m = 2; u < logn; u ++, m <<= 1) {
        size_t ht, hm, i1, j1;

        ht = t >> 1;
        hm = m >> 1;
        for (i1 = 0, j1 = 0; i1 < hm; i1 ++, j1 += t) {
            size_t j, j2;

            j2 = j1 + ht;
            fpr s_re, s_im;

            s_re = fpr_gm_tab[((m + i1) << 1) + 0];
            s_im = fpr_gm_tab[((m + i1) << 1) + 1];
            for (j = j1; j < j2; j ++) {
                fpr x_re, x_im, y_re, y_im;

                x_re = f[j];
                x_im = f[j + hn];
                y_re = f[j + ht];
                y_im = f[j + ht + hn];
                FPC_MUL(y_re, y_im, y_re, y_im, s_re, s_im);
                FPC_ADD(f[j], f[j + hn],
                        x_re, x_im, y_re, y_im);
                FPC_SUB(f[j + ht], f[j + ht + hn],
                        x_re, x_im, y_re, y_im);
            }
        }
        t = ht;
    }
}
