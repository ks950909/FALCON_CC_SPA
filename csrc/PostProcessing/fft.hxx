#ifndef _FFT_H__
#define _FFT_H__

#include <cstdint>
#include <vector>
typedef uint64_t fpr;

fpr fpr_add(fpr x, fpr y,fpr *num);
fpr fpr_add_leak(fpr x, fpr y,std::vector<int> &out) ;
fpr fpr_sub(fpr x, fpr y,fpr *num);
fpr fpr_sub_leak(fpr x, fpr y,std::vector<int> &out) ;
fpr fpr_of(int64_t i);
fpr fpr_of_leak(int64_t i,std::vector<int> &out);
fpr fft_one_layer(fpr arr[]);
void fft_one_layer_leak(fpr arr[],std::vector<int> &out);
extern const fpr fpr_gm_tab[];

#endif //_FFT_H__