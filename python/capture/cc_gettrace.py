import chipwhisperer as cw
import argparse
import numpy as np
import time
from tqdm import tqdm
import copy
from pathlib import Path
import yaml

def get_trace(N,ptlen,ctlen,pt_in,flag,scope,target,cfg):
    tracelen = 0
    pt = np.zeros((N,ptlen),dtype=np.uint8)
    ct = np.zeros((N,ctlen),dtype=np.uint8)
    for i in range(N):
        pt[i] = np.copy(pt_in[i])
    for i in tqdm(range(N)):
        scope.arm()
        target.simpleserial_write(flag,pt[i])
        tra = scope.capture()
        regack = target.simpleserial_read('r',ctlen)
        tr = scope.get_last_trace()
        if tracelen == 0:
            tracelen = scope.adc.trig_count
            trace = np.zeros((N,tracelen),dtype=np.float64)
            trace[i] = tr
        elif scope.adc.trig_count != tracelen: #Length test
            print("Length test Fail",i, scope.adc.trig_count, tracelen)
        else:
            trace[i] = tr
        ct[i] = copy.deepcopy(regack)
    for i in range(N): # functional test
        if pt[i,11] == 0:
            if ct[i,0] != pt[i,0]:
                print("No",pt,ct)
        else:
            if ct[i,0] != pt[i,4]:
                print("No",pt,ct)
    np.save(cfg["capture"]["trace_file"],trace)
    np.save(cfg["capture"]["pt_file"],pt)
    np.save(cfg["capture"]["ct_file"],ct)

def load_yaml(p: str) -> dict:
    with open(p, "r", encoding="utf-8") as f:
        return yaml.safe_load(f) or {}

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--config", default="configs/capture.yaml")
    args = ap.parse_args()
    cfg = load_yaml(args.config)

    out_file = Path(cfg["capture"]["trace_file"])
    out_file.parent.mkdir(parents=True, exist_ok = True)

    scope = cw.scope()
    target = cw.target(scope)
    scope.default_setup()

    if cfg["firmware"]["program"]:
        fw_path = cfg["firmware"]["hex_cc"]
        prog = cw.programmers.STM32FProgrammer
        cw.program_target(scope, prog, fw_path, baud=target.baud)
    N = cfg["capture"]["n_traces"]
    pt    = np.zeros((N,12),dtype=np.uint8)
    pt_m1 = np.zeros((N,12),dtype=np.uint8)
    pt_m2 = np.zeros((N,16),dtype=np.uint8)

    # x ^ ((x^y) & a)
    for j in range(N):
        for i in range(8): # set x,y
            pt   [j, i] = int(np.random.randint(0,256))
            pt_m1[j, i] = pt[j, i]
            pt_m2[j, i] = pt[j, i]
        x = np.random.randint(0,2) # set a
        if x == 1:
            for i in range(8,12):
                pt[j,i] = 0xff
            pt_m1[j, 11] = 1
            pt_m2[j, 11] = 1
        for i in range(12, 16):# set b,c at countermeasure 2
            pt_m2[j,i] = int(np.random.randint(0,256))

    time.sleep(0.1)
    get_trace(N,12,4,pt,'a',scope,target,cfg) 
    time.sleep(0.1)
    get_trace(N,12,4,pt_m1,'b',scope,target,cfg)
    time.sleep(0.1)
    get_trace(N,16,4,pt_m2,'c',scope,target,cfg)

    scope.dis()
if __name__ == "__main__":
    main()
