#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "pqclean/crypto_sign/falcon-512/clean/api.h"
#include "../include/main.h"

typedef uint64_t fpr;


#define TEMPALLOC
#define NONCELEN   40

int randombytes(unsigned char *x, unsigned long long xlen)
{
    unsigned long long i;
    for(i = 0 ; i < xlen ; i++)
    {
        x[i] = rand() & 0xff;
    }
    return 0;
}
#define MKN(logn)   ((size_t)1 << (logn))

static void
smallints_to_fpr(fpr *r, const int8_t *t, unsigned logn)
{
	size_t n, u;

	n = MKN(logn);
	for (u = 0; u < n; u ++) {
		r[u] = fpr_of(t[u]);
	}
}

void make_input(const size_t testN,const char* path_fprof, const char* path_fpradd)
{
	TEMPALLOC union {
		uint8_t b[FALCON_KEYGEN_TEMP_9];
		uint64_t dummy_u64;
		fpr dummy_fpr;
	} tmp;
	TEMPALLOC int8_t f[512], g[512], F[512], G[512];
	TEMPALLOC uint16_t h[512];
	TEMPALLOC unsigned char seed[48];
	TEMPALLOC inner_shake256_context rng;
    fpr b01[512];

    FILE *fp_of,*fp_add;
    fp_of = fopen(path_fprof,"w");
    fp_add = fopen(path_fpradd,"w");
    if (fp_of == NULL || fp_add == NULL) {
        fprintf(stderr, "error: failed to open output files: %s , %s\n",
                path_fprof, path_fpradd);
        if (fp_of) fclose(fp_of);
        if (fp_add) fclose(fp_add);
        return;
    }
    set_fpradd_log(fp_add);
    fprintf(stderr, "[make_input] N=%zu, writing to %s and %s\n", testN, path_fprof, path_fpradd);
    for(size_t N_test =  0 ; N_test < testN ; N_test++)
    {
        if (N_test % 1000 == 0) {
            fprintf(stderr, "[make_input] %zu/%zu\n", N_test, testN);
        }
        randombytes(seed, sizeof seed);
        inner_shake256_init(&rng);
        inner_shake256_inject(&rng, seed, sizeof seed);
        inner_shake256_flip(&rng);
        PQCLEAN_FALCON512_CLEAN_keygen(&rng, f, g, F, NULL, h, 9, tmp.b);
        if (!PQCLEAN_FALCON512_CLEAN_complete_private(G, f, g, F, 9, tmp.b)) {
            puts("error make G");
            return ;
        }
        for(size_t i = 0 ; i < 512 ; i++)
        {
            fprintf(fp_of,"%d ",f[i]);
        }
        fprintf(fp_of,"\n");
        smallints_to_fpr(b01,f,9);
        fft_leak(b01,9);
    }
    fclose(fp_of);
    fclose(fp_add);
    fprintf(stderr, "[make_input] done\n");
}

// argv[1] = testN
// argv[2] = path of inputdata fprof
// argv[3] = path of inputdata fpradd
// argv[4] = seed of rng
int main(int argc, char **argv)
{
    if (argc < 4) {
        fprintf(stderr, "usage: %s <testN> <path_fprof> <path_fpradd>\n", argv[0]);
        return 1;
    }
    size_t testN = (size_t)strtoull(argv[1], NULL, 10);
    const char *path_fprof = argv[2];
    const char *path_fpradd = argv[3];
    make_input(testN, path_fprof, path_fpradd);
    return 0;
}
