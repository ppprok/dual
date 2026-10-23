/// @file
/// @brief Directed graph algorithms on adjacency matrices stored as `bit_matrix`.

#pragma once

#include <stdexcept>

#include <dual/bit_matrix.h>

namespace dual {

/// @brief Number of edges (set bits) of a graph given by its adjacency matrix.
int edges_number(bit_matrix& adj_mat);

/// @brief Reverse all edges: `redirected` becomes the transposed `n x n` adjacency matrix,
///        where `n = adj_mat.height()`.
void redirect_graph(bit_matrix const& adj_mat, bit_matrix& redirected);

/// @brief Check whether @p i is a vertex, i.e. a row index of the adjacency matrix.
bool is_vertex(bit_matrix& adj_mat, int i);

/// @brief Build the `n x n` adjacency matrix of a directed graph.
/// @tparam Edges Container of pairs `(from, to)` with `first`/`second` members.
/// @throws std::runtime_error If an edge has a vertex outside `[0, n)`.
template<typename Edges>
void create_adjacency_matrix(int n, Edges const& edges, bit_matrix& adj_mat) {
    adj_mat.zero(n, n);
    for (auto& e : edges) {
        if (!is_vertex(adj_mat, e.first) || !is_vertex(adj_mat, e.second))
            throw std::runtime_error("vertex out of range");

        adj_mat.set(e.first, e.second);
    }
}

/// @brief Simple algorithm to build the transitive closure, in place, of a graph given by its
///        adjacency matrix (rows are OR-ed with the rows of their successors until nothing changes).
void transitive_closure(bit_matrix& adj_mat);

/// @brief Check whether the graph given by its adjacency matrix is transitively closed.
bool is_transitive_closed(bit_matrix const& adj_mat);

/// @brief Get the topological order of the vertices of a directed acyclic graph given by its
///        adjacency matrix.
/// @param adj_mat Adjacency matrix; an edge `i -> j` is bit `(i, j)`.
/// @param[out] order `order[v]` is the position of vertex `v`; every edge goes from a smaller
///             position to a larger one.
/// @throws std::runtime_error If the graph has a cycle.
void topological_order(bit_matrix const& adj_mat, std::vector<int>& order);

/// @brief Check that @p order (vertex -> position, as produced by `topological_order`) is a
///        topological order of the graph.
/// @throws std::invalid_argument If `order.size()` differs from the number of vertices.
bool is_topological_order(bit_matrix const& adj_mat, std::vector<int> const& order);

}  // namespace dual
