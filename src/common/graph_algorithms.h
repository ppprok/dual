#pragma once

#include "bit_matrix.h"
#include "errors.h"

struct not_acyclic_graph_error : virtual error {};

struct vertex_out_of_range : virtual error {};

int edges_number(bit_matrix& adj_mat);

void redirect_graph(bit_matrix const& adj_mat, bit_matrix& redirected);

bool is_vertex(bit_matrix& adj_mat, int i);

template<typename Edges>
void create_adjacency_matrix(int n, Edges const& edges, bit_matrix& adj_mat) {
    adj_mat.zero(n, n);
    for (auto& e : edges) {
        if (!is_vertex(adj_mat, e.first) || !is_vertex(adj_mat, e.second))
            BOOST_THROW_EXCEPTION(vertex_out_of_range());

        adj_mat.set(e.first, e.second);
    }
}

// Simple algorithm for build transitive closure for graph
// presented by adjacency matrix
void transitive_closure(bit_matrix& adj_mat);

bool is_transitive_closed(bit_matrix const& adj_mat);

// Get order of topological sorted verticies
// of directed graph by adjacency matrix
void topological_order(bit_matrix const& adj_mat, std::vector<int>& order);

bool is_topological_order(bit_matrix const& adj_mat, std::vector<int> const& order);