---
name: Bug report
about: Wrong covers, a crash, a build failure or other incorrect behaviour
title: ""
labels: []
assignees: []
---

## Context
What goes wrong and where it shows up: the algorithm (`RUNCM`, `RUNC`, `OPT`, `AOT1`, `PUNC`, `RuncFixed`),
the API (C++, C ABI, Python), the build. What happens if it is not fixed.

**How to reproduce**
1. Input matrix (attach the `.hg` file or paste a small one: one row per line, 0-based column indices).
2. Code or commands you ran.
3. Expected result (e.g. the number of minimal covers) and the actual one, with the error output.

**Environment:** OS, compiler and version, CMake preset or options, commit or version of `dual`.

## DoD
How we will know it is fixed, verifiably: e.g. a regression test that fails on the current code and passes after
the fix, the expected number of covers on a given matrix.
Out of scope: …

## Plan
Suspected cause and the fix, if known. The forks in the road that will have to be decided.

## Constraints and dependencies
Blocking issues (`#N`), external dependencies, API or ABI changes that need the owner's decision.
