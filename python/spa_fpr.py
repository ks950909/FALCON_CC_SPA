import numpy as np
import matplotlib.pyplot as plt
from matplotlib.colors import ListedColormap

inputfoldername = "./"
savefoldername = "./"
savefoldername_sidechannel = "../PostProcessing/"

trace_add = np.load(inputfoldername + "trace_fpr_add.npy")
pt_add_t = np.load(inputfoldername + "pt_fpr_add.npy")
ct_add_t = np.load(inputfoldername + "ct_fpr_add.npy")
trace_of = np.load(inputfoldername + "trace_fpr_of.npy")
pt_of_t = np.load(inputfoldername + "pt_fpr_of.npy")
ct_of_t = np.load(inputfoldername + "ct_fpr_of.npy")
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
with open(inputfoldername + "pt_add.txt", "w") as f:
    for i in range(N_add):
        f.write(hex(pt_add[i][0])[2:])
        f.write(" ")
        f.write(hex(pt_add[i][1])[2:])
        f.write(" ")
        f.write(hex(ct_add[i][0])[2:])
        f.write("\n")
del i, f
with open(inputfoldername + "pt_of.txt", "w") as f:
    for i in range(N_of):
        f.write(hex(pt_of[i][0])[2:])
        f.write(" ")
        f.write(hex(ct_of[i][0])[2:])
        f.write("\n")
del i, f
#%%
addgroupnum = 8
addgroupdata = np.zeros((N_add, addgroupnum), dtype=np.uint64)
with open(inputfoldername + "add_group.txt", "r") as f:
    data = f.readlines()
for i in range(N_add):
    aa = data[i].split(' ')
    for j in range(addgroupnum):
        addgroupdata[i, j] = aa[j]
del f, inputfoldername, aa, data, i, j

ofgroupnum = 8
ofgroupdata = np.zeros((N_of, ofgroupnum), dtype=np.uint64)
with open(inputfoldername + "of_group.txt", "r") as f:
    data = f.readlines()
for i in range(N_of):
    aa = data[i].split(' ')
    for j in range(ofgroupnum):
        ofgroupdata[i, j] = aa[j]
del f, aa, data, i, j

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
plt.savefig(savefoldername + "fpr_snr.pdf",bbox_inches='tight',format='pdf',dpi=600)

f = open(savefoldername+"recovery_bit_fpr.txt","w")# record recovery bit of conditional calculator
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
        f.write("recovery : "+str(anscnt)+ " / "+ str(N)+"\n")# print recovery bit of conditional calculator
    else:
        for i in range(N):
            group2.append('b')
        f.write("recovery : "+str(N)+ " / "+ str(N)+"\n")# print recovery bit of conditional calculator but fixed value

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
sidechannel_of = np.zeros(([8,10240]),dtype=np.uint8)
sidechannel_add = np.zeros(([8,122880]),dtype=np.uint8)
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
plt.savefig(savefoldername + "fpr_scatter.png",format='png',dpi=600)
f.close()
g = open(savefoldername_sidechannel+"sidechannel.txt","w")# record recovery bit of conditional calculator
sof = np.transpose(sidechannel_of,(1,0))
sadd = np.transpose(sidechannel_add,(1,0))
for tN in range(20):
    for i in range(512):
        for j in range(8):
            g.write(str(sof[tN*512+i,j]*64)+" ")
        g.write("\n")
    for i in range(6144):
        for j in range(8):
            g.write(str(sadd[tN*6144+i,j]*64)+" ")
        g.write("\n")
g.close()
