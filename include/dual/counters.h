/// @file
/// @brief `Counters`: a callback that counts search tree nodes.

#pragma once
#include <iostream>

#include <dual/dualization_backtrack_algorithm_base.h>

namespace dual {

/// @brief Callback that counts search tree nodes by type.
///
/// Install it as both `InnerCallback` (to count inner nodes) and `CoverCallback` (to count covers).
struct Counters : IDualizationCallback {
    /// Number of cover nodes.
    int cover;
    /// Number of extra nodes.
    int extra;
    /// Number of new (inner) nodes.
    int inner;
    /// Total size of the covers (for the average cover length).
    double length;

    /// @brief Zero counters.
    Counters() : cover(0), extra(0), inner(0), length(0) {}

    /// @brief Zero the counters.
    void Reset() {
        *this = Counters();
    }

    /// @brief Count @p node according to its type.
    void Call(DualizationNode& node) {
        switch (node.type) {
            case NodeType::CoverNode:
                length += node.columns.size();
                ++cover;
                break;
            case NodeType::NewNode:
                ++inner;
                break;
            case NodeType::ExtraNode:
                ++extra;
                break;
        }
    }

    /// @brief Print the counts and the average cover length to `std::cout`.
    void Print() const {
        std::cout << "count: " << cover << '\n';
        std::cout << "inner: " << inner << '\n';
        std::cout << "extra: " << extra << '\n';
        std::cout << "avgl: " << (cover > 0 ? (length / cover) : 0) << '\n';
    }
};

}  // namespace dual
