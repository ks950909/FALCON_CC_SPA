import numpy as np
import matplotlib.pyplot as plt
from matplotlib.colors import ListedColormap

foldername = "D:\\github\\SASCA_FALCON\\data\\paper2\\"
#savefoldername = "C:\\Users\\KimGyuSang\\Dropbox\\대학원\\부채널세미나_자료\\논문작업\\2025-04\\image\\"
savefoldername = "D:\\github\\SASCA_FALCON\\paper\\image\\"

num = 9

trace_add = np.load(foldername + "trace_fpr_add_"+str(num)+".npy")
pt_add_t = np.load(foldername + "pt_fpr_add_"+str(num)+".npy")
ct_add_t = np.load(foldername + "ct_fpr_add_"+str(num)+".npy")
trace_of = np.load(foldername + "trace_fpr_of_"+str(num)+".npy")
pt_of_t = np.load(foldername + "pt_fpr_of_"+str(num)+".npy")
ct_of_t = np.load(foldername + "ct_fpr_of_"+str(num)+".npy")
def u8arr2u64(x, size):
    output = np.uint64(0)
    for i in range(size - 1, -1, -1):
        output = np.add(output, np.uint64(x[i]))
        if i != 0:
            output = np.left_shift(output, np.uint64(8))
    return output


N_add = pt_add_t.shape[0]
tracelen_add = trace_add.shape[1]
pt_add = np.zeros((N_add, 2), dtype=np.uint64)
ct_add = np.zeros((N_add, 1), dtype=np.uint64)

for i in range(N_add):
    pt_add[i, 0] = u8arr2u64(pt_add_t[i, 0: 8], 8)
    pt_add[i, 1] = u8arr2u64(pt_add_t[i, 8:16], 8)
    ct_add[i, 0] = u8arr2u64(ct_add_t[i, 0: 8], 8)

N_of = pt_of_t.shape[0]
tracelen_of = trace_of.shape[1]
pt_of = np.zeros((N_of, 1), dtype=np.uint64)
ct_of = np.zeros((N_of, 1), dtype=np.uint64)

for i in range(N_of):
    pt_of[i, 0] = u8arr2u64(pt_of_t[i, 0: 8], 8)
    ct_of[i, 0] = u8arr2u64(ct_of_t[i, 0: 8], 8)
foldername = "D:\\github\\SASCA_FALCON\\data\\paper2\\"
with open(foldername + "pt_add_"+str(num)+".txt", "w") as f:
    for i in range(N_add):
        f.write(hex(pt_add[i][0])[2:])
        f.write(" ")
        f.write(hex(pt_add[i][1])[2:])
        f.write(" ")
        f.write(hex(ct_add[i][0])[2:])
        f.write("\n")
del i, f, foldername
foldername = "D:\\github\\SASCA_FALCON\\data\\paper2\\"
with open(foldername + "pt_of_"+str(num)+".txt", "w") as f:
    for i in range(N_of):
        f.write(hex(pt_of[i][0])[2:])
        f.write(" ")
        f.write(hex(ct_of[i][0])[2:])
        f.write("\n")
del i, f, foldername
#%%
foldername = "D:\\github\\SASCA_FALCON\\data\\paper2\\"
addgroupnum = 8
addgroupdata = np.zeros((N_add, addgroupnum), dtype=np.uint64)
with open(foldername + "add_group_"+str(num)+".txt", "r") as f:
    data = f.readlines()
for i in range(N_add):
    aa = data[i].split(' ')
    for j in range(addgroupnum):
        addgroupdata[i, j] = aa[j]
del f, foldername, aa, data, i, j

foldername = "D:\\github\\SASCA_FALCON\\data\\paper2\\"
ofgroupnum = 8
ofgroupdata = np.zeros((N_of, ofgroupnum), dtype=np.uint64)
with open(foldername + "of_group_"+str(num)+".txt", "r") as f:
    data = f.readlines()
for i in range(N_of):
    aa = data[i].split(' ')
    for j in range(ofgroupnum):
        ofgroupdata[i, j] = aa[j]
del f, foldername, aa, data, i, j
##%%

def calculate_snr(traces, labels):
    ulabels = np.unique(labels)
    num_samples = traces.shape[1]
    mean_signal = np.zeros((len(ulabels), num_samples))
    variance_signal = np.zeros((len(ulabels), num_samples))
    for i, label in enumerate(ulabels):
        traces_label = traces[labels == label]
        mean_signal[i] = np.mean(traces_label, axis=0)
        variance_signal[i] = np.var(traces_label, axis=0)
    signal_power = np.var(mean_signal, axis=0)
    noise_power = np.mean(variance_signal, axis=0)
    noise_power[noise_power == 0] = np.finfo(float).eps
    snr = signal_power / noise_power
    return snr

addsnr_trace = np.zeros((8,tracelen_add))
for i in range(8):
    addsnr_trace[i] = calculate_snr(trace_add, addgroupdata[:,i])
ofsnr_trace = np.zeros((8,tracelen_of))
for i in range(8):
    ofsnr_trace[i] = calculate_snr(trace_of, ofgroupdata[:,i])
plt.clf()
for i in range(8):
    plt.subplot(4, 4,i+1)
    plt.plot(ofsnr_trace[i])
    plt.xticks([])
    plt.tick_params(axis='y', labelsize=4)
    #plt.yticks([])
for i in range(8):
    plt.subplot(4, 4,i+9)
    plt.plot(addsnr_trace[i])
    plt.xticks([])
    plt.tick_params(axis='y', labelsize=4)
    #plt.yticks([])
#plt.savefig(savefoldername + "fpr_snr_"+str(num)+".pdf",bbox_inches='tight',format='pdf',dpi=600)
#np.save(savefoldername+"fpr_add_snr_"+str(num)+"_trace",addsnr_trace)
#np.save(savefoldername+"fpr_of_snr_"+str(num)+"_trace",ofsnr_trace)
##%%
#f = open(savefoldername+"recovery_bit_fpr_"+str(num)+".txt","w")# record recovery bit of conditional calculator
def see_scatter(trace, idx, group, N,sn,sm, view):
    plt.subplot(sn, sm, view)
    group2 = []
    zero = 0
    zerocnt = 0
    one = 0
    onecnt = 0
    anscnt = 0
    for i in range(N):
        if group[i] == 0:
            zero += trace[i][idx]
            zerocnt += 1
        else:
            one += trace[i][idx]
            onecnt += 1
    if zerocnt != 0 and onecnt != 0:
        zero /= zerocnt
        one /= onecnt
        midval = (zero + one) / 2
        for i in range(N):
            if zero > one:
                if trace[i,idx] > midval:
                    group2.append('r') #0
                    if group[i] == 0:
                        anscnt+=1
                else:
                    group2.append('b') #-1
                    if group[i] == 1:
                        anscnt+=1
            else:
                if trace[i,idx] < midval:
                    group2.append('r') #0
                    if group[i] == 0:
                        anscnt+=1
                else:
                    group2.append('b') #-1
                    if group[i] == 1:
                        anscnt+=1
        #f.write("recovery : "+str(anscnt)+ " / "+ str(N)+"\n")# print recovery bit of conditional calculator
    else:
        for i in range(N):
            group2.append('b')
        #f.write("recovery : "+str(N)+ " / "+ str(N)+"\n")# print recovery bit of conditional calculator but fixed value

    plt.xlim([-2,1])
    group3 = []
    for i in range(group.shape[0]):
        group3.append(group[i] * -1)
    plt.xticks([-1,0])
    plt.tick_params(axis='x', labelsize=4)
    plt.tick_params(axis='y', labelsize=4)
    plt.scatter(group3,trace[:, idx],c=group2)
    return group2

plt.clf()
sidechannel_of = np.zeros(([8,N_of]),dtype=np.uint8)
sidechannel_add = np.zeros(([8,N_add]),dtype=np.uint8)
for view in range(8):
    tmp = see_scatter(trace_of, np.argmax(ofsnr_trace[view]), ofgroupdata[:, view], N_of, 4,4,view + 1 )
    for i in range(N_of):
        if tmp[i] == 'r':
            sidechannel_of[view,i] = 0
        else:
            sidechannel_of[view, i] = 1
for view in range(8):
    tmp = see_scatter(trace_add, np.argmax(addsnr_trace[view]), addgroupdata[:, view], N_add, 4,4,view + 1+8)
    for i in range(N_add):
        if tmp[i] == 'r':
            sidechannel_add[view, i] = 0
        else:
            sidechannel_add[view, i] = 1
#plt.savefig(savefoldername + "fpr_scatter_"+str(num)+".png",format='png',dpi=600)
#f.close()
#%%
g = open(savefoldername+"sidechannel_"+str(num)+".txt","w")# record recovery bit of conditional calculator
sof = np.transpose(sidechannel_of,(1,0))
sadd = np.transpose(sidechannel_add,(1,0))
for tN in range(N_add//6144):
    for i in range(512):
        for j in range(8):
            g.write(str(sof[tN*512+i,j]*64)+" ")
        g.write("\n")
    for i in range(6144):
        for j in range(8):
            g.write(str(sadd[tN*6144+i,j]*64)+" ")
        g.write("\n")
g.close()
#%%
sidechannel_of = np.zeros(([8,N_of]),dtype=np.float64)
sidechannel_add = np.zeros(([8,N_add]),dtype=np.float64)

import numpy as np

def post_p0_gauss(x, m0, s0, m1, s1, prior0=0.5, temperature=1.0):
    # log-likelihoods
    a = -np.log(s0) - 0.5*((x-m0)/s0)**2
    b = -np.log(s1) - 0.5*((x-m1)/s1)**2
    log_prior_ratio = np.log(prior0) - np.log(1-prior0)
    d = (b - a) - log_prior_ratio
    d = d / max(temperature, 1e-9)
    return 1.0 / (1.0 + np.exp(np.clip(d, -709, 709)))


for view in range(8):
    snr_idx = np.argmax(ofsnr_trace[view])
    N0 = 0
    N1 = 0
    m0 = 0
    m1 = 0
    s0 = 0
    s1 = 0
    for i in range(N_of):
        if ofgroupdata[i,view] == 0:
            N0 += 1
            m0 += trace_of[i,snr_idx]
            s0 += trace_of[i,snr_idx] * trace_of[i,snr_idx]
        else:
            N1 += 1
            m1 += trace_of[i, snr_idx]
            s1 += trace_of[i, snr_idx] * trace_of[i, snr_idx]
    if N0 == 0:
        for i in range(N_of):
            sidechannel_of[view,i] = 0.0
    else:
        m0 /= N0
        s0 /= N0
        m1 /= N1
        s1 /= N1
        s0 -= m0 * m0
        s1 -= m1 * m1
        s0 = np.sqrt(s0)
        s1 = np.sqrt(s1)
        print(view,m0,m1,s0,s1)
        for i in range(N_of):
            sidechannel_of[view,i] = post_p0_gauss(trace_of[i,snr_idx],m0,s0,m1,s1,prior0=0.5)

for view in range(8):
    snr_idx = np.argmax(addsnr_trace[view])
    N0 = 0
    N1 = 0
    m0 = 0
    m1 = 0
    s0 = 0
    s1 = 0
    for i in range(N_of):
        if addgroupdata[i,view] == 0:
            N0 += 1
            m0 += trace_add[i,snr_idx]
            s0 += trace_add[i,snr_idx] * trace_add[i,snr_idx]
        else:
            N1 += 1
            m1 += trace_add[i, snr_idx]
            s1 += trace_add[i, snr_idx] * trace_add[i, snr_idx]
    m0 /= N0
    s0 /= N0
    m1 /= N1
    s1 /= N1
    s0 -= m0 * m0
    s1 -= m1 * m1
    s0 = np.sqrt(s0)
    s1 = np.sqrt(s1)
    print(view, m0, m1, s0, s1)
    for i in range(N_add):
        sidechannel_add[view, i] = post_p0_gauss(trace_add[i, snr_idx], m0, s0, m1, s1, prior0=0.5)
g = open(savefoldername+"sidechannel_pr_"+str(num)+".txt","w")# record recovery bit of conditional calculator

sof = np.transpose(sidechannel_of,(1,0))
sadd = np.transpose(sidechannel_add,(1,0))
for tN in range(N_add//6144):
    for i in range(512):
        for j in range(8):
            s = sof[tN*512+i,j]
            g.write(f"{s:.18f} ")
        g.write("\n")
    for i in range(6144):
        for j in range(8):
            s = sadd[tN*6144+i,j]
            g.write(f"{s:.18f} ")
        g.write("\n")

g.close()
