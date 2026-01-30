import argparse
import copy
import time
from pathlib import Path

import chipwhisperer as cw
import numpy as np
import yaml
from tqdm import tqdm


def load_yaml(p: str) -> dict:
    with open(p, "r", encoding="utf-8") as f:
        return yaml.safe_load(f) or {}


def get_trace_add(tracelen, foldername, n_st, n_ed, input_file, scope, target):
    scope.adc.samples = tracelen
    n = n_ed - n_st
    trace = np.zeros((n, tracelen), dtype=np.float64)
    pt = np.zeros((n, 16), dtype=np.uint8)
    ct = np.zeros((n, 8), dtype=np.uint8)

    with open(input_file, "r", encoding="utf-8") as f:
        data = f.readlines()

    for i in range(n_st, n_ed):
        x = data[i].split()
        tmp0 = np.uint64(int(x[0], 16))
        tmp1 = np.uint64(int(x[1], 16))
        for k in range(8):
            pt[i - n_st, k] = np.uint8(
                np.bitwise_and(np.right_shift(tmp0, np.uint64(8 * k)), np.uint64(0xFF))
            )
            pt[i - n_st, 8 + k] = np.uint8(
                np.bitwise_and(np.right_shift(tmp1, np.uint64(8 * k)), np.uint64(0xFF))
            )

    for i in tqdm(range(n)):
        scope.arm()
        target.simpleserial_write("a", pt[i])
        scope.capture()
        regack = target.simpleserial_read("r", 8)
        tr = scope.get_last_trace()
        trace[i] = tr
        if scope.adc.trig_count != tracelen:
            print(i, scope.adc.trig_count, tracelen)
        ct[i] = copy.deepcopy(regack)

    print(scope.adc.trig_count)
    np.save(Path(foldername) / f"trace_fpr_add_{n_st}_{n_ed}.npy", trace)
    np.save(Path(foldername) / f"pt_fpr_add_{n_st}_{n_ed}.npy", pt)
    np.save(Path(foldername) / f"ct_fpr_add_{n_st}_{n_ed}.npy", ct)


def get_trace_of(tracelen, foldername, n_st, n_ed, input_file, scope, target):
    scope.adc.samples = tracelen
    n = n_ed - n_st
    trace = np.zeros((n, tracelen), dtype=np.float64)
    pt = np.zeros((n, 8), dtype=np.uint8)
    ct = np.zeros((n, 8), dtype=np.uint8)

    with open(input_file, "r", encoding="utf-8") as f:
        data = f.readlines()

    for i in range(n_st // 512, n_ed // 512):
        x = data[i].split()
        for j in range(512):
            tmp = int(x[j])
            for k in range(8):
                pt[i * 512 + j - n_st, k] = np.uint8(
                    np.bitwise_and(np.right_shift(tmp, np.int64(8 * k)), np.int64(0xFF))
                )

    for i in tqdm(range(n)):
        scope.arm()
        target.simpleserial_write("b", pt[i])
        scope.capture()
        regack = target.simpleserial_read("r", 8)
        tr = scope.get_last_trace()
        trace[i] = tr
        if scope.adc.trig_count != tracelen:
            print(i, scope.adc.trig_count, tracelen)
        ct[i] = copy.deepcopy(regack)

    print(scope.adc.trig_count)
    np.save(Path(foldername) / f"trace_fpr_of_{n_st}_{n_ed}.npy", trace)
    np.save(Path(foldername) / f"pt_fpr_of_{n_st}_{n_ed}.npy", pt)
    np.save(Path(foldername) / f"ct_fpr_of_{n_st}_{n_ed}.npy", ct)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--config", default="configs/capture.yaml")
    args = ap.parse_args()
    cfg = load_yaml(args.config)

    scope = cw.scope()
    target = cw.target(scope)
    scope.default_setup()

    if cfg["firmware"]["program"]:
        fw_path = cfg["firmware"]["hex_falcon"]
        prog = cw.programmers.STM32FProgrammer
        cw.program_target(scope, prog, fw_path, baud=target.baud)

    output_dir = cfg.get("capture", {}).get("output_dir")
    if output_dir is None:
        output_dir = "trace/full"
    Path(output_dir).mkdir(parents=True, exist_ok=True)

    input_of = cfg.get("experiment", {}).get("inputs", {}).get("of_file")
    input_add = cfg.get("experiment", {}).get("inputs", {}).get("add_file")

    of_keys = cfg.get("capture", {}).get("of_keys", 1000)
    add_keys = cfg.get("capture", {}).get("add_keys", 1000)

    n_of = 512 * of_keys
    n_add = 6144 * add_keys

    tracelen_of = cfg.get("capture", {}).get("tracelen_of", 4188)
    tracelen_add = cfg.get("capture", {}).get("tracelen_add", 6460)

    n_st_of = 0
    n_ed_of = n_of
    n_st_add = 0
    n_ed_add = n_add

    time.sleep(0.11)
    get_trace_of(tracelen_of, output_dir, n_st_of, n_ed_of, input_of, scope, target)

    time.sleep(0.11)
    get_trace_add(tracelen_add, output_dir, n_st_add, n_ed_add, input_add, scope, target)

    scope.dis()


if __name__ == "__main__":
    main()
