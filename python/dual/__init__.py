"""Enumeration of the minimal covers of a Boolean matrix (dualization).

A set of columns *covers* a Boolean matrix when every row has a one in at least one of those
columns; a cover is *minimal* when no column can be dropped from it. Equivalently: the minimal
transversals (hitting sets) of the hypergraph whose edges are the rows.

A matrix is given as

* a sequence of rows, each a sequence of the 0-based column indices of its ones
  (``[[0, 1], [1, 2], [0, 2]]``);
* a 2-D numpy array (or anything with ``__array__``): nonzero entries are ones;
* a path (``str`` or ``os.PathLike``) of a ``.hg`` file: one row per line, space-separated column
  indices.

A matrix without rows has exactly one minimal cover, the empty one; a matrix with an all-zero row
has none. Covers are returned as lists of column indices in increasing order.

Example::

    >>> import dual
    >>> sorted(dual.enumerate_covers([[0, 1], [1, 2], [0, 2]]))
    [[0, 1], [0, 2], [1, 2]]
    >>> dual.count_covers([[0, 1], [1, 2], [0, 2]])
    3
"""

from __future__ import annotations

import os
from importlib.metadata import PackageNotFoundError
from importlib.metadata import version as _version
from typing import Any, Iterator, List, Sequence, Union

from ._dual import ALGORITHMS, Dualizer

__all__ = [
    "ALGORITHMS",
    "Dualizer",
    "MatrixLike",
    "count_covers",
    "enumerate_covers",
    "iter_batches",
    "make_dualizer",
    "minimal_covers",
]

try:  # the version of the installed distribution
    __version__ = _version("dual")
except PackageNotFoundError:  # running from a build tree
    __version__ = "0+unknown"

#: Accepted matrix inputs: rows of column indices, a 2-D array, or a path of a ``.hg`` file.
MatrixLike = Union[Sequence[Sequence[int]], "os.PathLike[str]", str, Any]

#: Default number of covers fetched from the C++ side at once by the generators.
DEFAULT_BATCH_SIZE = 1024


def make_dualizer(matrix: MatrixLike, algorithm: str = "runcm") -> Dualizer:
    """Create a :class:`Dualizer` for ``algorithm`` and set its matrix.

    :param matrix: rows of column indices, a 2-D array, or a path of a ``.hg`` file.
    :param algorithm: one of :data:`ALGORITHMS`; ``"runcm"`` (default), ``"runc"`` and ``"opt"``
        resume the search between portions, the others compute all covers at once.
    :raises ValueError: unknown algorithm, negative column index, or a malformed array.
    :raises FileNotFoundError: the ``.hg`` file does not exist.
    """
    dualizer = Dualizer(algorithm)
    if isinstance(matrix, (str, os.PathLike)):
        path = os.fspath(matrix)
        if not os.path.isfile(path):
            raise FileNotFoundError(path)
        dualizer.load_hg(path)
    elif hasattr(matrix, "__array__") or type(matrix).__module__.startswith("numpy"):
        import numpy  # only needed for array input

        array = numpy.asarray(matrix)
        if array.ndim != 2:
            raise ValueError("a dense matrix must be 2-dimensional, got shape %r" % (array.shape,))
        dualizer.set_dense(numpy.ascontiguousarray(array != 0))
    else:
        dualizer.set_rows([[int(j) for j in row] for row in matrix])
    return dualizer


def iter_batches(
    matrix: MatrixLike, algorithm: str = "runcm", batch_size: int = DEFAULT_BATCH_SIZE
) -> Iterator[List[List[int]]]:
    """Generate the minimal covers in portions (lists) of at most ``batch_size`` covers.

    The search runs in C++ between portions with the GIL released and is resumed on demand, so
    stopping the iteration early (``break``, ``close()``) stops the search and frees its state.
    """
    if batch_size < 1:
        raise ValueError("batch_size must be at least 1")
    dualizer = make_dualizer(matrix, algorithm)
    while True:
        batch = dualizer.next_batch(batch_size)
        if not batch:
            return
        yield batch


def enumerate_covers(
    matrix: MatrixLike, algorithm: str = "runcm", batch_size: int = DEFAULT_BATCH_SIZE
) -> Iterator[List[int]]:
    """Generate the minimal covers of ``matrix`` one by one.

    Covers are fetched from C++ in portions of ``batch_size`` (see :func:`iter_batches`); each
    cover is a list of 0-based column indices in increasing order. The order of the covers
    depends on the algorithm.
    """
    for batch in iter_batches(matrix, algorithm, batch_size):
        yield from batch


def minimal_covers(matrix: MatrixLike, algorithm: str = "runcm") -> List[List[int]]:
    """Return all minimal covers of ``matrix`` as a list (see :func:`enumerate_covers`)."""
    return list(enumerate_covers(matrix, algorithm))


def count_covers(matrix: MatrixLike, algorithm: str = "runcm") -> int:
    """Count the minimal covers of ``matrix`` without materializing them in Python."""
    return make_dualizer(matrix, algorithm).count()
