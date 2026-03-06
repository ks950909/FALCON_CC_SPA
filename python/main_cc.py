import argparse
from pathlib import Path

import numpy as np
import matplotlib.pyplot as plt
import yaml


def load_yaml(path: str | Path) -> dict:
    path = Path(path)
    with path.open("r", encoding="utf-8") as f:
        return yaml.safe_load(f) or {}


def u8arr2u32(x, size=4):
    output = np.uint32(0)
    for i in range(0, size):
        output = np.add(output, np.uint32(x[i]))
        if i != size - 1:
            output = np.left_shift(output, np.uint32(8))
    return output


def load_cc_triplets(cfg: dict):
    data_cfg = cfg.get("data", cfg).get("cc", cfg.get("data", cfg))
    input_dir = Path(data_cfg.get("input_dir", "."))
    expected_names = [
        "trace",
        "pt",
        "ct",
        "trace_m1",
        "pt_m1",
        "ct_m1",
        "trace_m2",
        "pt_m2",
        "ct_m2",
    ]

    def load_arr(key: str):
        name = data_cfg.get(key)
        if not name:
            raise KeyError(f"missing config key: {key}")
        path = input_dir / name
        if not path.exists():
            expected_list = "\n  - ".join(str(data_cfg.get(k)) for k in expected_names if data_cfg.get(k))
            raise FileNotFoundError(
                f"Missing trace file: {path}\n"
                f"Expected CC files under: {input_dir}\n"
                f"  - {expected_list}\n"
                "If this is a fresh Git clone, large trace .npy files may not be included.\n"
                "Place the CC files under trace/full/ (see README Data/Traces; "
                "full dataset DOI: 10.5281/zenodo.18430573)."
            )
        return np.load(path)

    trace = load_arr("trace")
    pt = load_arr("pt")
    ct = load_arr("ct")
    trace_m1 = load_arr("trace_m1")
    pt_m1 = load_arr("pt_m1")
    ct_m1 = load_arr("ct_m1")
    trace_m2 = load_arr("trace_m2")
    pt_m2 = load_arr("pt_m2")
    ct_m2 = load_arr("ct_m2")

    return (trace, pt, ct, trace_m1, pt_m1, ct_m1, trace_m2, pt_m2, ct_m2)


def functional_check(pt, ct, a_one_value: int, label: str):
    n = pt.shape[0]
    cnt = 0
    for i in range(n):
        x = u8arr2u32(pt[i, 0:4])
        y = u8arr2u32(pt[i, 4:8])
        a = u8arr2u32(pt[i, 8:12])
        t = u8arr2u32(ct[i, 0:4])
        if (a == 0 and x == t) or (a == a_one_value and y == t):
            cnt += 1
    print(f"{label}: {cnt}/{n}")


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


def plot_mean_traces(ax, traces, labels, xlim=None):
    zeros = traces[labels == 0]
    ones = traces[labels == 1]
    if zeros.shape[0] == 0 or ones.shape[0] == 0:
        raise ValueError("labels must contain both 0 and 1 for mean-trace plot")
    ax.plot(zeros.mean(axis=0), "r")
    ax.plot(ones.mean(axis=0), "b")
    if xlim is not None:
        ax.set_xlim(xlim)
    ax.set_xlabel("Samples")
    ax.set_ylabel("Power")


def makegroup2(trace, group, n, f):
    group2 = []
    zero = 0.0
    one = 0.0
    zerocnt = 0
    onecnt = 0
    anscnt = 0
    for i in range(n):
        if group[i] == 0:
            zero += trace[i]
            zerocnt += 1
        else:
            one += trace[i]
            onecnt += 1
    if zerocnt != 0 and onecnt != 0:
        zero /= zerocnt
        one /= onecnt
        midval = (zero + one) / 2
        for i in range(n):
            if zero > one:
                if trace[i] > midval:
                    group2.append("r")  # 0
                    if group[i] == 0:
                        anscnt += 1
                else:
                    group2.append("b")  # -1
                    if group[i] == -1:
                        anscnt += 1
            else:
                if trace[i] < midval:
                    group2.append("r")  # 0
                    if group[i] == 0:
                        anscnt += 1
                else:
                    group2.append("b")  # -1
                    if group[i] == -1:
                        anscnt += 1
        f.write(f"recovery : {anscnt} / {n}\n")
    else:
        for _ in range(n):
            group2.append("b")
        f.write(f"recovery : {n} / {n}\n")
    return group2


def auto_xlim_from_snr(snr, radius):
    if radius is None:
        return None
    radius = int(radius)
    if radius <= 0:
        return None
    window = 2 * radius + 1
    kernel = np.ones(window) / window
    mean_snr = np.convolve(snr, kernel, mode="same")
    center = int(np.argmax(mean_snr))
    lo = max(center - radius, 0)
    hi = min(center + radius, snr.shape[0] - 1)
    return (lo, hi)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--config", default="configs/main_cc.yaml")
    args = ap.parse_args()

    cfg = load_yaml(args.config)
    print(f"[main_cc] config: {args.config}")
    out_dir = Path(cfg.get("output_dir", "."))
    out_dir.mkdir(parents=True, exist_ok=True)
    plot_cfg = cfg.get("plot", {})
    xlims = plot_cfg.get("xlims", {})
    auto_cfg = plot_cfg.get("auto_xlim", {})
    auto_enabled = bool(auto_cfg.get("enabled", False))
    auto_radius = auto_cfg.get("radius")

    (
        trace,
        pt,
        ct,
        trace_m1,
        pt_m1,
        ct_m1,
        trace_m2,
        pt_m2,
        ct_m2,
    ) = load_cc_triplets(cfg)
    print("[main_cc] loaded trace/pt/ct sets")

    n = trace.shape[0]
    for name, arr in (
        ("pt", pt),
        ("ct", ct),
        ("trace_m1", trace_m1),
        ("pt_m1", pt_m1),
        ("ct_m1", ct_m1),
        ("trace_m2", trace_m2),
        ("pt_m2", pt_m2),
        ("ct_m2", ct_m2),
    ):
        if arr.shape[0] != n:
            raise ValueError(f"{name} has N={arr.shape[0]} but trace has N={n}")
    print(f"[main_cc] N={n}")

    print("[main_cc] functional check")
    functional_check(pt, ct, 0xFFFFFFFF, "cc")
    functional_check(pt_m1, ct_m1, 0x1, "cc_m1")
    functional_check(pt_m2, ct_m2, 0x1, "cc_m2")

    print("[main_cc] compute zarr (SNR labels)")
    zarr = np.zeros((n,), dtype=np.uint8)
    for i in range(n):
        z = u8arr2u32(pt[i, 8:12])
        if z == 0:
            zarr[i] = 0
        else:
            zarr[i] = 1

    print("[main_cc] calculate SNR")
    snrtrace = calculate_snr(trace, zarr)
    snrtrace_m1 = calculate_snr(trace_m1, zarr)
    snrtrace_m2 = calculate_snr(trace_m2, zarr)
    snr_idx = int(np.argmax(snrtrace))
    snr_idx_m1 = int(np.argmax(snrtrace_m1))
    snr_idx_m2 = int(np.argmax(snrtrace_m2))

    plt.clf()
    plt.subplot(3, 1, 1)
    plt.plot(snrtrace)
    plt.subplot(3, 1, 2)
    plt.plot(snrtrace_m1)
    plt.subplot(3, 1, 3)
    plt.plot(snrtrace_m2)
    plt.savefig(out_dir / "cc_snr.pdf", format="pdf", dpi=600)
    print(f"[main_cc] saved {out_dir / 'cc_snr.pdf'}")

    xlim_cc = xlims.get("cc")
    xlim_m1 = xlims.get("m1")
    xlim_m2 = xlims.get("m2")
    if auto_enabled:
        if isinstance(auto_radius, dict):
            xlim_cc = auto_xlim_from_snr(snrtrace, auto_radius.get("cc"))
            xlim_m1 = auto_xlim_from_snr(snrtrace_m1, auto_radius.get("m1"))
            xlim_m2 = auto_xlim_from_snr(snrtrace_m2, auto_radius.get("m2"))
        else:
            xlim_cc = auto_xlim_from_snr(snrtrace, auto_radius)
            xlim_m1 = auto_xlim_from_snr(snrtrace_m1, auto_radius)
            xlim_m2 = auto_xlim_from_snr(snrtrace_m2, auto_radius)

    fig, axes = plt.subplots(3, 1, sharex=False)
    plot_mean_traces(axes[0], trace, zarr, xlim_cc)
    plot_mean_traces(axes[1], trace_m1, zarr, xlim_m1)
    plot_mean_traces(axes[2], trace_m2, zarr, xlim_m2)
    plt.savefig(out_dir / "cc_trace.pdf", format="pdf", dpi=600)
    print(f"[main_cc] saved {out_dir / 'cc_trace.pdf'}")

    zarr_scatter = np.zeros((n,), dtype=np.int8)
    for i in range(n):
        z = u8arr2u32(pt[i, 8:12])
        if z == 0:
            zarr_scatter[i] = 0
        else:
            zarr_scatter[i] = -1

    zarr_scatter_m1 = zarr_scatter
    zarr_scatter_m2 = zarr_scatter

    with (out_dir / "recovery_bit_CC.txt").open("w", encoding="utf-8") as f:
        plt.clf()
        plt.subplot(3, 1, 1)
        f.write("Conditional Calculator\n")
        plt.xlim([-2, 1])
        plt.xticks([-1, 0])
        plt.ylabel("Power")
        group2 = makegroup2(trace[:, snr_idx], zarr_scatter, n, f)
        plt.scatter(zarr_scatter, trace[:, snr_idx], c=group2)

        plt.subplot(3, 1, 2)
        f.write("Countermeasure of Conditional Calculator 1\n")
        plt.xlim([-2, 1])
        plt.xticks([-1, 0])
        plt.ylabel("Power")
        group2 = makegroup2(trace_m1[:, snr_idx_m1], zarr_scatter_m1, n, f)
        plt.scatter(zarr_scatter_m1, trace_m1[:, snr_idx_m1], c=group2)

        plt.subplot(3, 1, 3)
        f.write("Countermeasure of Conditional Calculator 2\n")
        plt.xlim([-2, 1])
        plt.xticks([-1, 0])
        plt.xlabel("Conditional variable")
        plt.ylabel("Power")
        group2 = makegroup2(trace_m2[:, snr_idx_m2], zarr_scatter_m2, n, f)
        plt.scatter(zarr_scatter_m2, trace_m2[:, snr_idx_m2], c=group2)

        plt.savefig(out_dir / "cc_scatter.png", format="png", dpi=600)
    print(f"[main_cc] saved {out_dir / 'cc_scatter.png'} and recovery_bit_CC.txt")


if __name__ == "__main__":
    main()
