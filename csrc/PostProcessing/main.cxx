#include <iostream>
#include <vector>
#include <cstdio>
#include <map>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <utility>
#include <algorithm>
#include <queue>
#include <numeric>
#include <string>
#include <random>
#include "fft.hxx"
using namespace std;

struct state_fft
{
    int th;
    bool isans;
    vector<fpr> f;
    vector<fpr> v;
    state_fft(){th=1e9;}
    state_fft(int th): th(th){}
    state_fft(const state_fft& obj)
    {
        th = obj.th;
        isans = obj.isans;
        for(auto &x : obj.f)
        {
            f.push_back(x);
        }
        for(auto &x : obj.v)
        {
            v.push_back(x);
        }
    }
    bool operator<(const state_fft s) const{
        return this->th < s.th;
    }
};

struct state_fft_pr
{
    double th_pr_log;
    bool isans;
    vector<fpr> f;
    vector<fpr> v;
    state_fft_pr(){th_pr_log=0.0;}
    state_fft_pr(double th_pr_log): th_pr_log(th_pr_log){}
    state_fft_pr(const state_fft_pr& obj)
    {
        th_pr_log = obj.th_pr_log;
        isans = obj.isans;
        for(auto &x : obj.f)
        {
            f.push_back(x);
        }
        for(auto &x : obj.v)
        {
            v.push_back(x);
        }
    }
    bool operator<(const state_fft_pr s) const{
        return this->th_pr_log > s.th_pr_log;
    }
};

void fft_leak(fpr arr[],vector<int> &out,size_t logn)
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
                fpr tmp[6];
                tmp[0] = arr[j];
                tmp[1] = arr[j + hn];
                tmp[2] = arr[j + ht];
                tmp[3] = arr[j + ht + hn];
                tmp[4] = s_re;
                tmp[5] = s_im;
                fft_one_layer_leak(tmp,out);
                arr[j          ] = tmp[0];
                arr[j + hn     ] = tmp[1];
                arr[j      + ht] = tmp[2];
                arr[j + hn + ht] = tmp[3];
            }
        }
        t = ht;
    }
}

void fft_attack(vector<int> &ans_v,vector<state_fft> &output,unsigned logn)
{
    unsigned u;
    size_t t,n,hn,m,qn;
    n = (size_t) 1 << logn;
    hn = n >> 1;
    qn = n >> 2;
    t = hn;
    vector<vector<vector<state_fft>>> f;
    vector<vector<state_fft>> tmp;
    f.push_back(tmp);
    fpr arr[6];
    vector<int> v;
    int fpr_of_leaknum = 8;
    int fpr_add_leaknum = 8 * 6;
    double logcand = 0;

    for(int i = 0 ; i < hn ; i++)
    {
        vector<state_fft> ttmp;
        int cnt1=0,cnt2=0;
        vector<int> cnt_arr;
        for(int jre = -31 ; jre <= 31 ; jre++)
        {
            fpr jre_fpr = fpr_of_leak(jre,v);
            cnt1 = 0;
            for(int ii = 0 ; ii < fpr_of_leaknum ; ii++)
            {
                cnt1 += (v[ii] != ans_v[i * fpr_of_leaknum + ii]);
            }
            v.clear();
            for(int jim = -31 ; jim <= 31 ; jim++)
            {
                fpr jim_fpr = fpr_of_leak(jim,v);
                cnt2 = 0;
                for(int ii = 0 ; ii < fpr_of_leaknum ; ii++)
                {
                    cnt2 += (v[ii] != ans_v[(i + hn) * fpr_of_leaknum + ii]);
                }
                v.clear();
                if(cnt1 + cnt2 == 0)
                {
                    ttmp.emplace_back(cnt1+cnt2);
                    ttmp.back().f.push_back(jre_fpr);
                    ttmp.back().f.push_back(jim_fpr);
                    ttmp.back().v.push_back(jre_fpr);
                    ttmp.back().v.push_back(jim_fpr);
                }
            }
        }
        f[0].push_back(ttmp);
    }

     for (u = 1, m = 2; u < logn; u ++, m <<= 1) {
        size_t ht, hm, i1, j1, tm;
        f.push_back(tmp);
        ht = t >> 1;
        hm = m >> 1;
        tm = m << 1;
        for(int i = 0 ; i < ht ; i++)
        {
            vector<state_fft> ttmp;
            f[u].push_back(ttmp);
        }
        for (size_t j = 0 ; j < ht ; j++)
        {
            // j, j+ht
            vector<state_fft> &x_arr = f[u-1][j];
            vector<state_fft> &y_arr = f[u-1][j + ht];
            for(auto &x : x_arr)
            {
                for(auto &y : y_arr)
                {
                    int cnt = x.th + y.th;
                    vector<fpr> ff;
                    for(i1 = 0 ; i1 < hm ; i1++)
                    {
                        arr[0] = x.f[i1   ];
                        arr[1] = x.f[i1+hm];
                        arr[2] = y.f[i1   ];
                        arr[3] = y.f[i1+hm];
                        arr[4] = fpr_gm_tab[((m + i1) << 1) + 0];
                        arr[5] = fpr_gm_tab[((m + i1) << 1) + 1];

                        fft_one_layer_leak(arr,v);

                        ff.push_back(arr[0]);
                        ff.push_back(arr[1]);
                        ff.push_back(arr[2]);
                        ff.push_back(arr[3]);

                        for(int i = 0 ; i < fpr_add_leaknum ; i++)
                        {
                            cnt += (v[i] != ans_v[fpr_of_leaknum * n + fpr_add_leaknum * qn *(u-1)+ i1*fpr_add_leaknum * ht + j * fpr_add_leaknum + i]);
                        }
                        v.clear();
                    }
                    if(cnt == 0)
                    {
                        f[u][j].emplace_back(cnt);
                        for(int i = 0 ; i < m ; i++)
                        {
                            f[u][j].back().f.push_back(ff[i * 2]);
                        }
                        for(int i = 0 ; i < m ; i++)
                        {
                            f[u][j].back().f.push_back(ff[i * 2 + 1]);
                        }
                        for(int i = 0 ; i < m ; i++)
                        {
                            f[u][j].back().v.push_back(x.v[i]);
                            f[u][j].back().v.push_back(y.v[i]);
                        }
                    }
                    ff.clear();
                }
            }
        }
        t = ht;
    }
    for(auto &x : f[logn-1][0])
    {
        output.push_back(x);
    }
}

int fft_attack_getmaxrank(vector<int> &ans_v,vector<state_fft> &output,unsigned logn,fpr ans[])
{
    unsigned u;
    size_t t,n,hn,m,qn;
    n = (size_t) 1 << logn;
    hn = n >> 1;
    qn = n >> 2;
    t = hn;
    vector<vector<vector<state_fft>>> f;
    vector<vector<state_fft>> tmp;
    f.push_back(tmp);
    fpr arr[6];
    vector<int> v;
    int fpr_of_leaknum = 8;
    int fpr_add_leaknum = 8 * 6;
    double logcand = 0;
    int max_threshold;
    int output_rank = 0;

    for(int i = 0 ; i < hn ; i++)
    {
        vector<state_fft> ttmp;
        vector<state_fft> ttmp_arr;
        int cnt1=0,cnt2=0;
        for(int jre = -31 ; jre <= 31 ; jre++)
        {
            fpr jre_fpr = fpr_of_leak(jre,v);
            cnt1 = 0;
            for(int ii = 0 ; ii < fpr_of_leaknum ; ii++)
            {
                cnt1 += (v[ii] != ans_v[i * fpr_of_leaknum + ii]);
            }
            v.clear();
            for(int jim = -31 ; jim <= 31 ; jim++)
            {
                fpr jim_fpr = fpr_of_leak(jim,v);
                cnt2 = 0;
                for(int ii = 0 ; ii < fpr_of_leaknum ; ii++)
                {
                    cnt2 += (v[ii] != ans_v[(i + hn) * fpr_of_leaknum + ii]);
                }
                v.clear();
                state_fft tttmp;
                tttmp.th = cnt1+cnt2;
                tttmp.f.push_back(jre_fpr);
                tttmp.f.push_back(jim_fpr);
                tttmp.v.push_back(jre_fpr);
                tttmp.v.push_back(jim_fpr);
                if(jre_fpr == ans[i] && jim_fpr == ans[i + hn])
                    tttmp.isans = true;
                else
                    tttmp.isans = false;
                ttmp_arr.push_back(tttmp);
            }
        }
        std::sort(ttmp_arr.begin(),ttmp_arr.end(),[](state_fft &x,state_fft &y) -> bool{
            return x.th < y.th;
        });

        int sz = ttmp_arr.size();
        int anssz = 0;
        for(size_t j = 0 ; j < sz ; j++)
        {
            if(ttmp_arr[j].isans)
            {
                anssz = j;
                if (output_rank < j)
                    output_rank = j;
                break;
            }
        }
        f[0].push_back(vector<state_fft>(ttmp_arr.begin(),ttmp_arr.begin()+anssz+1));
    }

     for (u = 1, m = 2; u < logn; u ++, m <<= 1) {
        size_t ht, hm, i1, j1, tm;
        f.push_back(tmp);
        ht = t >> 1;
        hm = m >> 1;
        tm = m << 1;
        for (size_t j = 0 ; j < ht ; j++)
        {
            // j, j+ht
            vector<state_fft> &x_arr = f[u-1][j];
            vector<state_fft> &y_arr = f[u-1][j + ht];
            vector<int> cnt_arr;
            vector<state_fft> ttmp_arr;

            for(auto &x : x_arr)
            {
                for(auto &y : y_arr)
                {
                    int cnt = x.th + y.th;
                    vector<fpr> ff;
                    for(i1 = 0 ; i1 < hm ; i1++)
                    {
                        arr[0] = x.f[i1   ];
                        arr[1] = x.f[i1+hm];
                        arr[2] = y.f[i1   ];
                        arr[3] = y.f[i1+hm];
                        arr[4] = fpr_gm_tab[((m + i1) << 1) + 0];
                        arr[5] = fpr_gm_tab[((m + i1) << 1) + 1];

                        fft_one_layer_leak(arr,v);

                        ff.push_back(arr[0]);
                        ff.push_back(arr[1]);
                        ff.push_back(arr[2]);
                        ff.push_back(arr[3]);

                        for(int i = 0 ; i < fpr_add_leaknum ; i++)
                        {
                            cnt += (v[i] != ans_v[fpr_of_leaknum * n + fpr_add_leaknum * qn *(u-1)+ i1*fpr_add_leaknum * ht + j * fpr_add_leaknum + i]);
                        }
                        v.clear();
                    }
                    state_fft tttmp;
                    tttmp.th = cnt;
                    tttmp.isans = x.isans & y.isans;
                    for(int i = 0 ; i < m ; i++)
                        tttmp.f.push_back(ff[i * 2]);
                    for(int i = 0 ; i < m ; i++)
                        tttmp.f.push_back(ff[i*2+1]);
                    for(int i = 0 ; i < m ; i++)
                    {
                        tttmp.v.push_back(x.v[i]);
                        tttmp.v.push_back(y.v[i]);
                    }
                    ttmp_arr.push_back(tttmp);
                    ff.clear();
                }
            }
            std::sort(ttmp_arr.begin(),ttmp_arr.end(),[](state_fft &x,state_fft &y) -> bool{
                return x.th < y.th;
            });
            int sz = ttmp_arr.size();
            int anssz = 0;
            for(size_t k = 0 ; k < sz ; k++)
            {
                if(ttmp_arr[k].isans)
                {
                    anssz = k;
                    if(output_rank < k)
                        output_rank = k;
                    break;
                }
            }
            f[u].push_back(vector<state_fft>(ttmp_arr.begin(),ttmp_arr.begin()+anssz+1));
        }
        t = ht;
    }
    for(auto &x : f[logn-1][0])
    {
        output.push_back(x);
    }
    return output_rank;
}

int fft_attack_getmaxrank_pr(vector<double> &ans_v,vector<state_fft_pr> &output,unsigned logn,fpr ans[])
{
    unsigned u;
    size_t t,n,hn,m,qn;
    n = (size_t) 1 << logn;
    hn = n >> 1;
    qn = n >> 2;
    t = hn;
    vector<vector<vector<state_fft_pr>>> f;
    vector<vector<state_fft_pr>> tmp;
    f.push_back(tmp);
    fpr arr[6];
    vector<int> v;
    int fpr_of_leaknum = 8;
    int fpr_add_leaknum = 8 * 6;
    int output_rank = 0;
    double tol = 1e-18;
    double tolval = log(1e-18);

    for(int i = 0 ; i < (int)hn ; i++)
    {
        vector<state_fft_pr> ttmp_arr;
        double cnt1=0.0,cnt2=0.0;
        for(int jre = -31 ; jre <= 31 ; jre++)
        {
            fpr jre_fpr = fpr_of_leak(jre,v);
            cnt1 = 0.0;
            for(int ii = 0 ; ii < fpr_of_leaknum ; ii++)
            {
                if(v[ii] == 0)
                {
                    if(ans_v[i * fpr_of_leaknum + ii] < tol)
                        cnt1 += tolval;
                    else
                        cnt1 += log(ans_v[i * fpr_of_leaknum + ii]);
                }
                else
                {
                    if((1.0-ans_v[i * fpr_of_leaknum + ii]) < tol)
                        cnt1 += tolval;
                    else
                        cnt1 += log(1.0-ans_v[i * fpr_of_leaknum + ii]);
                }
            }
            v.clear();
            for(int jim = -31 ; jim <= 31 ; jim++)
            {
                fpr jim_fpr = fpr_of_leak(jim,v);
                cnt2 = 0.0;
                for(int ii = 0 ; ii < fpr_of_leaknum ; ii++)
                {
                    if(v[ii] == 0)
                    {
                        if(ans_v[(i + hn) * fpr_of_leaknum + ii] < tol)
                            cnt2 += tolval;
                        else
                            cnt2 += log(ans_v[(i + hn) * fpr_of_leaknum + ii]);
                    }
                    else
                    {
                        if((1.0-ans_v[(i + hn) * fpr_of_leaknum + ii]) < tol)
                            cnt2 += tolval;
                        else
                            cnt2 += log(1.0-ans_v[(i + hn) * fpr_of_leaknum + ii]);
                    }
                }
                v.clear();
                state_fft_pr tttmp;
                tttmp.th_pr_log = cnt1+cnt2;
                tttmp.f.push_back(jre_fpr);
                tttmp.f.push_back(jim_fpr);
                tttmp.v.push_back(jre_fpr);
                tttmp.v.push_back(jim_fpr);
                if(jre_fpr == ans[i] && jim_fpr == ans[i + hn])
                    tttmp.isans = true;
                else
                    tttmp.isans = false;
                ttmp_arr.push_back(tttmp);
            }
        }
        std::sort(ttmp_arr.begin(),ttmp_arr.end(),[](state_fft_pr &x,state_fft_pr &y) -> bool{
            return x.th_pr_log > y.th_pr_log;
        });
        int sz = ttmp_arr.size();
        int anssz = 0;
        for(size_t j = 0 ; j < (size_t)sz ; j++)
        {
            if(ttmp_arr[j].isans)
            {
                anssz = j;
                if(output_rank < (int)j)
                    output_rank = (int)j;
                break;
            }
        }
        f[0].push_back(vector<state_fft_pr>(ttmp_arr.begin(),ttmp_arr.begin()+anssz+1));
    }

    for (u = 1, m = 2; u < logn; u ++, m <<= 1) {
        size_t ht, hm, i1, j1;
        f.push_back(tmp);
        ht = t >> 1;
        hm = m >> 1;
        for (size_t j = 0 ; j < ht ; j++)
        {
            vector<state_fft_pr> &x_arr = f[u-1][j];
            vector<state_fft_pr> &y_arr = f[u-1][j + ht];
            vector<state_fft_pr> ttmp_arr;

            for(auto &x : x_arr)
            {
                for(auto &y : y_arr)
                {
                    double cnt = x.th_pr_log + y.th_pr_log;
                    vector<fpr> ff;
                    for(i1 = 0 ; i1 < hm ; i1++)
                    {
                        arr[0] = x.f[i1   ];
                        arr[1] = x.f[i1+hm];
                        arr[2] = y.f[i1   ];
                        arr[3] = y.f[i1+hm];
                        arr[4] = fpr_gm_tab[((m + i1) << 1) + 0];
                        arr[5] = fpr_gm_tab[((m + i1) << 1) + 1];

                        fft_one_layer_leak(arr,v);

                        ff.push_back(arr[0]);
                        ff.push_back(arr[1]);
                        ff.push_back(arr[2]);
                        ff.push_back(arr[3]);

                        for(int i = 0 ; i < fpr_add_leaknum ; i++)
                        {
                            if(v[i] == 0)
                            {
                                if(ans_v[fpr_of_leaknum * n + fpr_add_leaknum * qn *(u-1)+ i1*fpr_add_leaknum * ht + j * fpr_add_leaknum + i] < tol)
                                    cnt += tolval;
                                else
                                    cnt += log(ans_v[fpr_of_leaknum * n + fpr_add_leaknum * qn *(u-1)+ i1*fpr_add_leaknum * ht + j * fpr_add_leaknum + i]);
                            }
                            else
                            {
                                if((1.0-ans_v[fpr_of_leaknum * n + fpr_add_leaknum * qn *(u-1)+ i1*fpr_add_leaknum * ht + j * fpr_add_leaknum + i]) < tol)
                                    cnt += tolval;
                                else
                                    cnt += log(1.0-ans_v[fpr_of_leaknum * n + fpr_add_leaknum * qn *(u-1)+ i1*fpr_add_leaknum * ht + j * fpr_add_leaknum + i]);
                            }
                        }
                        v.clear();
                    }
                    state_fft_pr tttmp;
                    tttmp.th_pr_log = cnt;
                    tttmp.isans = x.isans & y.isans;
                    for(int i = 0 ; i < (int)m ; i++)
                        tttmp.f.push_back(ff[i * 2]);
                    for(int i = 0 ; i < (int)m ; i++)
                        tttmp.f.push_back(ff[i*2+1]);
                    for(int i = 0 ; i < (int)m ; i++)
                    {
                        tttmp.v.push_back(x.v[i]);
                        tttmp.v.push_back(y.v[i]);
                    }
                    ttmp_arr.push_back(tttmp);
                    ff.clear();
                }
            }
            std::sort(ttmp_arr.begin(),ttmp_arr.end(),[](state_fft_pr &x,state_fft_pr &y) -> bool{
                return x.th_pr_log > y.th_pr_log;
            });
            int sz = ttmp_arr.size();
            int anssz = 0;
            for(size_t k = 0 ; k < (size_t)sz ; k++)
            {
                if(ttmp_arr[k].isans)
                {
                    anssz = k;
                    if(output_rank < (int)k)
                        output_rank = (int)k;
                    break;
                }
            }
            f[u].push_back(vector<state_fft_pr>(ttmp_arr.begin(),ttmp_arr.begin()+anssz+1));
        }
        t = ht;
    }
    output = f[logn-1][0];
    return output_rank;
}

void fft_attack_getmaxrank_pr_test(int testmaxnum,double sigma)
{
    int ans[512];
    fpr arr[512];
    fpr arr_save[512];
    int logn = 9;
    int n = 1<<logn;
    vector<int> ans_v;
    vector<double> ans_v_d;
    vector<state_fft_pr> cand;
    unsigned seed = (unsigned)time(NULL);
    srand(seed);
    normal_distribution<double> distribution(0.0,4.053163803303075);
    normal_distribution<double> distribution_noise(0.0,sigma);
    default_random_engine generator;
    default_random_engine generator2;
    generator.seed(seed);
    generator2.seed(seed + 1);
    clock_t starttime = clock();
    for(int i = 0 ; i < n ; i++)
    {
        ans[i] = int(distribution(generator));
    }
    vector<int> ansrank_v;
    vector<int> ansmaxrank_v;

    for(int testnum = 0; testnum < testmaxnum ; testnum++)
    {
        if (testnum % 10 == 0) {
            fprintf(stderr, "[maxrank_pr] test %d/%d\n", testnum + 1, testmaxnum);
        }
        for(int i = 0 ; i < n ; i++)
        {
            arr[i] = fpr_of_leak(ans[i],ans_v);
            arr_save[i] = arr[i];
        }
        fft_leak(arr,ans_v,logn);
        int match_cnt = 0;
        for(int i = 0 ; i < (int)ans_v.size() ; i++)
        {
            double dist = distribution_noise(generator2);
            double pr0,pr1;
            if(ans_v[i] == 0)
            {
                pr0 = exp(-dist * dist / (2.0 * sigma * sigma));
                pr1 = exp(-(64.0-dist) * (64.0-dist) / (2.0 * sigma * sigma));
            }
            else
            {
                pr1 = exp(-dist * dist / (2.0 * sigma * sigma));
                pr0 = exp(-(64.0+dist) * (64.0+dist) / (2.0 * sigma * sigma));
            }
            ans_v_d.push_back(min(max(pr0 / (pr0+pr1),(double)1e-5),(double)1.0-(1e-5)));

            if     (ans_v_d.back() >= 0.5 && ans_v[i] == 0) match_cnt++;
            else if(ans_v_d.back() <  0.5 && ans_v[i] != 0) match_cnt++;
        }
        printf("match : %d\n",match_cnt);
        int keymaxrank = fft_attack_getmaxrank_pr(ans_v_d,cand,logn,arr_save);
        ansmaxrank_v.push_back(keymaxrank);
        int ansrank = -1;
        for(int i = 0 ; i < (int)cand.size() ; i++)
        {
            auto &x = cand[i];
            if(x.isans)
                ansrank = i;
        }
        ansrank_v.push_back(ansrank);
        ans_v.clear();
        ans_v_d.clear();
        cand.clear();
    }
    clock_t endtime = clock();
    printf("time : %lf ,mean time : %lf\n",(double)(endtime - starttime) / CLOCKS_PER_SEC,(double)(endtime - starttime) / testmaxnum/ CLOCKS_PER_SEC);
    int ansval = 0;
    int ansmaxrank = 0;
    int nofind = 0;
    for(auto &x : ansrank_v)
    {
        ansval += x;
    }
    for(auto &x : ansmaxrank_v)
    {
        ansmaxrank += x;
    }
    if(testmaxnum - nofind == 0)
        printf("%d/%d , mean : no find\n",testmaxnum-nofind,testmaxnum);
    else
    {
        // mean : mean of output rank, mean maxkey : Average of the maximum rank observed during the post-processing procedure
        printf("%d/%d , mean : %lf, mean maxrank : %lf\n",testmaxnum-nofind,testmaxnum,(double)(ansval)/(testmaxnum - nofind),(double)(ansmaxrank) / (testmaxnum - nofind));
    }
}


void fft_attack_rank(vector<int> &ans_v,vector<state_fft> &output,unsigned logn,int rank)
{
    unsigned u;
    size_t t,n,hn,m,qn;
    n = (size_t) 1 << logn;
    hn = n >> 1;
    qn = n >> 2;
    t = hn;
    vector<vector<vector<state_fft>>> f;
    vector<vector<state_fft>> tmp;
    f.push_back(tmp);
    fpr arr[6];
    vector<int> v;
    int fpr_of_leaknum = 8;
    int fpr_add_leaknum = 8 * 6;
    double logcand = 0;
    int max_threshold;

    for(int i = 0 ; i < hn ; i++)
    {
        vector<state_fft> ttmp;
        vector<state_fft> ttmp_arr;
        int cnt1=0,cnt2=0;
        for(int jre = -31 ; jre <= 31 ; jre++)
        {
            fpr jre_fpr = fpr_of_leak(jre,v);
            cnt1 = 0;
            for(int ii = 0 ; ii < fpr_of_leaknum ; ii++)
            {
                cnt1 += (v[ii] != ans_v[i * fpr_of_leaknum + ii]);
            }
            v.clear();
            for(int jim = -31 ; jim <= 31 ; jim++)
            {
                fpr jim_fpr = fpr_of_leak(jim,v);
                cnt2 = 0;
                for(int ii = 0 ; ii < fpr_of_leaknum ; ii++)
                {
                    cnt2 += (v[ii] != ans_v[(i + hn) * fpr_of_leaknum + ii]);
                }
                v.clear();
                state_fft tttmp;
                tttmp.th = cnt1+cnt2;
                tttmp.f.push_back(jre_fpr);
                tttmp.f.push_back(jim_fpr);
                tttmp.v.push_back(jre_fpr);
                tttmp.v.push_back(jim_fpr);
                ttmp_arr.push_back(tttmp);
            }
        }
        std::sort(ttmp_arr.begin(),ttmp_arr.end(),[](state_fft &x,state_fft &y) -> bool{
            return x.th < y.th;
        });
        f[0].push_back(vector<state_fft>(ttmp_arr.begin(),ttmp_arr.begin()+rank));
    }

     for (u = 1, m = 2; u < logn; u ++, m <<= 1) {
        size_t ht, hm, i1, j1, tm;
        f.push_back(tmp);
        ht = t >> 1;
        hm = m >> 1;
        tm = m << 1;
        for (size_t j = 0 ; j < ht ; j++)
        {
            vector<state_fft> &x_arr = f[u-1][j];
            vector<state_fft> &y_arr = f[u-1][j + ht];
            vector<int> cnt_arr;
            vector<state_fft> ttmp_arr;

            for(auto &x : x_arr)
            {
                for(auto &y : y_arr)
                {
                    int cnt = x.th + y.th;
                    vector<fpr> ff;
                    for(i1 = 0 ; i1 < hm ; i1++)
                    {
                        arr[0] = x.f[i1   ];
                        arr[1] = x.f[i1+hm];
                        arr[2] = y.f[i1   ];
                        arr[3] = y.f[i1+hm];
                        arr[4] = fpr_gm_tab[((m + i1) << 1) + 0];
                        arr[5] = fpr_gm_tab[((m + i1) << 1) + 1];

                        fft_one_layer_leak(arr,v);

                        ff.push_back(arr[0]);
                        ff.push_back(arr[1]);
                        ff.push_back(arr[2]);
                        ff.push_back(arr[3]);

                        for(int i = 0 ; i < fpr_add_leaknum ; i++)
                        {
                            cnt += (v[i] != ans_v[fpr_of_leaknum * n + fpr_add_leaknum * qn *(u-1)+ i1*fpr_add_leaknum * ht + j * fpr_add_leaknum + i]);
                        }
                        v.clear();
                    }
                    state_fft tttmp;
                    tttmp.th = cnt;
                    tttmp.isans = x.isans & y.isans;
                    for(int i = 0 ; i < m ; i++)
                        tttmp.f.push_back(ff[i * 2]);
                    for(int i = 0 ; i < m ; i++)
                        tttmp.f.push_back(ff[i*2+1]);
                    for(int i = 0 ; i < m ; i++)
                    {
                        tttmp.v.push_back(x.v[i]);
                        tttmp.v.push_back(y.v[i]);
                    }
                    ttmp_arr.push_back(tttmp);
                    ff.clear();
                }
            }
            std::sort(ttmp_arr.begin(),ttmp_arr.end(),[](state_fft &x,state_fft &y) -> bool{
                return x.th < y.th;
            });
            f[u].push_back(vector<state_fft>(ttmp_arr.begin(),ttmp_arr.begin()+rank));
        }
        t = ht;
    }
    for(auto &x : f[logn-1][0])
    {
        output.push_back(x);
    }
}

void fft_attack_pr_rank(vector<double> &ans_v,vector<state_fft_pr> &output,unsigned logn,int rank)
{
    unsigned u;
    size_t t,n,hn,m,qn;
    n = (size_t) 1 << logn;
    hn = n >> 1;
    qn = n >> 2;
    t = hn;
    vector<vector<vector<state_fft_pr>>> f;
    vector<vector<state_fft_pr>> tmp;
    f.push_back(tmp);
    fpr arr[6];
    vector<int> v;
    int fpr_of_leaknum = 8;
    int fpr_add_leaknum = 8 * 6;
    double logcand = 0;
    int max_threshold;
    double tol = 1e-18;
    double tolval = log(1e-18);

    for(int i = 0 ; i < hn ; i++)
    {
        vector<state_fft_pr> ttmp;
        vector<state_fft_pr> ttmp_arr;
        double cnt1=0.0,cnt2=0.0;
        for(int jre = -31 ; jre <= 31 ; jre++)
        {
            fpr jre_fpr = fpr_of_leak(jre,v);
            cnt1 = 0.0;
            for(int ii = 0 ; ii < fpr_of_leaknum ; ii++)
            {
                if(v[ii] == 0)
                {
                    if(ans_v[i * fpr_of_leaknum + ii] < tol)
                        cnt1 += tolval;
                    else
                        cnt1 += log(ans_v[i * fpr_of_leaknum + ii]);
                }
                else
                {
                    if((1.0-ans_v[i * fpr_of_leaknum + ii]) < tol) 
                        cnt1 += tolval;
                    else
                        cnt1 += log(1.0-ans_v[i * fpr_of_leaknum + ii]);
                }
            }
            v.clear();
            for(int jim = -31 ; jim <= 31 ; jim++)
            {
                fpr jim_fpr = fpr_of_leak(jim,v);
                cnt2 = 0.0;
                for(int ii = 0 ; ii < fpr_of_leaknum ; ii++)
                {
                    if(v[ii] == 0)
                    {
                        if(ans_v[(i + hn) * fpr_of_leaknum + ii] < tol)
                            cnt2 += tolval;
                        else
                            cnt2 += log(ans_v[(i + hn) * fpr_of_leaknum + ii]);
                    }
                    else
                    {
                        if((1.0-ans_v[(i + hn) * fpr_of_leaknum + ii]) < tol)
                            cnt2 += tolval;
                        else
                            cnt2 += log(1.0-ans_v[(i + hn) * fpr_of_leaknum + ii]);
                    }
                }
                v.clear();
                state_fft_pr tttmp;
                tttmp.th_pr_log = cnt1+cnt2;
                tttmp.f.push_back(jre_fpr);
                tttmp.f.push_back(jim_fpr);
                tttmp.v.push_back(jre_fpr);
                tttmp.v.push_back(jim_fpr);
                ttmp_arr.push_back(tttmp);
            }
        }
        std::sort(ttmp_arr.begin(),ttmp_arr.end(),[](state_fft_pr &x,state_fft_pr &y) -> bool{
            return x.th_pr_log > y.th_pr_log;
        });
        f[0].push_back(vector<state_fft_pr>(ttmp_arr.begin(),ttmp_arr.begin()+rank));
    }

    for (u = 1, m = 2; u < logn; u ++, m <<= 1) {
        size_t ht, hm, i1, j1, tm;
        f.push_back(tmp);
        ht = t >> 1;
        hm = m >> 1;
        tm = m << 1;
        for (size_t j = 0 ; j < ht ; j++)
        {
            vector<state_fft_pr> &x_arr = f[u-1][j];
            vector<state_fft_pr> &y_arr = f[u-1][j + ht];
            vector<double> cnt_arr;
            vector<state_fft_pr> ttmp_arr;

            for(auto &x : x_arr)
            {
                for(auto &y : y_arr)
                {
                    double cnt = x.th_pr_log + y.th_pr_log;
                    vector<fpr> ff;
                    for(i1 = 0 ; i1 < hm ; i1++)
                    {
                        arr[0] = x.f[i1   ];
                        arr[1] = x.f[i1+hm];
                        arr[2] = y.f[i1   ];
                        arr[3] = y.f[i1+hm];
                        arr[4] = fpr_gm_tab[((m + i1) << 1) + 0];
                        arr[5] = fpr_gm_tab[((m + i1) << 1) + 1];

                        fft_one_layer_leak(arr,v);

                        ff.push_back(arr[0]);
                        ff.push_back(arr[1]);
                        ff.push_back(arr[2]);
                        ff.push_back(arr[3]);

                        for(int i = 0 ; i < fpr_add_leaknum ; i++)
                        {
                            if(v[i] == 0)
                            {
                                if(ans_v[fpr_of_leaknum * n + fpr_add_leaknum * qn *(u-1)+ i1*fpr_add_leaknum * ht + j * fpr_add_leaknum + i] < tol)
                                    cnt += tolval;
                                else
                                    cnt += log(ans_v[fpr_of_leaknum * n + fpr_add_leaknum * qn *(u-1)+ i1*fpr_add_leaknum * ht + j * fpr_add_leaknum + i]);
                            }
                            else
                            {
                                if((1.0-ans_v[fpr_of_leaknum * n + fpr_add_leaknum * qn *(u-1)+ i1*fpr_add_leaknum * ht + j * fpr_add_leaknum + i]) < tol)
                                    cnt += tolval;
                                else
                                    cnt += log(1.0-ans_v[fpr_of_leaknum * n + fpr_add_leaknum * qn *(u-1)+ i1*fpr_add_leaknum * ht + j * fpr_add_leaknum + i]);
                            }
                        }
                        v.clear();
                    }
                    state_fft_pr tttmp;
                    tttmp.th_pr_log = cnt;
                    tttmp.isans = x.isans & y.isans;
                    for(int i = 0 ; i < m ; i++)
                        tttmp.f.push_back(ff[i * 2]);
                    for(int i = 0 ; i < m ; i++)
                        tttmp.f.push_back(ff[i*2+1]);
                    for(int i = 0 ; i < m ; i++)
                    {
                        tttmp.v.push_back(x.v[i]);
                        tttmp.v.push_back(y.v[i]);
                    }
                    ttmp_arr.push_back(tttmp);
                    ff.clear();
                }
            }
            std::sort(ttmp_arr.begin(),ttmp_arr.end(),[](state_fft_pr &x,state_fft_pr &y) -> bool{
                return x.th_pr_log > y.th_pr_log;
            });
            f[u].push_back(vector<state_fft_pr>(ttmp_arr.begin(),ttmp_arr.begin()+rank));
        }
        t = ht;
    }
    for(auto &x : f[logn-1][0])
    {
        output.push_back(x);
    }
}

void fft_attack_test(int testmaxnum,int mode)
{
    int ans[512];
    fpr arr[512];
    fpr arr_save[512];
    int logn = 9;
    int n = 1<<logn;
    vector<int> ans_v;
    vector<state_fft> cand;
    unsigned seed = (unsigned)time(NULL);
    srand(seed);
    int anscnt = 0;
    normal_distribution<double> distribution(0.0,4.053163803303075);
    default_random_engine generator;
    generator.seed(seed);
    clock_t starttime = clock();
    for(int testnum = 0; testnum < testmaxnum ; testnum++)
    {
        if (testnum % 10 == 0) {
            fprintf(stderr, "[postproc] test %d/%d\n", testnum + 1, testmaxnum);
        }
        bool ansflag = true;
        for(int i = 0 ; i < n ; i++)
        {
            if(mode == 1)
                ans[i] = int(distribution(generator));
            else
                ans[i] = (rand() % 63) - 31;
            arr[i] = fpr_of_leak(ans[i],ans_v);
            arr_save[i] = arr[i];
        }
        fft_leak(arr,ans_v,logn);
        fft_attack(ans_v,cand,logn);
        if(cand.size() != 1)
        {
            puts("make many cand"); // find many candidate
            for(int i = 0 ; i < n ; i++)
            {
                printf("%d ",ans[i]);
            }
            puts("");
        }
        for(auto &x : cand)
        {
            for(int i = 0 ; i < n ; i++)
            {
                if(arr_save[i] != x.v[i])
                {
                    printf("Wrong %d : %016llx %016llx\n",i,arr_save[i],x.v[i]);// find wrong answer
                    ansflag = false;
                }
            }
        }
        if(ansflag) anscnt++;
        ans_v.clear();
        cand.clear();
    }
    clock_t endtime = clock();
    printf("Find answer : %d/%d\n",anscnt,testmaxnum);
    printf("time : %lf ,mean time : %lf\n",(double)(endtime - starttime) / CLOCKS_PER_SEC,(double)(endtime - starttime) / testmaxnum/ CLOCKS_PER_SEC);
}

void fft_attack_getmaxrank_test(int testmaxnum,int wrongnum)
{
    int ans[512];
    fpr arr[512];
    fpr arr_save[512];
    int logn = 9;
    int n = 1<<logn;
    vector<int> ans_v;
    vector<state_fft> cand;
    srand(time(NULL));
    int anscnt = 0;
    normal_distribution<double> distribution(0.0,4.053163803303075);
    default_random_engine generator;
    clock_t starttime = clock();
    for(int i = 0 ; i < n ; i++)
    {
        ans[i] = int(distribution(generator));
    }
    vector<int> ansrank_v;
    vector<int> ansmaxrank_v;

    for(int testnum = 0; testnum < testmaxnum ; testnum++)
    {
        if (testnum % 10 == 0) {
            fprintf(stderr, "[maxrank] test %d/%d\n", testnum + 1, testmaxnum);
        }
        for(int i = 0 ; i < n ; i++)
        {
            arr[i] = fpr_of_leak(ans[i],ans_v);
            arr_save[i] = arr[i];
        }
        fft_leak(arr,ans_v,logn);
        int maxerr = (int)ans_v.size();
        if (wrongnum > maxerr) wrongnum = maxerr;
        vector<int> idx(ans_v.size());
        iota(idx.begin(), idx.end(), 0);
        random_shuffle(idx.begin(), idx.end());
        for(int i = 0 ; i < wrongnum ; i++)
        {
            int linenum = idx[i];
            ans_v[linenum] = 64 - ans_v[linenum];
        }
        int keymaxrank = fft_attack_getmaxrank(ans_v,cand,logn,arr_save);
        ansmaxrank_v.push_back(keymaxrank);
        bool ansflag = true;
        int ansrank = -1;
        for(int i = 0 ; i < cand.size() ; i++)
        {
            auto &x = cand[i];
            if(x.isans)
                ansrank = i;
        }
        ansrank_v.push_back(ansrank);
        ans_v.clear();
        cand.clear();
    }
    clock_t endtime = clock();
    printf("time : %lf ,mean time : %lf\n",(double)(endtime - starttime) / CLOCKS_PER_SEC,(double)(endtime - starttime) / testmaxnum/ CLOCKS_PER_SEC);
    int ansval = 0;
    int ansmaxrank = 0;
    int nofind = 0;
    for(auto &x : ansrank_v)
    {
        ansval += x;
    }
    for(auto &x : ansmaxrank_v)
    {
        ansmaxrank += x;
    }
    if(testmaxnum - nofind == 0)
        printf("%d/%d , mean : no find\n",testmaxnum-nofind,testmaxnum);
    else
    {
        // mean : mean of output rank, mean maxkey : Average of the maximum rank observed during the post-processing procedure
        printf("%d/%d , mean : %lf, mean maxrank : %lf\n",testmaxnum-nofind,testmaxnum,(double)(ansval)/(testmaxnum - nofind),(double)(ansmaxrank) / (testmaxnum - nofind));
    }
}

void fft_attack_ranktest(int testmaxnum, int rank, const char *sidechannel_path, const char *input_path, const char *log_path)
{
    int logn = 9;
    int n = 1<<logn;
    vector<fpr> ans;
    vector<int> ans_v;
    vector<state_fft> cand;
    int anscnt = 0;
    clock_t starttime = clock();
    FILE *fp = fopen(sidechannel_path,"r");
    FILE *fp_input = fopen(input_path,"r");
    FILE *flog = stdout;
    if (log_path != NULL) {
        flog = fopen(log_path, "w");
        if (flog == NULL) {
            fprintf(stderr, "error: failed to open log file: %s\n", log_path);
            if (fp) fclose(fp);
            if (fp_input) fclose(fp_input);
            return;
        }
    }
    if (fp == NULL || fp_input == NULL) {
        fprintf(stderr, "error: failed to open input files: %s , %s\n", sidechannel_path, input_path);
        if (fp) fclose(fp);
        if (fp_input) fclose(fp_input);
        if (flog != stdout) fclose(flog);
        return;
    }
    int val;
    int first_success_test = -1;
    int first_success_rank = -1;

    for(int testnum = 0; testnum < testmaxnum ; testnum++)
    {
        if (testnum % 10 == 0) {
            fprintf(stderr, "[rank] test %d/%d\n", testnum + 1, testmaxnum);
        }
        for(int i = 0 ; i < 53248 ; i++)
        {
            if (fscanf(fp,"%d",&val) != 1) {
                fprintf(stderr, "error: failed to read sidechannel at test %d\n", testnum+1);
                goto done;
            }
            ans_v.push_back(val);
        }
        //vector<int> ans_v_side(ans_v.begin()+53248 * testnum,ans_v.begin()+53248 * (testnum+1));
        fft_attack_rank(ans_v,cand,logn,rank);
        int candsz = cand.size();
        for(int i = 0 ; i < 512 ; i++)
        {
            if (fscanf(fp_input,"%d",&val) != 1) {
                fprintf(stderr, "error: failed to read inputdata at test %d\n", testnum+1);
                goto done;
            }
            fpr x = fpr_of(val);
            ans.push_back(x);
        }
        int found_rank = -1;
        for(int i = 0 ; i < candsz ; i++)
        {
            int flag = 0;
            for(int j = 0 ; j < 512 ; j++)
            {
                if(cand[i].v[j] != ans[j])
                {
                    flag = 1;
                    break;
                }
            }
            if(flag) continue;
            found_rank = i;
            if (first_success_test < 0) {
                first_success_test = testnum + 1;
                first_success_rank = i;
            }
            break;
        }
        if (found_rank < 0) {
            int best_idx = -1;
            int best_mismatch = 1e9;
            for (int i = 0; i < candsz; i++) {
                int mismatch = 0;
                for (int j = 0; j < 512; j++) {
                    if (cand[i].v[j] != ans[j]) mismatch++;
                }
                if (mismatch < best_mismatch) {
                    best_mismatch = mismatch;
                    best_idx = i;
                }
            }
            fprintf(stderr, "[rank][debug] test %d: no exact match, best_idx=%d mismatch=%d\n",
                    testnum + 1, best_idx, best_mismatch);
        } else {
            fprintf(stderr, "[rank][debug] test %d: exact match at rank=%d\n",
                    testnum + 1, found_rank);
        }
        if (found_rank >= 0) anscnt++;
        fprintf(flog, "test %d: %s (rank=%d, cand=%d)\n",
                testnum + 1,
                (found_rank >= 0 ? "success" : "fail"),
                found_rank,
                candsz);
        ans_v.clear();
        ans.clear();
        cand.clear();
    }
done:
    fclose(fp);
    fclose(fp_input);
    if (flog != stdout) fclose(flog);
    clock_t endtime = clock();
    printf("success : %d/%d\n", anscnt, testmaxnum);
    printf("time : %lf ,mean time : %lf\n",(double)(endtime - starttime) / CLOCKS_PER_SEC,(double)(endtime - starttime) / testmaxnum/ CLOCKS_PER_SEC);
}

void fft_attack_pr_ranktest(int testmaxnum, int rank, const char *sidechannel_path, const char *input_path, const char *log_path)
{
    int logn = 9;
    int n = 1<<logn;
    vector<fpr> ans;
    vector<double> ans_v;
    vector<state_fft_pr> cand;
    int anscnt = 0;
    clock_t starttime = clock();
    FILE *fp = fopen(sidechannel_path,"r");
    FILE *fp_input = fopen(input_path,"r");
    FILE *flog = stdout;
    if (log_path != NULL) {
        flog = fopen(log_path, "w");
        if (flog == NULL) {
            fprintf(stderr, "error: failed to open log file: %s\n", log_path);
            if (fp) fclose(fp);
            if (fp_input) fclose(fp_input);
            return;
        }
    }
    if (fp == NULL || fp_input == NULL) {
        fprintf(stderr, "error: failed to open input files: %s , %s\n", sidechannel_path, input_path);
        if (fp) fclose(fp);
        if (fp_input) fclose(fp_input);
        if (flog != stdout) fclose(flog);
        return;
    }
    int val;
    double val_f;
    int first_success_test = -1;
    int first_success_rank = -1;

    for(int testnum = 0; testnum < testmaxnum ; testnum++)
    {
        if (testnum % 10 == 0) {
            fprintf(stderr, "[pr_rank] test %d/%d\n", testnum + 1, testmaxnum);
        }
        for(int i = 0 ; i < 53248 ; i++)
        {
            if (fscanf(fp,"%lf",&val_f) != 1) {
                fprintf(stderr, "error: failed to read sidechannel at test %d\n", testnum+1);
                goto done;
            }
            ans_v.push_back(val_f);
        }
        ans.clear();
        for(int i = 0 ; i < 512 ; i++)
        {
            if (fscanf(fp_input,"%d",&val) != 1) {
                fprintf(stderr, "error: failed to read inputdata at test %d\n", testnum+1);
                goto done;
            }
            fpr x = fpr_of(val);
            ans.push_back(x);
        }
        fft_attack_pr_rank(ans_v,cand,logn,rank);
        int candsz = cand.size();
        int found_rank = -1;
        for(int i = 0 ; i < candsz ; i++)
        {
            int flag = 0;
            for(int j = 0 ; j < 512 ; j++)
            {
                if(cand[i].v[j] != ans[j])
                {
                    flag = 1;
                    break;
                }
            }
            if(flag) continue;
            found_rank = i;
            if (first_success_test < 0) {
                first_success_test = testnum + 1;
                first_success_rank = i;
            }
            break;
        }
        if (found_rank < 0) {
            int best_idx = -1;
            int best_mismatch = 1e9;
            for (int i = 0; i < candsz; i++) {
                int mismatch = 0;
                for (int j = 0; j < 512; j++) {
                    if (cand[i].v[j] != ans[j]) mismatch++;
                }
                if (mismatch < best_mismatch) {
                    best_mismatch = mismatch;
                    best_idx = i;
                }
            }
            fprintf(stderr, "[pr_rank][debug] test %d: no exact match, best_idx=%d mismatch=%d\n",
                    testnum + 1, best_idx, best_mismatch);
        } else {
            fprintf(stderr, "[pr_rank][debug] test %d: exact match at rank=%d\n",
                    testnum + 1, found_rank);
        }
        if (found_rank >= 0) anscnt++;
        fprintf(flog, "test %d: %s (rank=%d, cand=%d)\n",
                testnum + 1,
                (found_rank >= 0 ? "success" : "fail"),
                found_rank,
                candsz);
        ans_v.clear();
        ans.clear();
        cand.clear();
    }
done:
    fclose(fp);
    fclose(fp_input);
    if (flog != stdout) fclose(flog);
    clock_t endtime = clock();
    printf("success : %d/%d\n", anscnt, testmaxnum);
    if (first_success_test >= 0) {
        printf("first success : test %d , rank %d\n", first_success_test, first_success_rank);
    }
    printf("time : %lf ,mean time : %lf\n",(double)(endtime - starttime) / CLOCKS_PER_SEC,(double)(endtime - starttime) / testmaxnum/ CLOCKS_PER_SEC);
}

static void print_usage(const char *prog)
{
    printf("usage:\n");
    printf("  %s postproc <testmaxnum> <mode(0=uniform,1=gaussian)> [log.txt]\n", prog);
    printf("  %s maxrank <testmaxnum> <wrongnum> [log.txt]\n", prog);
    printf("  %s rank <testmaxnum> <rank> <sidechannel.txt> <inputdata_of.txt> [log.txt]\n", prog);
    printf("  %s pr_rank <testmaxnum> <rank> <sidechannel_pr.txt> <inputdata_of.txt> [log.txt]\n", prog);
    printf("  %s maxrank_pr <testmaxnum> <sigma> [log.txt]\n", prog);
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    string mode = argv[1];
    if (mode == "postproc") {
        if (argc < 4) {
            print_usage(argv[0]);
            return 1;
        }
        int testmaxnum = atoi(argv[2]);
        int dist_mode = atoi(argv[3]);
        if (argc >= 5) {
            if (freopen(argv[4], "w", stdout) == NULL) {
                fprintf(stderr, "error: failed to open log file: %s\n", argv[4]);
                return 1;
            }
        }
        fft_attack_test(testmaxnum, dist_mode);
        return 0;
    }
    if (mode == "maxrank") {
        if (argc < 4) {
            print_usage(argv[0]);
            return 1;
        }
        int testmaxnum = atoi(argv[2]);
        int wrongnum = atoi(argv[3]);
        if (argc >= 5) {
            if (freopen(argv[4], "w", stdout) == NULL) {
                fprintf(stderr, "error: failed to open log file: %s\n", argv[4]);
                return 1;
            }
        }
        fft_attack_getmaxrank_test(testmaxnum, wrongnum);
        return 0;
    }
    if (mode == "rank") {
        if (argc < 6) {
            print_usage(argv[0]);
            return 1;
        }
        int testmaxnum = atoi(argv[2]);
        int rank = atoi(argv[3]);
        const char *sidechannel_path = argv[4];
        const char *input_path = argv[5];
        const char *log_path = (argc >= 7) ? argv[6] : NULL;
        fft_attack_ranktest(testmaxnum, rank, sidechannel_path, input_path, log_path);
        return 0;
    }
    if (mode == "pr_rank") {
        if (argc < 6) {
            print_usage(argv[0]);
            return 1;
        }
        int testmaxnum = atoi(argv[2]);
        int rank = atoi(argv[3]);
        const char *sidechannel_path = argv[4];
        const char *input_path = argv[5];
        const char *log_path = (argc >= 7) ? argv[6] : NULL;
        printf("%d %d %s %s %s\n",testmaxnum,rank,sidechannel_path,input_path,log_path); //debug
        fft_attack_pr_ranktest(testmaxnum, rank, sidechannel_path, input_path, log_path);
        return 0;
    }
    if (mode == "maxrank_pr") {
        if (argc < 4) {
            print_usage(argv[0]);
            return 1;
        }
        int testmaxnum = atoi(argv[2]);
        double sigma = atof(argv[3]);
        if (argc >= 5) {
            if (freopen(argv[4], "w", stdout) == NULL) {
                fprintf(stderr, "error: failed to open log file: %s\n", argv[4]);
                return 1;
            }
        }
        fft_attack_getmaxrank_pr_test(testmaxnum, sigma);
        return 0;
    }

    print_usage(argv[0]);
    return 1;
}
