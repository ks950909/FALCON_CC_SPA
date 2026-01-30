import argparse
from pathlib import Path

import numpy as np
import matplotlib.pyplot as plt
from matplotlib.colors import ListedColormap


def build_figure(n=1000):
    x = np.linspace(-2.5, 2.5, n)
    y = np.linspace(-2.5, 2.5, n)
    X, Y = np.meshgrid(x, y)
    cs_arr = np.zeros((n, n))
    s_arr = np.zeros((n, n))
    e_arr = np.zeros((n, n))

    for i in range(n):
        for j in range(n):
            xi = X[i, j]
            yi = Y[i, j]
            if abs(xi) > abs(yi):
                cs = 0
            elif abs(xi) < abs(yi):
                cs = 1
            else:
                cs = 0 if xi >= 0 else 1
            cs_arr[i, j] = cs

            if (xi >= 0 and yi >= 0) or (xi < 0 and yi < 0):
                s_arr[i, j] = 0
            else:
                s_arr[i, j] = 1

            maxe = np.floor(np.log2(max(abs(xi), abs(yi))))
            xyi = xi + yi
            if -1e-6 < xyi < 1e-6:
                newe = -64
                e_arr[i, j] = 0
            else:
                newe = np.floor(np.log2(abs(xyi)))
                e_arr[i, j] = newe - maxe + 4

    cmap_b = plt.get_cmap("tab20b")
    cmap_c = plt.get_cmap("tab20c")
    colors_b = [cmap_b(i) for i in range(20)]
    colors_c = [cmap_c(i) for i in range(20)]
    all_colors = colors_b + colors_c
    all_colors.reverse()
    custom_cmap = ListedColormap(all_colors)

    plt.clf()
    plt.figure(figsize=(6.4, 6.4))
    tmparr = cs_arr + s_arr * 2 + e_arr * 4
    plt.xlabel("X")
    plt.ylabel("Y")
    plt.scatter(X, Y, c=tmparr, marker="o", s=5, label=cs_arr, cmap=custom_cmap)


def main():
    parser = argparse.ArgumentParser(description="Generate FPR add region plot")
    parser.add_argument(
        "--out",
        default="outputs/falcon/fpradd.png",
        help="Output image path (default: outputs/falcon/fpradd.png)",
    )
    parser.add_argument(
        "--n",
        type=int,
        default=1000,
        help="Grid resolution (default: 1000)",
    )
    args = parser.parse_args()

    build_figure(n=args.n)

    out_path = Path(args.out)
    out_path.parent.mkdir(parents=True, exist_ok=True)
    plt.savefig(out_path, bbox_inches="tight", format="png", dpi=600)


if __name__ == "__main__":
    main()
