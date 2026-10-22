# datasets

Sample input matrices for the dualization algorithms in `.hg` format. Nothing in the build reads
this directory; the tests use their own copies in [`../src/tests/data`](../src/tests/README.md).

## Format

One matrix row (hyperedge) per line, holding the indices of the columns that have a one in that row.
Any non-digit character separates numbers (spaces and tabs both occur here), and each newline ends a
row. Columns are numbered from 0; files with 1-based numbering (e.g. `10_50_7187.hg`) just get an
empty column 0, which does not change the covers. Read with `IntegersFileReader::ReadBitMatrix()` or
`DualizationBacktrackAlgorithmBase::loadMatrix(file)`.

```
0 1
1 2
0 2
```

is the 3x3 matrix with rows `110`, `011`, `101`; its minimal covers are `{0,1}`, `{0,2}`, `{1,2}`.

## Files

| Files | Contents |
|---|---|
| `<rows>_<cols>_<count>.hg` (`4_3_2`, `5_5_2`, `5_5_3`, `5_5_4`, `6_6_4`, `6_6_6`, `10_25_480`, `10_50_7187`, `13_7_10`, `15_10_14`, `15_20_275`, `15_35_12226`) | Matrices named by size and number of minimal covers; RUNCM finds exactly `count` covers on each. |
| `6_6_4_dual.hg`, `6_6_4_ordered.hg`, `6_6_4_size.hg`, `6_6_4_depRows.hg` | Outputs for `6_6_4`: `_dual` lists its 4 minimal covers in search order, `_ordered` the same with sorted columns, `_size` their sizes. `_depRows` is a separate 7-row matrix with dependent rows (rows containing other rows); RUNCM finds 8 covers on it. |
| `10_50_7187_*` | Derived from `10_50_7187.hg`: `_dual` holds its 7187 minimal covers, `_dual_dual` the minimal covers of those (the original 10 rows again); `_min`, `_max`, `_min100`, `_wmin`, `_wmax` and `_dual_max.weights` are outputs of old experiments (subsets of the covers and weights; the exact selection rules are not recorded). |
| `lose100.hg`, `lose800.hg`, `bms2_800.hg`, `SDTH282.hg`, `SDTH402.hg` | Larger benchmark inputs without the count in the name. RUNCM finishes on all of them; `SDTH282` and `SDTH402` are self-dual threshold hypergraphs (as many minimal covers as rows: 19742 and 40202). |
| `extra.hg`, `cmp-submatrix-test.hg`, `write_bm_test.hg`, `integers.txt` | Small inputs used by old tests. |

Try one with the C++ quickstart from the [top-level README](../README.md#use-from-c) via
`runc.loadMatrix("datasets/15_20_275.hg")`, or from Python as in [`../src/pyrunc`](../src/pyrunc/README.md).
