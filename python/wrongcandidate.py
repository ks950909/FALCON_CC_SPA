import numpy as np
import matplotlib.pyplot as plt

N = 51
outputtrace = np.zeros((N))
xspace = np.array(range(0,5100,100))
# array value is result of fft_attack_genmaxrank_test on main.cxx file
outputtrace[0] = 29.0
outputtrace[1] = 124.16
outputtrace[2] = 147.50
outputtrace[3] = 172.05
outputtrace[4] = 221.64
outputtrace[5] = 257.73
outputtrace[6] = 269.27
outputtrace[7] = 332.91
outputtrace[8] = 369.02
outputtrace[9] = 387.03
outputtrace[10] = 454.38
outputtrace[11] = 480.73
outputtrace[12] = 485.92
outputtrace[13] = 559.42
outputtrace[14] = 573.17
outputtrace[15] = 654.14
outputtrace[16] = 648.24
outputtrace[17] = 681.09
outputtrace[18] = 740.33
outputtrace[19] = 785.36
outputtrace[20] = 795.20
outputtrace[21] = 773.91
outputtrace[22] = 965.60
outputtrace[23] = 882.53
outputtrace[24] = 924.36
outputtrace[25] = 997.08
outputtrace[26] = 1032.16
outputtrace[27] = 1049.78
outputtrace[28] = 1101.98
outputtrace[29] = 1176.55
outputtrace[30] = 1122.01
outputtrace[31] = 1157.16
outputtrace[32] = 1097.75
outputtrace[33] = 1204.19
outputtrace[34] = 1157.74
outputtrace[35] = 1347.53
outputtrace[36] = 1486.44
outputtrace[37] = 1455.38
outputtrace[38] = 1512.94
outputtrace[39] = 1756.93
outputtrace[40] = 1489.50
outputtrace[41] = 1540.69
outputtrace[42] = 1411.13
outputtrace[43] = 2228.99
outputtrace[44] = 1905.87
outputtrace[45] = 1882.01
outputtrace[46] = 1946.89
outputtrace[47] = 2276.64
outputtrace[48] = 2079.34
outputtrace[49] = 2078.25
outputtrace[50] = 2291.56
plt.clf()
plt.xlabel('Incorrect Candidate information')
plt.ylabel('Average maximum rank')
plt.plot(xspace,outputtrace)
plt.savefig("wrongcandidate.pdf",format='pdf')
