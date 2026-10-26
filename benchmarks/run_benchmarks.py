#!/usr/bin/env python3
"""Benchmark driver for the dualization algorithms (#48).

Runs dual_bench (built with -DDUAL_BUILD_BENCHMARKS=ON, e.g. by the release preset) on random
matrices of three shape regimes, writes the raw results to CSV, computes percentile bootstrap
estimates of the median and mean running time and time per cover, and writes a Markdown report and,
when matplotlib is installed, plots. Only the standard library is required. See benchmarks/README.md.

    python3 benchmarks/run_benchmarks.py                      # default configuration, a few minutes
    python3 benchmarks/run_benchmarks.py --config large       # documented larger configuration
    python3 benchmarks/run_benchmarks.py --algorithms RUNCM,FK --reps 10 --time-limit 5
"""

import argparse
import copy
import csv
import datetime
import json
import math
import os
import platform
import random
import statistics
import subprocess
import sys
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
DEFAULT_BENCH = ROOT / "build" / "release" / "benchmarks" / ("dual_bench.exe" if os.name == "nt" else "dual_bench")

# A regime fixes the shape m : n of the matrix; a size is the value of its free dimension (the
# smaller of m and n), so every regime and density gives a curve of time against size.
#   tall:   m >> n, m = ratio * n, size = n
#   wide:   m << n, n = ratio * m, size = m
#   square: m ~ n,  m = n,         size = n
ALL_ALGORITHMS = ["RUNCM", "RUNC", "OPT", "RuncFixed", "AO1", "AO2", "AO1K", "AO2K", "AO1M", "AO2M", "PUNC", "PUNC-R",
                  "PUNC-RC", "FK"]

# `densities` maps a density p to the sizes run at it: the number of minimal covers grows much faster
# at low density, so each density has its own size range.
CONFIGS = {
    # A few minutes: the largest sizes take about 0.1-1 s for the fastest algorithms.
    "default": {
        "reps": 5,
        "time_limit": 1.0,
        "regimes": {
            "tall": {"ratio": 8, "densities": {0.2: [10, 15, 20, 25, 30], 0.5: [10, 15, 20, 25, 30]}},
            "wide": {"ratio": 8, "densities": {0.2: [4, 5, 6, 7, 8, 9], 0.5: [4, 6, 8, 10, 12]}},
            "square": {"ratio": 1, "densities": {0.2: [10, 15, 20, 25, 30, 35], 0.5: [10, 20, 30, 40, 50]}},
        },
        "algorithms": ALL_ALGORITHMS,
    },
    # Hours: more matrices per point, a longer time limit, larger sizes and more densities.
    "large": {
        "reps": 20,
        "time_limit": 30.0,
        "regimes": {
            "tall": {"ratio": 8, "densities": {0.1: [10, 15, 20, 25, 30], 0.2: [10, 15, 20, 25, 30, 35, 40],
                                               0.3: [10, 15, 20, 25, 30, 35, 40, 45],
                                               0.5: [10, 15, 20, 25, 30, 35, 40, 45, 50]}},
            "wide": {"ratio": 8, "densities": {0.1: [4, 5, 6, 7, 8, 9, 10], 0.2: [4, 5, 6, 7, 8, 9, 10, 11, 12],
                                               0.3: [4, 6, 8, 10, 12, 14], 0.5: [4, 6, 8, 10, 12, 14, 16, 18, 20]}},
            "square": {"ratio": 1, "densities": {0.1: [10, 15, 20, 25, 30, 35], 0.2: [10, 15, 20, 25, 30, 35, 40, 45],
                                                 0.3: [10, 20, 30, 40, 50], 0.5: [10, 20, 30, 40, 50, 60, 70]}},
        },
        "algorithms": ALL_ALGORITHMS,
    },
}

# The plots show one curve per algorithm in a fixed color (and marker), at most eight of them; the
# report tables have all algorithms.
PLOT_STYLE = {"RUNCM": ("#2a78d6", "o"), "RUNC": ("#eb6834", "s"), "OPT": ("#1baf7a", "^"),
              "RuncFixed": ("#eda100", "D"), "AO1M": ("#e87ba4", "v"), "AO2M": ("#008300", "P"),
              "PUNC-R": ("#6250d6", "X"), "FK": ("#e34948", "*")}

RAW_FIELDS = ["regime", "size", "rows", "cols", "density", "rep", "seed", "algorithm", "status", "covers",
              "time_s", "timed_out", "error"]


def shape(regime, size):
    """(rows, cols) of a matrix of the given regime and size."""
    ratio = regime["ratio"]
    return (size * ratio, size) if regime["name"] == "tall" else (size, size * ratio)


def matrix_seed(base_seed, regime_name, size, density, rep):
    """Seed of one random matrix: the same for every algorithm, so they all run on the same matrices."""
    key = f"{base_seed}:{regime_name}:{size}:{density}:{rep}"
    h = 1469598103934665603  # FNV-1a, stable across Python runs (hash() is salted)
    for b in key.encode():
        h = ((h ^ b) * 1099511628211) % (1 << 64)
    return h % (1 << 63)


def run_one(bench, algorithm, rows, cols, density, seed, time_limit):
    """One dual_bench run; a hard kill after a grace period counts as a timeout."""
    cmd = [str(bench), "--algorithm", algorithm, "--random", str(rows), str(cols), str(density), str(seed),
           "--time-limit", str(time_limit)]
    # The algorithms check the time limit only between search nodes or covers; FK can spend long
    # inside one duality check, so the driver kills a run that overshoots by much.
    hard_limit = 2 * time_limit + 5
    start = time.perf_counter()
    try:
        proc = subprocess.run(cmd, capture_output=True, text=True, timeout=hard_limit)
    except subprocess.TimeoutExpired:
        return {"status": "timeout", "covers": 0, "time_s": time.perf_counter() - start, "timed_out": True,
                "error": "killed by the driver"}
    try:
        result = json.loads(proc.stdout.strip().splitlines()[-1])
    except (ValueError, IndexError):
        return {"status": "error", "covers": 0, "time_s": 0.0, "timed_out": False,
                "error": f"exit code {proc.returncode}: {proc.stderr.strip()[:200]}"}
    return result


def bootstrap(values, statistic, resamples, rng):
    """Point estimate and 95% percentile bootstrap CI of statistic(values)."""
    point = statistic(values)
    if len(values) < 2:
        return point, point, point
    n = len(values)
    estimates = sorted(statistic([values[rng.randrange(n)] for _ in range(n)]) for _ in range(resamples))
    lo = estimates[int(0.025 * (resamples - 1))]
    hi = estimates[int(math.ceil(0.975 * (resamples - 1)))]
    return point, lo, hi


def summarize(raw, resamples, seed):
    """Bootstrap estimates per (regime, size, density, algorithm) cell.

    A timed-out run counts with the time at which it stopped (at least the limit), so a cell with
    timeouts gives a lower bound of the time (marked with >= in the report). Time per cover uses only completed runs with covers > 0.
    """
    cells = {}
    for r in raw:
        if r["status"] == "skipped":
            continue
        key = (r["regime"], int(r["size"]), float(r["density"]), r["algorithm"])
        cells.setdefault(key, []).append(r)
    summary = []
    for key in sorted(cells):
        runs = cells[key]
        rng = random.Random(f"{seed}:{key}")
        ok = [r for r in runs if r["status"] == "ok"]
        errors = sum(r["status"] == "error" for r in runs)
        timeouts = sum(r["status"] == "timeout" for r in runs)
        times = [float(r["time_s"]) for r in runs if r["status"] != "error"]
        per_cover = [float(r["time_s"]) / int(r["covers"]) for r in ok if int(r["covers"]) > 0]
        row = {"regime": key[0], "size": key[1], "rows": runs[0]["rows"], "cols": runs[0]["cols"],
               "density": key[2], "algorithm": key[3], "runs": len(runs), "timeouts": timeouts, "errors": errors,
               "covers_median": statistics.median([int(r["covers"]) for r in ok]) if ok else None}
        for name, values in (("time", times), ("per_cover", per_cover)):
            if values:
                for stat_name, stat in (("median", statistics.median), ("mean", statistics.fmean)):
                    point, lo, hi = bootstrap(values, stat, resamples, rng)
                    row[f"{name}_{stat_name}"], row[f"{name}_{stat_name}_lo"], row[f"{name}_{stat_name}_hi"] = (
                        point, lo, hi)
        summary.append(row)
    return summary


def check_agreement(raw):
    """Matrices on which completed runs of different algorithms report different cover counts."""
    counts = {}
    for r in raw:
        if r["status"] == "ok":
            counts.setdefault((r["regime"], r["size"], r["density"], r["rep"]), {})[r["algorithm"]] = int(r["covers"])
    return {k: v for k, v in counts.items() if len(set(v.values())) > 1}


def fmt_time(t):
    if t is None:
        return "-"
    if t >= 1:
        return f"{t:.2f} s"
    if t >= 1e-3:
        return f"{t * 1e3:.1f} ms"
    if t >= 1e-6:
        return f"{t * 1e6:.1f} us"
    return f"{t * 1e9:.0f} ns"


def fmt_est(row, name, censored=False):
    if f"{name}" not in row:
        return "-"
    prefix = ">= " if censored else ""
    return f"{prefix}{fmt_time(row[name])} [{fmt_time(row[name + '_lo'])}, {fmt_time(row[name + '_hi'])}]"


def machine_info(bench):
    info = {"date": datetime.datetime.now().isoformat(timespec="seconds"), "platform": platform.platform(),
            "python": platform.python_version(), "cpu": platform.processor() or platform.machine(),
            "logical_cpus": os.cpu_count()}
    try:
        with open("/proc/cpuinfo") as f:
            for line in f:
                if line.startswith("model name"):
                    info["cpu"] = line.split(":", 1)[1].strip()
                    break
    except OSError:
        pass
    try:
        with open("/proc/meminfo") as f:
            info["memory"] = f"{int(f.readline().split()[1]) // 1024} MiB"
    except (OSError, ValueError, IndexError):
        pass
    if hasattr(os, "getloadavg"):
        info["load_before"] = round(os.getloadavg()[0], 1)
    try:
        info["build"] = json.loads(subprocess.run([str(bench), "--info"], capture_output=True, text=True).stdout)
    except (OSError, ValueError):
        pass
    try:
        info["git_commit"] = subprocess.run(["git", "-C", str(ROOT), "rev-parse", "--short", "HEAD"],
                                            capture_output=True, text=True).stdout.strip()
    except OSError:
        pass
    return info


def write_report(path, args, config, info, summary, raw, mismatches, elapsed, plots):
    lines = [f"# Benchmark report {info['date'][:10]}", ""]
    lines += ["Generated by `benchmarks/run_benchmarks.py` (see `benchmarks/README.md` for how to read it).", ""]
    lines += ["## Setup", ""]
    lines += [f"- Machine: {info.get('cpu')}, {info.get('logical_cpus')} logical CPUs, {info.get('memory', '?')} RAM; "
              f"{info.get('platform')}",
              f"- Load average (1 min) before / after the run: {info.get('load_before', '?')} / "
              f"{info.get('load_after', '?')} (other processes were running if it is well above 0)",
              f"- Build: {json.dumps(info.get('build', {}))}; commit `{info.get('git_commit', '?')}`",
              f"- Configuration `{args.config}`: {config['reps']} random matrices per point, time limit "
              f"{config['time_limit']} s per run, {args.bootstrap} bootstrap resamples, seed {args.seed}; "
              f"runs one at a time; total wall time {elapsed / 60:.1f} min",
              "- Matrices: every cell is 1 with probability p (a zero row gets one random 1); regimes "
              + "; ".join(f"`{name}`: {size_label(name, r['ratio'])}" for name, r in config["regimes"].items()),
              "- Time: median and mean over the matrices of a point with 95% percentile bootstrap CIs; "
              "a timed-out run counts with the time at which it stopped (at least the limit), so cells with "
              "timeouts (marked `>=`) are lower bounds. "
              "Once more than half of an algorithm's runs at a point time out or fail (RuncFixed takes at most "
              "64 columns), its remaining runs there and all larger sizes of that regime and density are skipped.",
              ""]
    if mismatches:
        lines += ["**Cover counts disagree** on these matrices (regime, size, density, rep):", ""]
        lines += [f"- {k}: {v}" for k, v in sorted(mismatches.items())] + [""]
    else:
        lines += ["All completed runs on the same matrix reported the same number of covers.", ""]

    # Fastest algorithm per point
    lines += ["## Fastest algorithm per point", "",
              "By median time over completed runs only; ties within 5% are listed together.", "",
              "| regime | m | n | p | covers (median) | fastest | median time | runner-up | median time |",
              "|---|---:|---:|---:|---:|---|---:|---|---:|"]
    points = {}
    for row in summary:
        points.setdefault((row["regime"], row["size"], row["density"]), []).append(row)
    order = list(config["regimes"])
    for key in sorted(points, key=lambda k: (order.index(k[0]), k[2], k[1])):
        done = sorted((r for r in points[key] if r["timeouts"] == 0 and r["errors"] == 0 and "time_median" in r),
                      key=lambda r: r["time_median"])
        if not done:
            continue
        best = done[0]
        tied = [r["algorithm"] for r in done if r["time_median"] <= best["time_median"] * 1.05]
        second = next((r for r in done if r["algorithm"] not in tied), None)
        lines.append(f"| {key[0]} | {best['rows']} | {best['cols']} | {key[2]} | {best['covers_median']:g} | "
                     f"{', '.join(tied)} | {fmt_time(best['time_median'])} | "
                     f"{second['algorithm'] if second else '-'} | {fmt_time(second['time_median']) if second else '-'} |")
    lines.append("")

    for regime in order:
        lines += [f"## Regime `{regime}`", ""]
        for density in config["regimes"][regime]["densities"]:
            density = float(density)
            lines += [f"### p = {density}", "",
                      "| m | n | algorithm | runs | timeouts | errors | covers (median) | median time [95% CI] | "
                      "mean time [95% CI] | median time per cover [95% CI] |",
                      "|---:|---:|---|---:|---:|---:|---:|---:|---:|---:|"]
            for row in summary:
                if row["regime"] != regime or row["density"] != density:
                    continue
                cens = row["timeouts"] > 0
                covers = "-" if row["covers_median"] is None else f"{row['covers_median']:g}"
                lines.append(f"| {row['rows']} | {row['cols']} | {row['algorithm']} | {row['runs']} | "
                             f"{row['timeouts']} | {row['errors']} | {covers} | {fmt_est(row, 'time_median', cens)} | "
                             f"{fmt_est(row, 'time_mean', cens)} | {fmt_est(row, 'per_cover_median')} |")
            skipped = sorted({(int(r["size"]), r["algorithm"]) for r in raw
                              if r["regime"] == regime and float(r["density"]) == density and r["status"] == "skipped"})
            if skipped:
                by_alg = {}
                for size, alg in skipped:
                    by_alg.setdefault(alg, []).append(size)
                lines += ["", "Skipped: " + "; ".join(
                    f"{alg} (size {', '.join(map(str, sizes))})" for alg, sizes in sorted(by_alg.items()))]
            lines.append("")
        for plot in plots:
            if plot.stem.endswith(regime):
                lines += [f"![{regime}]({plot.name})", ""]
    path.write_text("\n".join(lines))


def size_label(regime, ratio):
    if regime == "tall":
        return f"size = n (m = {ratio} n)"
    if regime == "wide":
        return f"size = m (n = {ratio} m)"
    return "size = m = n"


def make_plots(summary, config, out_dir, stem):
    try:
        import matplotlib
        matplotlib.use("Agg")
        import matplotlib.pyplot as plt
    except ImportError:
        print("matplotlib is not installed, no plots", file=sys.stderr)
        return []
    paths = []
    for regime, spec in config["regimes"].items():
        densities = list(spec["densities"])
        fig, axes = plt.subplots(1, len(densities), figsize=(6 * len(densities), 4.5), squeeze=False, sharey=True)
        for ax, density in zip(axes[0], densities):
            for alg in config["algorithms"]:
                if alg not in PLOT_STYLE:
                    continue
                color, marker = PLOT_STYLE[alg]
                rows = [r for r in summary if r["regime"] == regime and r["density"] == density
                        and r["algorithm"] == alg and "time_median" in r and r["timeouts"] == 0 and r["errors"] == 0]
                if not rows:
                    continue
                xs = [r["size"] for r in rows]
                ys = [r["time_median"] for r in rows]
                err = [[r["time_median"] - r["time_median_lo"] for r in rows],
                       [r["time_median_hi"] - r["time_median"] for r in rows]]
                ax.errorbar(xs, ys, yerr=err, color=color, marker=marker, markersize=5, capsize=2, linewidth=1.5,
                            label=alg)
            ax.set_xlabel(size_label(regime, spec["ratio"]))
            ax.set_yscale("log")
            ax.set_title(f"{regime}, p = {density}")
            ax.grid(True, which="both", alpha=0.3)
        axes[0][0].set_ylabel("median time, s (95% bootstrap CI)")
        fig.suptitle("Algorithms without timeouts or errors at a point; all algorithms are in the tables",
                     fontsize=8, color="#555555")
        if axes[0][-1].get_legend_handles_labels()[0]:
            axes[0][-1].legend(fontsize=7, ncol=2)
        fig.tight_layout()
        path = out_dir / f"{stem}-{regime}.png"
        fig.savefig(path, dpi=90)
        plt.close(fig)
        paths.append(path)
    return paths


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--config", choices=sorted(CONFIGS), default="default")
    parser.add_argument("--bench", type=Path, default=DEFAULT_BENCH, help=f"dual_bench executable ({DEFAULT_BENCH})")
    parser.add_argument("--algorithms", help="comma-separated algorithms (default: those of the configuration)")
    parser.add_argument("--regimes", help="comma-separated regimes (default: all of the configuration)")
    parser.add_argument("--reps", type=int, help="random matrices per grid point")
    parser.add_argument("--time-limit", type=float, help="seconds per run")
    parser.add_argument("--bootstrap", type=int, default=2000, help="bootstrap resamples (2000)")
    parser.add_argument("--seed", type=int, default=48, help="seed of the matrices and of the bootstrap (48)")
    parser.add_argument("--out", type=Path, default=Path(__file__).resolve().parent / "results",
                        help="output directory (benchmarks/results)")
    parser.add_argument("--name", help="file name stem of the outputs (default: today's date)")
    args = parser.parse_args()

    config = copy.deepcopy(CONFIGS[args.config])
    if args.algorithms:
        config["algorithms"] = args.algorithms.split(",")
    if args.regimes:
        config["regimes"] = {k: v for k, v in config["regimes"].items() if k in args.regimes.split(",")}
    if args.reps:
        config["reps"] = args.reps
    if args.time_limit:
        config["time_limit"] = args.time_limit
    if not args.bench.exists():
        sys.exit(f"{args.bench} not found; build it with: cmake --preset release && cmake --build --preset release")
    known = subprocess.run([str(args.bench), "--list"], capture_output=True, text=True).stdout.split()
    unknown = [a for a in config["algorithms"] if a not in known]
    if unknown:
        sys.exit(f"unknown algorithms {unknown}; see {args.bench} --list")

    args.out.mkdir(parents=True, exist_ok=True)
    stem = args.name or datetime.date.today().isoformat()
    info = machine_info(args.bench)
    raw = []
    started = time.perf_counter()
    for regime_name, regime in config["regimes"].items():
        regime = dict(regime, name=regime_name)
        for density, sizes in regime["densities"].items():
            given_up = set()  # algorithms that timed out (or failed) at a smaller size
            for size in sorted(sizes):
                rows, cols = shape(regime, size)
                for alg in config["algorithms"]:
                    failed = 0
                    for rep in range(config["reps"]):
                        seed = matrix_seed(args.seed, regime_name, size, density, rep)
                        base = {"regime": regime_name, "size": size, "rows": rows, "cols": cols, "density": density,
                                "rep": rep, "seed": seed, "algorithm": alg}
                        if alg in given_up:
                            raw.append(dict(base, status="skipped", covers=0, time_s=0.0, timed_out=False, error=""))
                            continue
                        r = run_one(args.bench, alg, rows, cols, density, seed, config["time_limit"])
                        failed += r["status"] != "ok"
                        raw.append(dict(base, status=r["status"], covers=r["covers"], time_s=r["time_s"],
                                        timed_out=r["timed_out"], error=r.get("error", "")))
                        # Once most runs of the point timed out or failed, the rest of it and the
                        # larger sizes are skipped.
                        if failed * 2 > config["reps"]:
                            given_up.add(alg)
                print(f"{regime_name:6} p={density:<4} {rows}x{cols}: done, {time.perf_counter() - started:.0f} s",
                      file=sys.stderr, flush=True)
    elapsed = time.perf_counter() - started
    if hasattr(os, "getloadavg"):
        info["load_after"] = round(os.getloadavg()[0], 1)

    csv_path = args.out / f"{stem}.csv"
    with open(csv_path, "w", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=RAW_FIELDS)
        writer.writeheader()
        for r in raw:
            writer.writerow({k: r[k] for k in RAW_FIELDS})
    summary = summarize(raw, args.bootstrap, args.seed)
    plots = make_plots(summary, config, args.out, stem)
    report = args.out / f"{stem}.md"
    write_report(report, args, config, info, summary, raw, check_agreement(raw), elapsed, plots)
    print(f"wrote {csv_path}, {report}" + "".join(f", {p}" for p in plots), file=sys.stderr)


if __name__ == "__main__":
    main()
