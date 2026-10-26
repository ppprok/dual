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
    assert "fk" in ALL_ALGORITHMS


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
    full = list(dual.enumerate_covers(path, algorithm))
    batches = list(dual.iter_batches(path, algorithm, batch_size))
    assert all(1 <= len(b) <= batch_size for b in batches)
    assert [c for b in batches for c in b] == full
    assert as_set(dual.minimal_covers(path, algorithm)) == as_set(full)


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


# --- FK: Khachiyan's incremental dualization and the Fredman-Khachiyan duality test (#46) ---


def check_witness(h, g, witness):
    """Check that ``witness`` proves that the row families ``h`` and ``g`` are not dual."""
    hs, gs = [set(r) for r in h], [set(r) for r in g]
    n = max([j + 1 for r in h + g for j in r], default=0)
    w = set(witness.columns)
    assert witness.columns == sorted(w) and w <= set(range(n))
    if witness.kind == "new_transversal":
        assert all(s & w for s in hs) and not any(t <= w for t in gs)
    else:
        assert witness.kind == "not_transversal"
        complement = set(range(n)) - w
        assert any(t <= w for t in gs) and any(s <= complement for s in hs)


def minimal_sets(rows):
    sets = {frozenset(r) for r in rows}
    return {tuple(sorted(s)) for s in sets if not any(o < s for o in sets)}


@pytest.mark.parametrize("path", HG_FILES, ids=lambda p: p.name)
def test_fk_hg_same_covers_as_runcm(path):
    covers = dual.minimal_covers(path, "fk")
    assert as_set(covers) == as_set(dual.minimal_covers(path, "runcm"))
    assert len(covers) == expected_count(path)
    assert dual.count_covers(path, "fk") == expected_count(path)


@pytest.mark.parametrize("path", HG_FILES, ids=lambda p: p.name)
def test_fk_generator_same_covers_as_runcm(path):
    covers = list(dual.enumerate_covers(path, "fk", batch_size=50))
    assert as_set(covers) == as_set(dual.minimal_covers(path, "runcm"))


def test_fk_random_same_covers_as_runcm():
    rng = random.Random(46)
    for _ in range(100):
        m, n = rng.randint(0, 12), rng.randint(1, 12)
        rows = random_rows(rng, m, n, rng.choice([0.2, 0.4, 0.6]))
        expected = as_set(dual.minimal_covers(rows, "runcm"))
        assert as_set(dual.minimal_covers(rows, "fk")) == expected, rows
        assert as_set(list(dual.enumerate_covers(rows, "fk", batch_size=3))) == expected, rows
        assert dual.count_covers(rows, "fk") == len(expected), rows


def test_fk_dualizer_next_cover_is_resumable():
    path = DATA_DIR / "15_20_275.hg"
    d = dual.Dualizer("fk")
    assert d.algorithm == "fk" and d.resumable
    d.load_hg(str(path))
    first = [d.next_cover() for _ in range(5)]
    assert all(isinstance(c, list) for c in first) and not d.finished
    rest = d.next_batch(100)
    assert len(rest) == 100 and not d.finished
    while True:
        cover = d.next_cover()
        if cover is None:
            break
        rest.append(cover)
    assert d.finished
    assert as_set(first + rest) == as_set(dual.minimal_covers(path))
    assert d.next_cover() is None and d.next_batch(10) == []
    d.reset()
    assert [d.next_cover() for _ in range(5)] == first
    d.set_rows(EXAMPLE)  # a new matrix restarts the enumeration
    assert sorted(d.next_batch(10)) == [[0, 1], [0, 2], [1, 2]]
    assert d.finished


def test_fk_count_and_all_covers_restart_the_enumeration():
    d = dual.make_dualizer(EXAMPLE, "fk")
    first = d.next_cover()
    assert d.count() == 3
    assert d.next_cover() == first
    assert sorted(d.all_covers()) == [[0, 1], [0, 2], [1, 2]]
    assert d.next_cover() == first


def test_next_cover_for_every_algorithm():
    for algorithm in ALL_ALGORITHMS:
        d = dual.make_dualizer(EXAMPLE, algorithm)
        covers = [d.next_cover() for _ in range(4)]
        assert covers[3] is None, algorithm
        assert sorted(covers[:3]) == [[0, 1], [0, 2], [1, 2]], algorithm


def test_fk_corner_cases():
    assert dual.minimal_covers([], "fk") == [[]]
    assert list(dual.enumerate_covers([], "fk")) == [[]]
    assert dual.minimal_covers([[0, 1], []], "fk") == []
    assert list(dual.enumerate_covers([[]], "fk")) == []
    d = dual.Dualizer("fk")
    assert d.next_cover() == [] and d.next_cover() is None and d.finished


def test_is_dual_example():
    assert dual.is_dual(EXAMPLE, [[0, 1], [0, 2], [1, 2]]) is None
    assert dual.is_dual(EXAMPLE, [[1, 2], [0, 2], [0, 1], [0, 1, 2]]) is None  # non-minimal rows are dropped
    witness = dual.is_dual(EXAMPLE, [[0, 1], [0, 2]])
    assert witness == dual.DualityWitness("new_transversal", [1, 2])
    assert witness.kind == "new_transversal" and witness.columns == [1, 2]
    witness = dual.is_dual(EXAMPLE, [[0, 1], [0, 2], [1]])
    assert witness is not None and witness.kind == "not_transversal"
    check_witness(EXAMPLE, [[0, 1], [0, 2], [1]], witness)


@pytest.mark.parametrize("path", HG_FILES[:5], ids=lambda p: p.name)
def test_is_dual_hg(path):
    h = [list(map(int, line.split())) for line in path.read_text().splitlines() if line.strip()]
    covers = dual.minimal_covers(path)
    assert dual.is_dual(path, covers) is None
    assert dual.is_dual(covers, h) is None  # duality is symmetric
    missing = covers[len(covers) // 2]
    g = covers[: len(covers) // 2] + covers[len(covers) // 2 + 1 :]
    witness = dual.is_dual(path, g)
    assert witness is not None and witness.kind == "new_transversal"
    check_witness(h, g, witness)
    assert set(missing) <= set(witness.columns)  # the only minimal cover not in g


def test_is_dual_corner_cases():
    # no rows in h: the only minimal cover is the empty set
    assert dual.is_dual([], [[]]) is None
    witness = dual.is_dual([], [])
    assert witness == dual.DualityWitness("new_transversal", [])
    # a zero row in h: no covers, dual only to an empty g
    assert dual.is_dual([[]], []) is None
    assert dual.is_dual([[0, 1], []], []) is None
    witness = dual.is_dual([[]], [[0]])
    assert witness is not None and witness.kind == "not_transversal"
    # empty g with a coverable h
    witness = dual.is_dual(EXAMPLE, [])
    assert witness is not None and witness.kind == "new_transversal"
    check_witness(EXAMPLE, [], witness)
    # g = {{}} is dual only to an empty h
    witness = dual.is_dual(EXAMPLE, [[]])
    assert witness is not None and witness.kind == "not_transversal"
    check_witness(EXAMPLE, [[]], witness)


def test_is_dual_random_against_brute_force():
    rng = random.Random(4646)
    for _ in range(300):
        m, n = rng.randint(0, 7), rng.randint(1, 7)
        h = random_rows(rng, m, n, rng.choice([0.3, 0.5]))
        covers = sorted(brute_force(h, n))
        if rng.random() < 0.3:
            g = [list(c) for c in covers]
        else:  # perturb the covers: drop some, add random sets
            g = [list(c) for c in covers if rng.random() < 0.7]
            g += random_rows(rng, rng.randint(0, 2), n, 0.4)
        result = dual.is_dual(h, g)
        if minimal_sets(g) == set(covers):
            assert result is None, (h, g)
        else:
            assert result is not None, (h, g)
            check_witness(h, g, result)


def test_is_dual_numpy_input():
    np = pytest.importorskip("numpy")
    h = np.array([[1, 1, 0], [0, 1, 1], [1, 0, 1]], dtype=bool)
    assert dual.is_dual(h, np.array([[1, 1, 0], [1, 0, 1], [0, 1, 1]], dtype=np.uint8)) is None
    assert dual.is_dual(h, [[0, 1], [0, 2]]) == dual.DualityWitness("new_transversal", [1, 2])


def test_is_dual_low_level_same_object():
    d = dual.make_dualizer([[0]], "fk")
    assert _dual.is_dual(d, d) is None  # {{0}} is its own dual
    d.set_rows([[0, 1]])
    kind, columns = _dual.is_dual(d, d)
    check_witness([[0, 1]], [[0, 1]], dual.DualityWitness(kind, columns))
