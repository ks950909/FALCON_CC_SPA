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
#include <random>
#include <string>
#include "fft.hxx"
using namespace std;

const int LOGN = 5;
const int N = 32;

struct state_fft
{
    int th = 1e9;
    bool isans = false;
    uint16_t len = 0;
    array<fpr,N> f;
    bool operator<(const state_fft& s) const{
        return this->th < s.th;
    }
};
struct WorseByTh{
    bool operator()(const state_fft& a,const state_fft &b) const{
        return a.th < b.th;
    }
};

inline void try_keep(priority_queue<state_fft, vector<state_fft>,WorseByTh>& heap,state_fft &&s, int K_KEEP)
{
    if ((int)heap.size() < K_KEEP) heap.push(move(s));
    else if(s.th < heap.top().th) {heap.pop(); heap.push(move(s));}
}

struct LayerStat{
    int L;
    size_t blocks;
    size_t max_pairs;
    size_t max_kept;
    uint64_t max_rank_upper;
    size_t blowup_blocks;
};

struct AttackPrintOpt{
    int K = 50000;
    int verbose = 1;
    int eq_keep = 0;
    uint64_t max_pairs_eval = 50000000ULL;
};
struct LayerSummary{
    int L = 0;
    size_t blocks = 0;

    size_t max_pairs = 0;
    size_t max_kept = 0;
    size_t sat_blocks = 0;
    size_t skipped_blocks = 0;

    uint64_t max_rank_lower = 0;
    uint64_t max_rank_upper = 0;
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


void fft_attack_getmaxrank_layer(
    vector<int> &ans_v,
    unsigned logn,
    fpr ans[],
    const AttackPrintOpt& opt,
    vector<LayerSummary>* layer_summaries
)
{
    unsigned u;
    size_t t,n,hn,m,qn;
    n = (size_t) 1 << logn;
    hn = n >> 1;
    qn = n >> 2;

    const int fpr_of_leaknum = 8;
    const int fpr_add_leaknum = 8 * 6;

    vector<vector<vector<state_fft>>> f;
    vector<vector<state_fft>> tmp;
    f.push_back(tmp);
    fpr arr[6];
    vector<int> v;
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
                tttmp.f[0] = jre_fpr;
                tttmp.f[1] = jim_fpr;
                if(jre_fpr == ans[i] && jim_fpr == ans[i + hn])
                    tttmp.isans = true;
                else
                    tttmp.isans = false;
                ttmp_arr.push_back(move(tttmp));
            }
        }
        std::sort(ttmp_arr.begin(),ttmp_arr.end(),[](const state_fft &x,const state_fft &y) -> bool{
            return x.th < y.th;
        });

        int sz = ttmp_arr.size();
        size_t anssz = 0;
        bool found = false;

        for(size_t j = 0 ; j < sz ; j++)
        {
            if(ttmp_arr[j].isans)
            {
                anssz = j;
                found = true;
                break;
            }
        }
        if(!found)
        {
            anssz = sz - 1;
        }
        if(opt.verbose >= 2){
            printf("fprof cand %d : %d of %zu\n",i,63*63,anssz+1);
        }
        size_t needed = anssz + 1;
        vector<state_fft> keep;
        if((size_t) opt.K >= needed){
            keep.assign(ttmp_arr.begin(), ttmp_arr.begin() + needed);
        }
        else{
            size_t knon = (opt.K > 0) ? (size_t)(opt.K - 1) : 0;
            keep.assign(ttmp_arr.begin(), ttmp_arr.begin() + min(knon, ttmp_arr.size()));
            bool has_true = false;
            for (auto &s : keep) if (s.isans) { has_true = true; break; }
            if (!has_true) keep.push_back(ttmp_arr[anssz]);
        }
        f[0].push_back(keep);
    }

    t = hn;
    for (u = 1, m = 2; u < logn; u ++, m <<= 1) {
        size_t ht, hm, i1, j1, tm;
        f.push_back(tmp);
        ht = t >> 1;
        hm = m >> 1;
        tm = m << 1;
        LayerSummary ls;
        ls.L = (int)u;

        for (size_t j = 0 ; j < ht ; j++)
        {
            // j, j+ht
            vector<state_fft> &x_arr = f[u-1][j];
            vector<state_fft> &y_arr = f[u-1][j + ht];

            const uint64_t pairs = (uint64_t)x_arr.size() * (uint64_t) y_arr.size();
            ls.blocks++;
            ls.max_pairs = max(ls.max_pairs, (size_t)min<uint64_t>(pairs,(uint64_t)SIZE_MAX));

            vector<int> cnt_arr;
            vector<state_fft> ttmp_arr;
            const state_fft* x_ans = nullptr;
            const state_fft* y_ans = nullptr;
            for(auto &x : x_arr)
                if(x.isans) {x_ans = &x; break;}
            for(auto &y : y_arr)
                if(y.isans) {y_ans = &y; break;}

            if(!x_ans || !y_ans)
            {
                if(opt.verbose >= 2){
                    printf("fft cand %u %u %zu: pairs=%llu (true missing)\n",
                        u, m, j, (unsigned long long)pairs);
                }
                f[u].push_back({});
                continue;
            }
            
            int cnt_ans = x_ans->th + y_ans->th;
            state_fft true_state;
            bool have_true = false;
            true_state.isans = true;
            const int K_NONTRUE = (opt.K > 0) ? (opt.K - 1) : 0;

            vector<fpr> true_re,true_im;
            true_re.reserve(m);
            true_im.reserve(m);

            for(i1 = 0 ; i1 < hm ; i1++)
            {
                arr[0] = x_ans->f[i1   ];
                arr[1] = x_ans->f[i1+hm];
                arr[2] = y_ans->f[i1   ];
                arr[3] = y_ans->f[i1+hm];
                arr[4] = fpr_gm_tab[((m + i1) << 1) + 0];
                arr[5] = fpr_gm_tab[((m + i1) << 1) + 1];

                fft_one_layer_leak(arr,v);

                true_re.push_back(arr[0]);
                true_im.push_back(arr[1]);
                true_re.push_back(arr[2]);
                true_im.push_back(arr[3]);

                for(int i = 0 ; i < fpr_add_leaknum ; i++)
                {
                    cnt_ans += (v[i] != ans_v[fpr_of_leaknum * n + fpr_add_leaknum * qn *(u-1)+ i1*fpr_add_leaknum * ht + j * fpr_add_leaknum + i]);
                }
                v.clear();
            }
            for(size_t k = 0 ; k < (size_t)m ; k++) true_state.f[k] = true_re[k];
            for(size_t k = 0 ; k < (size_t)m ; k++) true_state.f[k+m] = true_im[k];
            true_state.th = cnt_ans;

            uint64_t rank_lower = 1;
            uint64_t rank_upper = 1;
            bool rank_computed = true;
            
            if(pairs > opt.max_pairs_eval){
                ls.skipped_blocks++;
                rank_computed = false;

                if(opt.verbose >= 2)
                {
                    printf("fft cand %u %u %zu: pairs=%llu SKIP(max_pairs_eval=%llu), kept=1(true)\n",
                        u, m, j,
                        (unsigned long long)pairs,
                        (unsigned long long)opt.max_pairs_eval);
                }
                vector<state_fft> keep;
                keep.reserve(1);
                keep.push_back(move(true_state));
                f[u].push_back(move(keep));
                continue;
            }

            auto worse = [](const state_fft& a, const state_fft& b){ return a.th < b.th; }; // max-heap
            priority_queue<state_fft, vector<state_fft>, decltype(worse)> heap(worse);

            auto try_keep = [&](state_fft&& s){
                if ((int)heap.size() < K_NONTRUE) heap.push(move(s));
                else if (s.th < heap.top().th) { heap.pop(); heap.push(move(s)); }
            };

            uint64_t cnt_less = 0;
            uint64_t cnt_eq = 0;
            int eq_kept = 0;
            vector<fpr> ff;
            ff.reserve(2*m);

            for(auto &x : x_arr)
            {
                for(auto &y : y_arr)
                {
                    int cnt = x.th + y.th;

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
                    if(cnt < cnt_ans) cnt_less++;
                    else if(cnt == cnt_ans) cnt_eq++;

                    state_fft tttmp;
                    tttmp.th = cnt;
                    tttmp.isans = x.isans & y.isans;
                    for(int i = 0 ; i < m ; i++)
                        tttmp.f[i] = ff[i * 2];
                    for(int i = 0 ; i < m ; i++)
                        tttmp.f[m+i] = ff[i * 2 + 1];
                    if(tttmp.isans){
                        true_state = tttmp;
                        have_true = true;
                        continue;
                    }


                    if(cnt < cnt_ans)
                        try_keep(move(tttmp));
                    else if(cnt == cnt_ans && opt.eq_keep > 0 && eq_kept < opt.eq_keep)
                    {
                        eq_kept++;
                        try_keep(move(tttmp));
                    }
                    ff.clear();
                }
            }
            rank_lower = cnt_less + 1;
            rank_upper = cnt_less + cnt_eq ;

            if (rank_upper > pairs) {
                printf("BUG: rank_upper > pairs (u=%u m=%u j=%zu)\n", u, m, j);
                printf("pairs=%llu less=%llu eq=%llu rank=[%llu,%llu]\n",
                    (unsigned long long)pairs,
                    (unsigned long long)cnt_less,
                    (unsigned long long)cnt_eq,
                    (unsigned long long)rank_lower,
                    (unsigned long long)rank_upper);
                exit(1); 
            }

            vector<state_fft> keep;
            keep.reserve(heap.size() + 1);
            while(!heap.empty()){
                keep.push_back(heap.top());
                heap.pop();
            }

            std::sort(keep.begin(),keep.end(),[](const state_fft &x,const state_fft &y) -> bool{
                return x.th < y.th;
            });
            keep.push_back(move(true_state));

            const size_t keptN = keep.size();
            ls.max_kept = max(ls.max_kept,keptN);
            if((int)keptN >= opt.K) ls.sat_blocks += 1;

            ls.max_rank_lower = max<uint64_t>(ls.max_rank_lower,rank_lower);
            ls.max_rank_upper = max<uint64_t>(ls.max_rank_upper,rank_upper);

            if (opt.verbose >= 2) {
                printf("fft cand %u %u %zu: pairs=%llu kept=%zu rank=[%llu,%llu]\n",
                    u, m, j,
                    (unsigned long long)pairs,
                    keptN,
                    (unsigned long long)rank_lower,
                    (unsigned long long)rank_upper);
            }

            f[u].push_back(move(keep));
        }
        if (opt.verbose >= 1) {
            printf("[Layer %d] blocks=%zu max_pairs=%zu max_kept=%zu sat=%zu skipped=%zu max_rank=[%llu,%llu]\n",
                ls.L,
                ls.blocks,
                ls.max_pairs,
                ls.max_kept,
                ls.sat_blocks,
                ls.skipped_blocks,
                (unsigned long long)ls.max_rank_lower,
                (unsigned long long)ls.max_rank_upper);
        }
        if(layer_summaries) layer_summaries ->push_back(ls);
        t = ht;
    }
}

void fft_attack_getmaxrank_test_layer(int testmaxnum,int wrongnum, AttackPrintOpt &opt)
{
    int ans[32];
    fpr arr[32];
    fpr arr_save[32];
    int logn = LOGN;
    int n = N;
    vector<int> ans_v;
    int anscnt = 0;
    mt19937 rng(123456);
    normal_distribution<double> distribution(0.0,4.053163803303075);
    clock_t starttime = clock();
    for(int i = 0 ; i < n ; i++)
    {
        ans[i] = int(distribution(rng));
    }
    vector<LayerSummary> sums;

    for(int testnum = 0; testnum < testmaxnum ; testnum++)
    {
        for(int i = 0 ; i < n ; i++)
        {
            arr[i] = fpr_of_leak(ans[i],ans_v);
            arr_save[i] = arr[i];
        }
        fft_leak(arr,ans_v,logn);
        printf("Error: %d/%d\n",wrongnum,ans_v.size());
        vector<int> idx(ans_v.size());
        iota(idx.begin(),idx.end(),0);
        shuffle(idx.begin(),idx.end(),rng);
        for(int i = 0 ; i < wrongnum ; i++)
        {
            ans_v[idx[i]] = 64 - ans_v[idx[i]];
        }
        fft_attack_getmaxrank_layer(ans_v,logn,arr_save,opt,&sums);


        ans_v.clear();
    }
    clock_t endtime = clock();
    printf("time : %lf ,mean time : %lf\n",(double)(endtime - starttime) / CLOCKS_PER_SEC,(double)(endtime - starttime) / testmaxnum/ CLOCKS_PER_SEC);
}
int main(int argc, char* argv[])
{
    int testmaxnum = 20;
    int wrongnum   = 505;
    int K          = 10000;
    unsigned long long max_pairs = 1000000ULL;
    int eq_keep    = 100;
    int verbose = 1;
    const char* log_path = nullptr;

    if (argc >= 2) testmaxnum = std::atoi(argv[1]);
    if (argc >= 3) wrongnum   = std::atoi(argv[2]);
    if (argc >= 4) K          = std::atoi(argv[3]);
    if (argc >= 5) max_pairs  = std::strtoull(argv[4], nullptr, 10);
    if (argc >= 6) eq_keep    = std::atoi(argv[5]);
    if (argc >= 7) verbose    = std::atoi(argv[6]);
    if (argc >= 8) log_path   = argv[7];

    if (log_path != nullptr) {
        if (freopen(log_path, "w", stdout) == nullptr) {
            fprintf(stderr, "error: failed to open log file: %s\n", log_path);
            return 1;
        }
    }

    printf("argv : %d %d %d %lld %d %d\n",testmaxnum,wrongnum,K,max_pairs,eq_keep,verbose);

    AttackPrintOpt opt;
    opt.K = K;
    opt.max_pairs_eval = max_pairs;
    opt.eq_keep = eq_keep;
    opt.verbose = verbose;

    fft_attack_getmaxrank_test_layer(testmaxnum,wrongnum,opt);
    return 0;
}
