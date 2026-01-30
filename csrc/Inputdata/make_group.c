#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
typedef uint64_t fpr;

#define FPR_NORM64(m, e)   do { \
        uint32_t nt; \
        \
        (e) -= 63; \
        \
        nt = (uint32_t)((m) >> 32); \
        nt = (nt | -nt) >> 31; \
        (m) ^= ((m) ^ ((m) << 32)) & ((uint64_t)nt - 1); \
        (e) += (int)(nt << 5); \
        \
        nt = (uint32_t)((m) >> 48); \
        nt = (nt | -nt) >> 31; \
        (m) ^= ((m) ^ ((m) << 16)) & ((uint64_t)nt - 1); \
        (e) += (int)(nt << 4); \
        \
        nt = (uint32_t)((m) >> 56); \
        nt = (nt | -nt) >> 31; \
        (m) ^= ((m) ^ ((m) <<  8)) & ((uint64_t)nt - 1); \
        (e) += (int)(nt << 3); \
        \
        nt = (uint32_t)((m) >> 60); \
        nt = (nt | -nt) >> 31; \
        (m) ^= ((m) ^ ((m) <<  4)) & ((uint64_t)nt - 1); \
        (e) += (int)(nt << 2); \
        \
        nt = (uint32_t)((m) >> 62); \
        nt = (nt | -nt) >> 31; \
        (m) ^= ((m) ^ ((m) <<  2)) & ((uint64_t)nt - 1); \
        (e) += (int)(nt << 1); \
        \
        nt = (uint32_t)((m) >> 63); \
        (m) ^= ((m) ^ ((m) <<  1)) & ((uint64_t)nt - 1); \
        (e) += (int)(nt); \
    } while (0)

static inline fpr
FPR(int s, int e, uint64_t m) 
{
    fpr x;
    uint32_t t;
    unsigned f;

    /*
     * If e >= -1076, then the value is "normal"; otherwise, it
     * should be a subnormal, which we clamp down to zero.
     */
    e += 1076;
    t = (uint32_t)e >> 31;
    m &= (uint64_t)t - 1;

    /*
     * If m = 0 then we want a zero; make e = 0 too, but conserve
     * the sign.
     */
    t = (uint32_t)(m >> 54);
    e &= -(int)t;

    /*
     * The 52 mantissa bits come from m. Value m has its top bit set
     * (unless it is a zero); we leave it "as is": the top bit will
     * increment the exponent by 1, except when m = 0, which is
     * exactly what we want.
     */
    x = (((uint64_t)s << 63) | (m >> 2)) + ((uint64_t)(uint32_t)e << 52);

    /*
     * Rounding: if the low three bits of m are 011, 110 or 111,
     * then the value should be incremented to get the next
     * representable value. This implements the usual
     * round-to-nearest rule (with preference to even values in case
     * of a tie). Note that the increment may make a carry spill
     * into the exponent field, which is again exactly what we want
     * in that case.
     */
    f = (unsigned)m & 7U;
    x += (0xC8U >> f) & 1;
    return x;
}
static inline uint64_t
fpr_ulsh(uint64_t x, int n) {
    x ^= (x ^ (x << 32)) & -(uint64_t)(n >> 5);
    return x << (n & 31);
}
static inline uint64_t
fpr_ursh(uint64_t x, int n) {
    x ^= (x ^ (x >> 32)) & -(uint64_t)(n >> 5);
    return x >> (n & 31);
}

fpr
fpr_scaled(int64_t i, int sc)
{
	/*
	 * To convert from int to float, we have to do the following:
	 *  1. Get the absolute value of the input, and its sign
	 *  2. Shift right or left the value as appropriate
	 *  3. Pack the result
	 *
	 * We can assume that the source integer is not -2^63.
	 */
	int s, e;
	uint32_t t;
	uint64_t m;

	/*
	 * Extract sign bit.
	 * We have: -i = 1 + ~i
	 */
	s = (int)((uint64_t)i >> 63);
	i ^= -(int64_t)s;
	i += s;

	/*
	 * For now we suppose that i != 0.
	 * Otherwise, we set m to i and left-shift it as much as needed
	 * to get a 1 in the top bit. We can do that in a logarithmic
	 * number of conditional shifts.
	 */
	m = (uint64_t)i;
	e = 9 + sc;
	FPR_NORM64(m, e);

	/*
	 * Now m is in the 2^63..2^64-1 range. We must divide it by 512;
	 * if one of the dropped bits is a 1, this should go into the
	 * "sticky bit".
	 */
	m |= ((uint32_t)m & 0x1FF) + 0x1FF;
	m >>= 9;

	/*
	 * Corrective action: if i = 0 then all of the above was
	 * incorrect, and we clamp e and m down to zero.
	 */
	t = (uint32_t)((uint64_t)(i | -i) >> 63);
	m &= -(uint64_t)t;
	e &= -(int)t;

	/*
	 * Assemble back everything. The FPR() function will handle cases
	 * where e is too low.
	 */
	return FPR(s, e, m);
}

fpr
fpr_add(fpr x, fpr y,fpr *num) {
    uint64_t m, xu, yu, za;
    uint32_t cs;
    int ex, ey, sx, sy, cc;

    /*
     * Make sure that the first operand (x) has the larger absolute
     * value. This guarantees that the exponent of y is less than
     * or equal to the exponent of x, and, if they are equal, then
     * the mantissa of y will not be greater than the mantissa of x.
     *
     * After this swap, the result will have the sign x, except in
     * the following edge case: abs(x) = abs(y), and x and y have
     * opposite sign bits; in that case, the result shall be +0
     * even if the sign bit of x is 1. To handle this case properly,
     * we do the swap is abs(x) = abs(y) AND the sign of x is 1.
     */
    m = ((uint64_t)1 << 63) - 1;
    za = (x & m) - (y & m);
    cs = (uint32_t)(za >> 63)
         | ((1U - (uint32_t)(-za >> 63)) & (uint32_t)(x >> 63));
    m = (x ^ y) & -(uint64_t)cs;
    x ^= m;
    y ^= m;

    /*
     * Extract sign bits, exponents and mantissas. The mantissas are
     * scaled up to 2^55..2^56-1, and the exponent is unbiased. If
     * an operand is zero, its mantissa is set to 0 at this step, and
     * its exponent will be -1078.
     */
    ex = (int)(x >> 52);
    sx = ex >> 11;
    ex &= 0x7FF;
    m = (uint64_t)(uint32_t)((ex + 0x7FF) >> 11) << 52;
    xu = ((x & (((uint64_t)1 << 52) - 1)) | m) << 3;
    ex -= 1078;
    ey = (int)(y >> 52);
    sy = ey >> 11;
    ey &= 0x7FF;
    m = (uint64_t)(uint32_t)((ey + 0x7FF) >> 11) << 52;
    yu = ((y & (((uint64_t)1 << 52) - 1)) | m) << 3;
    ey -= 1078;

    /*
     * x has the larger exponent; hence, we only need to right-shift y.
     * If the shift count is larger than 59 bits then we clamp the
     * value to zero.
     */
    cc = ex - ey;
    yu &= -(uint64_t)((uint32_t)(cc - 60) >> 31);
    cc &= 63;

    /*
     * The lowest bit of yu is "sticky".
     */
    m = fpr_ulsh(1, cc) - 1;
    yu |= (yu & m) + m;
    yu = fpr_ursh(yu, cc);

    /*
     * If the operands have the same sign, then we add the mantissas;
     * otherwise, we subtract the mantissas.
     */
    xu += yu - ((yu << 1) & -(uint64_t)(sx ^ sy));

    /*
     * The result may be smaller, or slightly larger. We normalize
     * it to the 2^63..2^64-1 range (if xu is zero, then it stays
     * at zero).
     */
    *num = 63 - ex;
    FPR_NORM64(xu, ex);
    *num += ex;

    /*
     * Scale down the value to 2^54..s^55-1, handling the last bit
     * as sticky.
     */
    xu |= ((uint32_t)xu & 0x1FF) + 0x1FF;
    xu >>= 9;
    ex += 9;

    /*
     * In general, the result has the sign of x. However, if the
     * result is exactly zero, then the following situations may
     * be encountered:
     *   x > 0, y = -x   -> result should be +0
     *   x < 0, y = -x   -> result should be +0
     *   x = +0, y = +0  -> result should be +0
     *   x = -0, y = +0  -> result should be +0
     *   x = +0, y = -0  -> result should be +0
     *   x = -0, y = -0  -> result should be -0
     *
     * But at the conditional swap step at the start of the
     * function, we ensured that if abs(x) = abs(y) and the
     * sign of x was 1, then x and y were swapped. Thus, the
     * two following cases cannot actually happen:
     *   x < 0, y = -x
     *   x = -0, y = +0
     * In all other cases, the sign bit of x is conserved, which
     * is what the FPR() function does. The FPR() function also
     * properly clamps values to zero when the exponent is too
     * low, but does not alter the sign in that case.
     */
    return FPR(sx, ex, xu);
}
static inline fpr
fpr_sub(fpr x, fpr y,fpr *num) {
    y ^= (uint64_t)1 << 63;
    return fpr_add(x, y,num);
}
fpr
make_group_add(fpr x, fpr y,int *group) {
    uint64_t m, xu, yu, za;
    uint32_t cs;
    int ex, ey, sx, sy, cc;
    fpr num;
    int idx = 0;
    m = ((uint64_t)1 << 63) - 1;
    za = (x & m) - (y & m);
    cs = (uint32_t)(za >> 63)
         | ((1U - (uint32_t)(-za >> 63)) & (uint32_t)(x >> 63));
    m = (x ^ y) & -(uint64_t)cs;
    group[idx++] = cs & 0x1;
    x ^= m;
    y ^= m;
    ex = (int)(x >> 52);
    sx = ex >> 11;
    ex &= 0x7FF;
    m = (uint64_t)(uint32_t)((ex + 0x7FF) >> 11) << 52;
    xu = ((x & (((uint64_t)1 << 52) - 1)) | m) << 3;
    ex -= 1078;
    ey = (int)(y >> 52);
    sy = ey >> 11;
    ey &= 0x7FF;
    m = (uint64_t)(uint32_t)((ey + 0x7FF) >> 11) << 52;
    yu = ((y & (((uint64_t)1 << 52) - 1)) | m) << 3;
    ey -= 1078;
    cc = ex - ey;
    yu &= -(uint64_t)((uint32_t)(cc - 60) >> 31);
    cc &= 63;
    m = fpr_ulsh(1, cc) - 1;
    yu |= (yu & m) + m;
    yu = fpr_ursh(yu, cc);
    group[idx++] = (sx ^ sy);
    xu += yu - ((yu << 1) & -(uint64_t)(sx ^ sy));
    num = 63 - ex;
    FPR_NORM64(xu, ex);
    num += ex;
    xu |= ((uint32_t)xu & 0x1FF) + 0x1FF;
    xu >>= 9;
    ex += 9;
    idx = 7;
    for(int i = 0 ; i < 6 ; i++)
    {
        group[idx--] = (num & 0x1) ^ 0x1;
        num >>= 1;
    }
    return FPR(sx, ex, xu);
}

fpr
make_group_scaled(int64_t i, int sc,int *group)
{
	/*
	 * To convert from int to float, we have to do the following:
	 *  1. Get the absolute value of the input, and its sign
	 *  2. Shift right or left the value as appropriate
	 *  3. Pack the result
	 *
	 * We can assume that the source integer is not -2^63.
	 */
	int s, e;
	uint32_t t;
    int idx = 0;
	uint64_t m;
    fpr num;

	/*
	 * Extract sign bit.
	 * We have: -i = 1 + ~i
	 */
	s = (int)((uint64_t)i >> 63);
	i ^= -(int64_t)s;
	i += s;
    group[idx++] = s & 0x1;

	/*
	 * For now we suppose that i != 0.
	 * Otherwise, we set m to i and left-shift it as much as needed
	 * to get a 1 in the top bit. We can do that in a logarithmic
	 * number of conditional shifts.
	 */
	m = (uint64_t)i;
	e = 9 + sc;

    num = 63 - e;
	FPR_NORM64(m, e);
    num += e;

    idx = 6;
    for(int j = 0 ; j < 6 ; j++)
    {
        group[idx--] = (num & 0x1) ^ 0x1;
        num >>= 1;
    }
    idx = 7;

	/*
	 * Now m is in the 2^63..2^64-1 range. We must divide it by 512;
	 * if one of the dropped bits is a 1, this should go into the
	 * "sticky bit".
	 */
	m |= ((uint32_t)m & 0x1FF) + 0x1FF;
	m >>= 9;

	/*
	 * Corrective action: if i = 0 then all of the above was
	 * incorrect, and we clamp e and m down to zero.
	 */
	t = (uint32_t)((uint64_t)(i | -i) >> 63);
	m &= -(uint64_t)t;
	e &= -(int)t;
    group[idx++] = t & 0x1;


	/*
	 * Assemble back everything. The FPR() function will handle cases
	 * where e is too low.
	 */
	return FPR(s, e, m);
}


void add_test(const char *pt_path, const char *group_path, int n)
{
    FILE *fp = fopen(pt_path,"r");
    if (fp == NULL) {
        fprintf(stderr, "error: failed to open %s\n", pt_path);
        return;
    }
    int cnt = 0;
    fpr **a;
    a = (fpr **)malloc(sizeof(fpr *)*n);
    for(int i = 0 ; i < n ; i++)
    {
        a[i] = (fpr *)malloc(sizeof(fpr) * 3);
    }
    //fpr a[614500][3];// more than n
    fpr e,d;
    for(int i = 0 ; i < n ; i++)
    {
        fscanf(fp,"%llx%llx%llx",&a[i][0],&a[i][1],&a[i][2]);
        d = fpr_add(a[i][0],a[i][1],&e);
        if(a[i][2] == d) cnt++;
        else
        {
            printf("%d: %llx %llx %llx %llx\n",i,a[i][0],a[i][1],a[i][2],d);
        }
    }
    printf("%d/%d\n",cnt,n); // functional check
    fclose(fp);
    fp = fopen(group_path,"w");
    if (fp == NULL) {
        fprintf(stderr, "error: failed to open %s\n", group_path);
        for(int i = 0 ; i < n ; i++) {
            free(a[i]);
        }
        free(a);
        return;
    }
    int group[10];// more than n, more than groupnum
    int groupnum = 8;
    for(int i = 0 ; i < n ; i++)
    {
        make_group_add(a[i][0],a[i][1],group);
        for(int j = 0 ; j < groupnum ; j++)
        {
            fprintf(fp,"%d ",group[j]);
        }
        fprintf(fp,"\n");
    }
    fclose(fp);
    for(int i = 0 ; i < n ; i++)
    {
        free(a[i]);
    }
    free(a);
    puts("add group fin");
}

void of_test(const char *pt_path, const char *group_path, int n)
{
    FILE *fp = fopen(pt_path,"r");
    if (fp == NULL) {
        fprintf(stderr, "error: failed to open %s\n", pt_path);
        return;
    }
    int cnt = 0;
    //fpr a[51300][2];// more than n
    fpr **a;
    a = (fpr **)malloc(sizeof(fpr *)*n);
    for(int i = 0 ; i < n ; i++)
    {
        a[i] = (fpr *)malloc(sizeof(fpr) * 2);
    }
    fpr e,d;
    for(int i = 0 ; i < n ; i++)
    {
        fscanf(fp,"%llx%llx",&a[i][0],&a[i][1]);
        d = fpr_scaled((int64_t)a[i][0],0);
        if(a[i][1] == d) cnt++;
        else
        {
            printf("%d: %llx %llx %llx\n",i,a[i][0],a[i][1],d);
        }
    }
    printf("%d/%d\n",cnt,n);// functional check
    fclose(fp);
    fp = fopen(group_path,"w");
    if (fp == NULL) {
        fprintf(stderr, "error: failed to open %s\n", group_path);
        for(int i = 0 ; i < n ; i++) {
            free(a[i]);
        }
        free(a);
        return;
    }
    int group[30];// more than n, more than groupnum
    int groupnum = 8;
    for(int i = 0 ; i < n ; i++)
    {
        make_group_scaled(a[i][0],0,group);
        for(int j = 0 ; j < groupnum ; j++)
        {
            fprintf(fp,"%d ",group[j]);
        }
        fprintf(fp,"\n");
    }
    fclose(fp);
    for(int i = 0 ; i < n ; i++)
    {
        free(a[i]);
    }
    free(a);
    puts("of group fin");
}

int main(int argc,char **argv)
{
    if (argc < 6) {
        fprintf(stderr, "usage: %s <testN> <pt_add.txt> <pt_of.txt> <add_group.txt> <of_group.txt>\n", argv[0]);
        return 1;
    }
    int testN = atoi(argv[1]);
    int n_add = testN * 6144;
    int n_of = testN * 512;
    add_test(argv[2], argv[4], n_add);
    of_test(argv[3], argv[5], n_of);
    return 0;
}
