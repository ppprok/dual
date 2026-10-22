"""Tests of the Python package ``dual`` (run with ``pytest`` from the repository root)."""

import gc
import itertools
import random
import re
import threading
from pathlib import Path

import pytest

import dual
from dual import _dual

DATA_DIR = Path(__file__).resolve().parents[2] / "src" / "tests" / "data"
HG_FILES = sorted(p for p in DATA_DIR.glob("*.hg") if re.fullmatch(r"\d+_\d+_\d+", p.stem))
RESUMABLE = ("runcm", "runc", "opt")
ALL_ALGORITHMS = dual.ALGORITHMS

EXAMPLE = [[0, 1], [1, 2], [0, 2]]


def expected_count(path):
    return int(path.stem.split("_")[2])


def as_set(covers):
    result = {tuple(c) for c in covers}
    assert len(result) == len(covers), "a cover is reported twice"
    return result


def brute_force(rows, n):
    """All minimal covers of a matrix with n columns, by checking every column subset."""
    sets = [set(r) for r in rows]
    covers = [
        set(c)
        for k in range(n + 1)
        for c in itertools.combinations(range(n), k)
        if all(s & set(c) for s in sets)
    ]
    return {tuple(sorted(c)) for c in covers if not any(o < c for o in covers)}


def random_rows(rng, m, n, density):
    return [[j for j in range(n) if rng.random() < density] for _ in range(m)]


def test_algorithms_list():
    assert set(RESUMABLE) <= set(ALL_ALGORITHMS)
    assert "punc" in ALL_ALGORITHMS


def test_example():
    assert sorted(dual.enumerate_covers(EXAMPLE)) == [[0, 1], [0, 2], [1, 2]]
    assert dual.count_covers(EXAMPLE) == 3


def test_covers_are_sorted_lists():
    for cover in dual.enumerate_covers(DATA_DIR / "15_20_275.hg"):
        assert isinstance(cover, list)
        assert cover == sorted(cover)


@pytest.mark.parametrize("algorithm", RESUMABLE)
@pytest.mark.parametrize("path", HG_FILES, ids=lambda p: p.name)
def test_hg_count(path, algorithm):
    covers = list(dual.enumerate_covers(path, algorithm))
    assert len(as_set(covers)) == expected_count(path)
    assert dual.count_covers(str(path), algorithm) == expected_count(path)


@pytest.mark.parametrize("path", HG_FILES[:4], ids=lambda p: p.name)
def test_hg_same_set_for_all_algorithms(path):
    reference = as_set(dual.minimal_covers(path))
    assert len(reference) == expected_count(path)
    for algorithm in ALL_ALGORITHMS:
        assert as_set(dual.minimal_covers(path, algorithm)) == reference, algorithm


@pytest.mark.parametrize("algorithm", ALL_ALGORITHMS)
def test_brute_force_random(algorithm):
    rng = random.Random(13)
    for _ in range(60):
        m, n = rng.randint(1, 8), rng.randint(1, 8)
        rows = random_rows(rng, m, n, rng.choice([0.3, 0.5, 0.7]))
        expected = brute_force(rows, n)
        assert as_set(dual.minimal_covers(rows, algorithm)) == expected, rows


@pytest.mark.parametrize("algorithm", ALL_ALGORITHMS)
def test_empty_matrix_has_one_empty_cover(algorithm):
    assert dual.minimal_covers([], algorithm) == [[]]
    assert dual.count_covers([], algorithm) == 1


@pytest.mark.parametrize("algorithm", ALL_ALGORITHMS)
def test_zero_row_has_no_covers(algorithm):
    assert dual.minimal_covers([[0, 1], [], [1, 2]], algorithm) == []
    assert dual.minimal_covers([[]], algorithm) == []
    assert dual.count_covers([[0], []], algorithm) == 0


@pytest.mark.parametrize("algorithm", ALL_ALGORITHMS)
@pytest.mark.parametrize("batch_size", [1, 2, 7, 1000])
def test_batches_equal_full_enumeration(algorithm, batch_size):
    path = DATA_DIR / "15_20_275.hg"
    full = dual.minimal_covers(path, algorithm)
    batches = list(dual.iter_batches(path, algorithm, batch_size))
    assert all(1 <= len(b) <= batch_size for b in batches)
    assert [c for b in batches for c in b] == full


def test_dualizer_portions_and_restart():
    d = dual.Dualizer("runcm")
    d.load_hg(str(DATA_DIR / "15_20_275.hg"))
    assert (d.n_rows, d.n_cols) == (15, 20)
    assert d.resumable
    first = d.next_batch(100)
    assert len(first) == 100 and not d.finished
    rest = []
    while True:
        batch = d.next_batch(64)
        if not batch:
            break
        rest.extend(batch)
    assert d.finished
    assert len(first) + len(rest) == 275
    assert d.next_batch(10) == []
    d.reset()
    assert d.next_batch(100) == first
    d.set_rows(EXAMPLE)  # a new matrix restarts the enumeration
    assert sorted(d.next_batch(10)) == [[0, 1], [0, 2], [1, 2]]


def test_non_resumable_algorithm_portions():
    d = dual.Dualizer("ao1m")
    assert not d.resumable
    d.set_rows(EXAMPLE)
    covers = d.next_batch(2) + d.next_batch(2)
    assert d.finished and d.next_batch(2) == []
    assert sorted(covers) == [[0, 1], [0, 2], [1, 2]]


def test_numpy_input():
    np = pytest.importorskip("numpy")
    dense = np.array([[1, 1, 0], [0, 1, 1], [1, 0, 1]], dtype=bool)
    expected = [[0, 1], [0, 2], [1, 2]]
    assert sorted(dual.enumerate_covers(dense)) == expected
    assert sorted(dual.enumerate_covers(dense.astype(np.int64))) == expected
    assert sorted(dual.enumerate_covers(np.asfortranarray(dense))) == expected
    assert sorted(dual.enumerate_covers(dense[:, ::-1])) == [[0, 1], [0, 2], [1, 2]]
    assert dual.minimal_covers(np.zeros((0, 4), dtype=bool)) == [[]]
    assert dual.minimal_covers(np.zeros((2, 0), dtype=bool)) == []
    with pytest.raises(ValueError):
        dual.minimal_covers(np.zeros(3, dtype=bool))


def test_numpy_matches_rows():
    np = pytest.importorskip("numpy")
    rng = np.random.default_rng(5)
    for _ in range(20):
        dense = rng.random((10, 12)) < 0.4
        rows = [list(np.flatnonzero(r)) for r in dense]
        assert as_set(dual.minimal_covers(dense)) == as_set(dual.minimal_covers(rows))


def test_errors():
    with pytest.raises(ValueError, match="unknown algorithm"):
        dual.Dualizer("nope")
    with pytest.raises(ValueError, match="out of range"):
        dual.minimal_covers([[0, -1]])
    with pytest.raises(FileNotFoundError):
        dual.minimal_covers(DATA_DIR / "no_such_file.hg")
    with pytest.raises(ValueError):
        list(dual.iter_batches(EXAMPLE, batch_size=0))


def test_interrupted_generator_does_not_leak():
    gc.collect()
    before = _dual._live_dualizers()
    gen = dual.enumerate_covers(DATA_DIR / "15_35_12226.hg", batch_size=10)
    assert len(list(itertools.islice(gen, 25))) == 25
    assert _dual._live_dualizers() == before + 1
    gen.close()
    assert _dual._live_dualizers() == before
    for _ in dual.enumerate_covers(DATA_DIR / "15_35_12226.hg"):
        break
    gc.collect()
    assert _dual._live_dualizers() == before


def test_threads_run_in_parallel_safely():
    path = DATA_DIR / "15_35_12226.hg"
    results = [None] * 4

    def work(k):
        results[k] = dual.count_covers(path, RESUMABLE[k % 3])

    threads = [threading.Thread(target=work, args=(k,)) for k in range(4)]
    for t in threads:
        t.start()
    for t in threads:
        t.join()
    assert results == [12226] * 4


def test_version():
    assert isinstance(dual.__version__, str)
