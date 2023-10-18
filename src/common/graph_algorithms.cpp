
#include <boost/format.hpp>

#include "bit_vector.h"
#include "graph_algorithms.h"

void transitive_closure(bit_matrix& adj_mat) {
    int n = adj_mat.height();

    for (int c = 1; c;) {
        c = 0;
        for (int i = 0; i != n; ++i) {
            auto& row = adj_mat.get_row(i);
            for (int t = row.find_first(); t != -1 && t < n; t = row.find_next(t)) {
                if (i == t)
                    continue;

                auto& row1 = adj_mat.get_row(t);
                if (row1.is_subset_of(row) || row1.none())
                    continue;

                row.assign_or(row, row1);
                ++c;
            }
        }
    }
}

void do_topological_order(int n, int i, bit_matrix const& adj_mat, std::vector<int>& order) {
    order[i] = 1;

    for (int t = 0; t != n; ++t) {
        if (i == t)
            continue;

        if (adj_mat.get(i, t)) {
            if (order[t] == 1)
                BOOST_THROW_EXCEPTION(not_acyclic_graph_error()
                                      << message(boost::str(boost::format("Edge (%1%, %2%) close cycle") % i % t)));
            if (order[t] == 0)
                do_topological_order(n, t, adj_mat, order);
        }
    }

    order[i] = 2;
    order.push_back(i);
}

void topological_order(bit_matrix const& adj_mat, std::vector<int>& order) {
    int n = adj_mat.height();

    order.clear();
    order.reserve(n * 2);
    order.resize(n, 0);

    for (int i = 0; i != n; ++i) {
        if (order[i] == 0)
            do_topological_order(n, i, adj_mat, order);
    }

    for (int t = 0; t != n; ++t) {
        order[order.back()] = t;
        order.pop_back();
    }
}

bool is_topological_order(bit_matrix const& adj_mat, std::vector<int> const& order) {
    int n = adj_mat.height();
    if (n != order.size())
        BOOST_THROW_EXCEPTION(std::invalid_argument("Length of array 'order' must be equal to number of verticies"));

    if (n == 0)
        return true;

    std::vector<int> verticies(n);

    for (int t = 0; t != n; ++t)
        verticies[order[t]] = t;

    for (int i = 0; i != n - 1; ++i) {
        for (int j = i + 1; j != n; ++j) {
            if (adj_mat.get(verticies[j], verticies[i]))
                return false;
        }
    }

    return true;
}

bool is_transitive_closed(bit_matrix const& adj_mat) {
    int n = adj_mat.size();
    for (int i = 0; i != n; ++i) {
        auto& row = adj_mat.get_row(i);

        for (int j = row.find_first(); j != -1; j = row.find_next(j)) {
            auto& row1 = adj_mat.get_row(j);
            for (int t = row1.find_first(); t != -1; t = row1.find_next(t)) {
                if (!row.get(t))
                    return false;
            }
        }
    }

    return true;
}

int edges_number(bit_matrix& adj_mat) {
    int c = 0;
    for (auto& r : adj_mat.get_rows())
        c += r.count();

    return c;
}

void redirect_graph(bit_matrix const& adj_mat, bit_matrix& redirected) {
    int n = adj_mat.height();
    redirected.zero(n, n);
    for (int i = 0; i != n; ++i) {
        auto& row = adj_mat.get_row(i);
        for (int j = row.find_first(); j != -1; j = row.find_next(j))
            redirected.set(j, i);
    }
}

bool is_vertex(bit_matrix& adj_mat, int i) {
    return inside(adj_mat.get_rows(), i);
}