# Benchmarks

Running time of the dualization algorithms as a function of the input size, for three shapes of
the matrix (#48). Two pieces:

- `dual_bench` (`dual_bench.cpp`): runs **one** algorithm on **one** matrix with a time limit and
  prints one JSON line. Built when `DUAL_BUILD_BENCHMARKS=ON` (OFF by default; the CMake presets
  turn it on, so CI builds it on every platform, but does not run it).
- `run_benchmarks.py`: the driver. Generates the grid of random matrices, runs `dual_bench` on each,
  writes raw results to CSV, computes bootstrap estimates and writes a Markdown report and, when
  matplotlib is installed, plots. Python 3.8+, standard library only.

Committed reports are in [`results/`](results/).

## How to run

```bash
cmake --preset release && cmake --build --preset release     # builds build/release/benchmarks/dual_bench
python3 benchmarks/run_benchmarks.py                         # default configuration, a few minutes
```

The outputs go to `benchmarks/results/<date>.{csv,md}` and `<date>-<regime>.png`
(`--out DIR` and `--name STEM` change that). Useful options:

| Option | Meaning |
|---|---|
| `--config default\|large` | the configuration (below) |
| `--algorithms RUNCM,FK` | a subset of the algorithms (`dual_bench --list` lists all) |
| `--regimes tall,square` | a subset of the regimes |
| `--reps R` | random matrices per grid point (default 5, `large`: 20) |
| `--time-limit S` | seconds per run (default 1, `large`: 30) |
| `--bootstrap B` | bootstrap resamples (2000) |
| `--seed S` | seed of the matrices and of the bootstrap (48) |
| `--bench PATH` | another `dual_bench` (default `build/release/benchmarks/dual_bench`) |

Runs are sequential, so the numbers are only as quiet as the machine: do not build in parallel
with a benchmark run. Plots need matplotlib (`uv venv --seed && .venv/bin/pip install matplotlib`,
then run the script with `.venv/bin/python`); without it the report has only tables.

`dual_bench` on its own:

```bash
build/release/benchmarks/dual_bench --list
build/release/benchmarks/dual_bench --algorithm RUNCM --random 30 30 0.2 1 --time-limit 5
build/release/benchmarks/dual_bench --algorithm FK --file src/tests/data/10_50_7187.hg
build/release/benchmarks/dual_bench --info    # compiler and whether assertions are on
```

Output: `{"algorithm": "RUNCM", "rows": 30, "cols": 30, "density": 0.2, "seed": 1, "file": "",
"covers": 12629, "time_s": 0.0081, "timed_out": false, "status": "ok", "error": ""}`. `status` is
`ok`, `timeout` (the enumeration was stopped at the limit; `covers` is the number found so far) or
`error` (e.g. RuncFixed on more than 64 columns; exit code 1). `time_s` is `steady_clock` time
around `dualize()` only: generating the matrix and building the algorithm's input are not counted.
Covers are only counted, not stored (except by RuncFixed, which always keeps them).

Algorithms: `RUNCM`, `RUNC`, `OPT` (`Runc::RuncGlobalState`), `RuncFixed` (`RuncFixed<bits32>`
for n <= 32, `bits64` for n <= 64), `AO1`, `AO2`, `AO1K`, `AO2K`, `AO1M`, `AO2M`
(`AOT1::GlobalState`), `PUNC` (input row order), `PUNC-R` (rows by increasing number of ones),
`PUNC-RC` (and columns by decreasing number of ones), `FK` (`FK::GlobalState`).

The time limit is checked by the callbacks every 1024 search nodes or covers (RuncFixed: every
1024 covers), so a run can overshoot it a little; FK can spend long inside one duality check, so
the driver also kills a run after `2 * limit + 5` seconds and records it as a timeout.

## How the matrices are generated

`--random M N P SEED` makes an M x N matrix whose cells are 1 independently with probability P;
a row that came out empty gets a single 1 in a uniformly chosen column (so every matrix has
covers). Only the raw output of `std::mt19937_64` is used, so a seed gives the same matrix on every
compiler and platform. No other preprocessing: rows that are supersets of other rows stay in.

The driver's grid has three **regimes** (m rows, n columns), each a curve of time against a
**size**, the smaller dimension:

| Regime | Shape | Size |
|---|---|---|
| `tall` | m >> n: m = 8 n | n |
| `wide` | m << n: n = 8 m | m |
| `square` | m ~ n: m = n | n |

Each regime is run at several densities p, each with its own size range (the number of minimal
covers, and with it the time, grows much faster at low density). For every (regime, p, size) point
the driver draws R matrices; their seeds depend only on `--seed` and the point, so all algorithms
run on the same matrices and a rerun reproduces them.

| Configuration | R | Limit | Densities | Sizes | Takes |
|---|---:|---:|---|---|---|
| `default` | 5 | 1 s | 0.2, 0.5 | up to 30 (`tall`), 12 (`wide`), 50 (`square`) | a few minutes |
| `large` | 20 | 30 s | 0.1, 0.2, 0.3, 0.5 | up to 50 (`tall`), 20 (`wide`), 70 (`square`) | hours |

The exact grids are `CONFIGS` at the top of `run_benchmarks.py`. Once more than half of an
algorithm's runs at a point time out or fail, its remaining runs there and all larger sizes of
that regime and density are skipped (recorded as `skipped` in the CSV).

## How to read the output

- `<stem>.csv`: one row per run: `regime, size, rows, cols, density, rep, seed, algorithm, status,
  covers, time_s, timed_out, error`.
- `<stem>.md`: the report.
  - **Setup**: machine, compiler, commit, configuration, wall time.
  - A cross-check: all algorithms that finished on the same matrix must report the same number of
    covers; disagreements are listed.
  - **Fastest algorithm per point**: the algorithm with the smallest median time among those with
    no timeouts and errors at that point, ties within 5%, and the runner-up.
  - Per regime and density, a table of every algorithm at every size: runs, timeouts, errors,
    median number of covers, and **median** and **mean** time with **95% percentile bootstrap
    confidence intervals** (B resamples of the R runs, fixed seed), and the median **time per
    cover** with its CI (completed runs only).
  - A timed-out run enters the time statistics with the time at which it stopped, at least the
    limit, so such cells are lower bounds and marked `>=`.
- `<stem>-<regime>.png`: median time (log scale) against size, one panel per density, with the
  bootstrap CI as error bars; only points without timeouts or errors are drawn.

With R = 5 the intervals are wide and coarse (a bootstrap of 5 values has few distinct resamples);
use `--reps` or the `large` configuration for tighter ones. Time per cover is the more stable
number to compare algorithms at a point, since the number of covers varies a lot between random
matrices of the same size.
