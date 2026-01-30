import argparse
from pathlib import Path

import subprocess
import numpy as np
import matplotlib.pyplot as plt
import yaml


def load_yaml(path: str | Path) -> dict:
    path = Path(path)
    with path.open("r", encoding="utf-8") as f:
        return yaml.safe_load(f) or {}


def u8arr2u64(x, size=8):
    output = np.uint64(0)
    for i in range(size - 1, -1, -1):
        output = np.add(output, np.uint64(x[i]))
        if i != 0:
            output = np.left_shift(output, np.uint64(8))
    return output


def write_pt_ct_add(pt_t, ct_t, out_path: Path):
    n = pt_t.shape[0]
    with out_path.open("w", encoding="utf-8") as f:
        for i in range(n):
            p0 = u8arr2u64(pt_t[i, 0:8], 8)
            p1 = u8arr2u64(pt_t[i, 8:16], 8)
            c0 = u8arr2u64(ct_t[i, 0:8], 8)
            f.write(f"{p0:x} {p1:x} {c0:x}\n")


def write_pt_ct_of(pt_t, ct_t, out_path: Path):
    n = pt_t.shape[0]
    with out_path.open("w", encoding="utf-8") as f:
        for i in range(n):
            p0 = u8arr2u64(pt_t[i, 0:8], 8)
            c0 = u8arr2u64(ct_t[i, 0:8], 8)
            f.write(f"{p0:x} {c0:x}\n")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--config", default="configs/main_falcon.yaml")
    args = ap.parse_args()

    cfg = load_yaml(args.config)
    print(f"[main_falcon] config: {args.config}")

    make_cfg = cfg.get("make_input", {})
    make_enabled = bool(make_cfg.get("enabled", True)) if make_cfg else False
    if make_cfg and make_enabled:
        exe = Path(make_cfg.get("exe", "bin/make_input"))
        test_n = int(make_cfg.get("testN", 100))
        out_of = Path(make_cfg.get("out_of", "outputs/falcon/inputdata_of.txt"))
        out_add = Path(make_cfg.get("out_add", "outputs/falcon/inputdata_add.txt"))

        out_of.parent.mkdir(parents=True, exist_ok=True)
        out_add.parent.mkdir(parents=True, exist_ok=True)

        cmd = [str(exe), str(test_n), str(out_of), str(out_add)]
        print(f"[main_falcon] running: {' '.join(cmd)}")
        subprocess.run(cmd, check=True)

    post_cfg = cfg.get("post_capture", {})
    post_enabled = bool(post_cfg.get("enabled", True)) if post_cfg else False
    if post_cfg and post_enabled:
        input_dir = Path(post_cfg.get("input_dir", "trace/full"))
        print(f"[main_falcon] loading traces from {input_dir}")
        trace_add = np.load(input_dir / post_cfg.get("trace_add", "trace_fpr_add.npy"))
        pt_add_t = np.load(input_dir / post_cfg.get("pt_add", "pt_fpr_add.npy"))
        ct_add_t = np.load(input_dir / post_cfg.get("ct_add", "ct_fpr_add.npy"))
        trace_of = np.load(input_dir / post_cfg.get("trace_of", "trace_fpr_of.npy"))
        pt_of_t = np.load(input_dir / post_cfg.get("pt_of", "pt_fpr_of.npy"))
        ct_of_t = np.load(input_dir / post_cfg.get("ct_of", "ct_fpr_of.npy"))

        out_dir = Path(post_cfg.get("output_dir", "outputs/falcon"))
        out_dir.mkdir(parents=True, exist_ok=True)
        pt_add_txt = out_dir / post_cfg.get("pt_add_txt", "pt_add.txt")
        pt_of_txt = out_dir / post_cfg.get("pt_of_txt", "pt_of.txt")

        print("[main_falcon] writing pt_add/pt_of text files")
        write_pt_ct_add(pt_add_t, ct_add_t, pt_add_txt)
        write_pt_ct_of(pt_of_t, ct_of_t, pt_of_txt)

        group_exe = Path(post_cfg.get("make_group_exe", "bin/make_group"))
        test_n = int(post_cfg.get("testN", 100))
        add_group_txt = out_dir / post_cfg.get("add_group_txt", "add_group.txt")
        of_group_txt = out_dir / post_cfg.get("of_group_txt", "of_group.txt")

        cmd = [
            str(group_exe),
            str(test_n),
            str(pt_add_txt),
            str(pt_of_txt),
            str(add_group_txt),
            str(of_group_txt),
        ]
        print(f"[main_falcon] running: {' '.join(cmd)}")
        subprocess.run(cmd, check=True)

        print("[main_falcon] loading group files")
        addgroupdata = np.loadtxt(add_group_txt, dtype=np.uint64)
        ofgroupdata = np.loadtxt(of_group_txt, dtype=np.uint64)
        print("[main_falcon] group files loaded")

        def calculate_snr_chunked(traces, labels, chunk_size=20000):
            ulabels = np.unique(labels)
            num_labels = len(ulabels)
            num_samples = traces.shape[1]
            label_to_idx = {int(l): i for i, l in enumerate(ulabels)}

            counts = np.zeros(num_labels, dtype=np.int64)
            sum1 = np.zeros((num_labels, num_samples), dtype=np.float64)
            sum2 = np.zeros((num_labels, num_samples), dtype=np.float64)

            n = traces.shape[0]
            for start in range(0, n, chunk_size):
                end = min(start + chunk_size, n)
                t_chunk = traces[start:end]
                l_chunk = labels[start:end]
                for l in ulabels:
                    mask = (l_chunk == l)
                    if not np.any(mask):
                        continue
                    idx = label_to_idx[int(l)]
                    chunk = t_chunk[mask]
                    counts[idx] += chunk.shape[0]
                    sum1[idx] += chunk.sum(axis=0)
                    sum2[idx] += (chunk * chunk).sum(axis=0)

            mean = sum1 / counts[:, None]
            var = (sum2 / counts[:, None]) - (mean * mean)
            signal_power = np.var(mean, axis=0)
            noise_power = np.mean(var, axis=0)
            noise_power[noise_power == 0] = np.finfo(float).eps
            return signal_power / noise_power

        print("[main_falcon] calculating SNR")
        addsnr_trace = np.zeros((8, trace_add.shape[1]))
        for i in range(8):
            print(f"[main_falcon] SNR add group {i+1}/8")
            addsnr_trace[i] = calculate_snr_chunked(trace_add, addgroupdata[:, i])
        ofsnr_trace = np.zeros((8, trace_of.shape[1]))
        for i in range(8):
            print(f"[main_falcon] SNR of group {i+1}/8")
            ofsnr_trace[i] = calculate_snr_chunked(trace_of, ofgroupdata[:, i])

        plt.clf()
        for i in range(8):
            plt.subplot(4, 4, i + 1)
            plt.plot(ofsnr_trace[i])
            plt.xticks([])
            plt.tick_params(axis="y", labelsize=4)
        for i in range(8):
            plt.subplot(4, 4, i + 9)
            plt.plot(addsnr_trace[i])
            plt.xticks([])
            plt.tick_params(axis="y", labelsize=4)
        plt.savefig(out_dir / "fpr_snr.pdf", bbox_inches="tight", format="pdf", dpi=600)
        print(f"[main_falcon] saved {out_dir / 'fpr_snr.pdf'}")

        def see_scatter(trace, idx, group, n, sn, sm, view, f):
            plt.subplot(sn, sm, view)
            group2 = []
            zero = 0.0
            zerocnt = 0
            one = 0.0
            onecnt = 0
            anscnt = 0
            for i in range(n):
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
                for i in range(n):
                    if zero > one:
                        if trace[i, idx] > midval:
                            group2.append("r")
                            if group[i] == 0:
                                anscnt += 1
                        else:
                            group2.append("b")
                            if group[i] == 1:
                                anscnt += 1
                    else:
                        if trace[i, idx] < midval:
                            group2.append("r")
                            if group[i] == 0:
                                anscnt += 1
                        else:
                            group2.append("b")
                            if group[i] == 1:
                                anscnt += 1
                f.write(f"recovery : {anscnt} / {n}\n")
            else:
                for _ in range(n):
                    group2.append("b")
                f.write(f"recovery : {n} / {n}\n")

            plt.xlim([-2, 1])
            group3 = (-group.astype(np.int64)).tolist()
            plt.xticks([-1, 0])
            plt.tick_params(axis="x", labelsize=4)
            plt.tick_params(axis="y", labelsize=4)
            plt.scatter(group3, trace[:, idx], c=group2)
            return group2

        print("[main_falcon] scatter + sidechannel")
        with (out_dir / "recovery_bit_fpr.txt").open("w", encoding="utf-8") as f:
            plt.clf()
            sidechannel_of = np.zeros((8, trace_of.shape[0]), dtype=np.uint8)
            sidechannel_add = np.zeros((8, trace_add.shape[0]), dtype=np.uint8)
            for view in range(8):
                print(f"[main_falcon] scatter of {view+1}/8")
                tmp = see_scatter(
                    trace_of, np.argmax(ofsnr_trace[view]), ofgroupdata[:, view],
                    trace_of.shape[0], 4, 4, view + 1, f
                )
                for i in range(trace_of.shape[0]):
                    sidechannel_of[view, i] = 0 if tmp[i] == "r" else 1
            for view in range(8):
                print(f"[main_falcon] scatter add {view+1}/8")
                tmp = see_scatter(
                    trace_add, np.argmax(addsnr_trace[view]), addgroupdata[:, view],
                    trace_add.shape[0], 4, 4, view + 1 + 8, f
                )
                for i in range(trace_add.shape[0]):
                    sidechannel_add[view, i] = 0 if tmp[i] == "r" else 1
            plt.savefig(out_dir / "fpr_scatter.png", format="png", dpi=600)
        print(f"[main_falcon] saved {out_dir / 'fpr_scatter.png'} and recovery_bit_fpr.txt")

        side_out = Path(post_cfg.get("sidechannel_out", "outputs/falcon/sidechannel.txt"))
        side_out.parent.mkdir(parents=True, exist_ok=True)
        sof = np.transpose(sidechannel_of, (1, 0))
        sadd = np.transpose(sidechannel_add, (1, 0))
        blocks = test_n
        print(f"[main_falcon] writing sidechannel blocks: {blocks}")
        with side_out.open("w", encoding="utf-8") as g:
            for tN in range(blocks):
                if tN % 10 == 0:
                    print(f"[main_falcon] sidechannel block {tN}/{blocks}")
                for i in range(512):
                    for j in range(8):
                        g.write(str(sof[tN * 512 + i, j] * 64) + " ")
                    g.write("\n")
                for i in range(6144):
                    for j in range(8):
                        g.write(str(sadd[tN * 6144 + i, j] * 64) + " ")
                    g.write("\n")
        print(f"[main_falcon] saved {side_out}")

        side_pr_enabled = bool(post_cfg.get("sidechannel_pr_enabled", True))
        side_pr_out = Path(post_cfg.get("sidechannel_pr_out", "outputs/falcon/sidechannel_pr.txt"))
        prior0 = float(post_cfg.get("sidechannel_pr_prior0", 0.5))
        temperature = float(post_cfg.get("sidechannel_pr_temperature", 1.0))

        def post_p0_gauss(x, m0, s0, m1, s1, prior0=0.5, temperature=1.0):
            a = -np.log(s0) - 0.5 * ((x - m0) / s0) ** 2
            b = -np.log(s1) - 0.5 * ((x - m1) / s1) ** 2
            log_prior_ratio = np.log(prior0) - np.log(1 - prior0)
            d = (b - a) - log_prior_ratio
            d = d / max(temperature, 1e-9)
            return 1.0 / (1.0 + np.exp(np.clip(d, -709, 709)))

        if not side_pr_enabled:
            return
        print("[main_falcon] computing sidechannel_pr")
        sidechannel_of_pr = np.zeros((8, trace_of.shape[0]), dtype=np.float64)
        sidechannel_add_pr = np.zeros((8, trace_add.shape[0]), dtype=np.float64)

        for view in range(8):
            snr_idx = np.argmax(ofsnr_trace[view])
            N0 = 0
            N1 = 0
            m0 = 0.0
            m1 = 0.0
            s0 = 0.0
            s1 = 0.0
            for i in range(trace_of.shape[0]):
                if ofgroupdata[i, view] == 0:
                    N0 += 1
                    m0 += trace_of[i, snr_idx]
                    s0 += trace_of[i, snr_idx] * trace_of[i, snr_idx]
                else:
                    N1 += 1
                    m1 += trace_of[i, snr_idx]
                    s1 += trace_of[i, snr_idx] * trace_of[i, snr_idx]
            if N0 == 0 or N1 == 0:
                sidechannel_of_pr[view, :] = 0.0
            else:
                m0 /= N0
                s0 = np.sqrt((s0 / N0) - m0 * m0)
                m1 /= N1
                s1 = np.sqrt((s1 / N1) - m1 * m1)
                for i in range(trace_of.shape[0]):
                    sidechannel_of_pr[view, i] = post_p0_gauss(trace_of[i, snr_idx], m0, s0, m1, s1, prior0, temperature)

        for view in range(8):
            snr_idx = np.argmax(addsnr_trace[view])
            N0 = 0
            N1 = 0
            m0 = 0.0
            m1 = 0.0
            s0 = 0.0
            s1 = 0.0
            for i in range(trace_add.shape[0]):
                if addgroupdata[i, view] == 0:
                    N0 += 1
                    m0 += trace_add[i, snr_idx]
                    s0 += trace_add[i, snr_idx] * trace_add[i, snr_idx]
                else:
                    N1 += 1
                    m1 += trace_add[i, snr_idx]
                    s1 += trace_add[i, snr_idx] * trace_add[i, snr_idx]
            if N0 == 0 or N1 == 0:
                sidechannel_add_pr[view, :] = 0.0
            else:
                m0 /= N0
                s0 = np.sqrt((s0 / N0) - m0 * m0)
                m1 /= N1
                s1 = np.sqrt((s1 / N1) - m1 * m1)
                for i in range(trace_add.shape[0]):
                    sidechannel_add_pr[view, i] = post_p0_gauss(trace_add[i, snr_idx], m0, s0, m1, s1, prior0, temperature)

        side_pr_out.parent.mkdir(parents=True, exist_ok=True)
        sof_pr = np.transpose(sidechannel_of_pr, (1, 0))
        sadd_pr = np.transpose(sidechannel_add_pr, (1, 0))
        print(f"[main_falcon] writing sidechannel_pr blocks: {blocks}")
        with side_pr_out.open("w", encoding="utf-8") as g:
            for tN in range(blocks):
                if tN % 10 == 0:
                    print(f"[main_falcon] sidechannel_pr block {tN}/{blocks}")
                for i in range(512):
                    for j in range(8):
                        g.write(f"{sof_pr[tN * 512 + i, j]:.18f} ")
                    g.write("\n")
                for i in range(6144):
                    for j in range(8):
                        g.write(f"{sadd_pr[tN * 6144 + i, j]:.18f} ")
                    g.write("\n")
        print(f"[main_falcon] saved {side_pr_out}")

    postproc_cfg = cfg.get("postproc_cpp", {})
    postproc_enabled = bool(postproc_cfg.get("enabled", False)) if postproc_cfg else False
    if postproc_cfg and postproc_enabled:
        exe = Path(postproc_cfg.get("exe", "bin/falcon_postproc"))
        out_dir = Path(postproc_cfg.get("output_dir", "outputs/falcon/postproc"))
        out_dir.mkdir(parents=True, exist_ok=True)

        def run_postproc(cmd):
            print(f"[main_falcon] running: {' '.join(cmd)}")
            subprocess.run(cmd, check=True)

        # Announce enabled tests
        enabled_tests = []
        for key in ("rank", "pr_rank", "maxrank", "postproc", "maxrank_pr"):
            cfg_k = postproc_cfg.get(key, {})
            if cfg_k.get("enabled", False):
                enabled_tests.append(key)
        if enabled_tests:
            print(f"[main_falcon] postproc_cpp enabled tests: {', '.join(enabled_tests)}")
        else:
            print("[main_falcon] postproc_cpp enabled but no tests selected")

        # Defaults from existing sections
        sidechannel_default = None
        sidechannel_pr_default = None
        inputdata_default = None
        if post_cfg:
            sidechannel_default = str(Path(post_cfg.get("sidechannel_out", "outputs/falcon/sidechannel.txt")))
            sidechannel_pr_default = str(Path(post_cfg.get("sidechannel_pr_out", "outputs/falcon/sidechannel_pr.txt")))
        if make_cfg:
            inputdata_default = str(Path(make_cfg.get("out_of", "outputs/falcon/inputdata_of.txt")))

        rank_cfg = postproc_cfg.get("rank", {})
        if rank_cfg.get("enabled", False):
            print("[main_falcon] preparing test: rank")
            testmaxnum = int(rank_cfg.get("testmaxnum", 100))
            rank = int(rank_cfg.get("rank", 300))
            sidechannel = rank_cfg.get("sidechannel", sidechannel_default)
            inputdata = rank_cfg.get("inputdata", inputdata_default)
            logname = rank_cfg.get("log", "rank.log")
            log_path = str(out_dir / logname)
            cmd = [str(exe), "rank", str(testmaxnum), str(rank), sidechannel, inputdata, log_path]
            run_postproc(cmd)

        pr_rank_cfg = postproc_cfg.get("pr_rank", {})
        if pr_rank_cfg.get("enabled", False):
            print("[main_falcon] preparing test: pr_rank")
            testmaxnum = int(pr_rank_cfg.get("testmaxnum", 100))
            rank = int(pr_rank_cfg.get("rank", 300))
            sidechannel = pr_rank_cfg.get("sidechannel", sidechannel_pr_default or sidechannel_default)
            inputdata = pr_rank_cfg.get("inputdata", inputdata_default)
            logname = pr_rank_cfg.get("log", "pr_rank.log")
            log_path = str(out_dir / logname)
            cmd = [str(exe), "pr_rank", str(testmaxnum), str(rank), sidechannel, inputdata, log_path]
            run_postproc(cmd)

        maxrank_cfg = postproc_cfg.get("maxrank", {})
        if maxrank_cfg.get("enabled", False):
            print("[main_falcon] preparing test: maxrank")
            testmaxnum = int(maxrank_cfg.get("testmaxnum", 100))
            wrongnum = int(maxrank_cfg.get("wrongnum", 100))
            logname = maxrank_cfg.get("log", "maxrank.log")
            log_path = str(out_dir / logname)
            cmd = [str(exe), "maxrank", str(testmaxnum), str(wrongnum), log_path]
            run_postproc(cmd)

        postproc_mode_cfg = postproc_cfg.get("postproc", {})
        if postproc_mode_cfg.get("enabled", False):
            print("[main_falcon] preparing test: postproc")
            testmaxnum = int(postproc_mode_cfg.get("testmaxnum", 100))
            mode = int(postproc_mode_cfg.get("mode", 1))
            logname = postproc_mode_cfg.get("log", "postproc.log")
            log_path = str(out_dir / logname)
            cmd = [str(exe), "postproc", str(testmaxnum), str(mode), log_path]
            run_postproc(cmd)

        maxrank_pr_cfg = postproc_cfg.get("maxrank_pr", {})
        if maxrank_pr_cfg.get("enabled", False):
            print("[main_falcon] preparing test: maxrank_pr")
            testmaxnum = int(maxrank_pr_cfg.get("testmaxnum", 100))
            sigma = float(maxrank_pr_cfg.get("sigma", 4.0))
            logname = maxrank_pr_cfg.get("log", "maxrank_pr.log")
            log_path = str(out_dir / logname)
            cmd = [str(exe), "maxrank_pr", str(testmaxnum), str(sigma), log_path]
            run_postproc(cmd)

        maxrank_layer_cfg = postproc_cfg.get("maxrank_layer", {})
        if maxrank_layer_cfg.get("enabled", False):
            test_exe = Path(maxrank_layer_cfg.get("exe", "bin/falcon_postproc_test"))
            testmaxnum = int(maxrank_layer_cfg.get("testmaxnum", 20))
            wrongnum = int(maxrank_layer_cfg.get("wrongnum", 505))
            K = int(maxrank_layer_cfg.get("K", 10000))
            max_pairs = int(maxrank_layer_cfg.get("max_pairs", 1000000))
            eq_keep = int(maxrank_layer_cfg.get("eq_keep", 100))
            verbose = int(maxrank_layer_cfg.get("verbose", 1))
            logname = maxrank_layer_cfg.get("log")
            log_path = str(out_dir / logname) if logname else None
            cmd = [
                str(test_exe),
                str(testmaxnum),
                str(wrongnum),
                str(K),
                str(max_pairs),
                str(eq_keep),
                str(verbose),
            ]
            if log_path:
                cmd.append(log_path)
            print(f"[main_falcon] running maxrank_layer: {' '.join(cmd)}")
            subprocess.run(cmd, check=True)


if __name__ == "__main__":
    main()
