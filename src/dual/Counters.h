#pragma once
#include <iostream>

#include "DualizationBacktrackAlgorithmBase.h"

struct Counters : IDualizationCallback {
    int cover;
    int extra;
    int inner;
    double length;

    Counters() : cover(0), extra(0), inner(0), length(0) {}

    void Reset() {
        *this = Counters();
    }

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

    void Print() const {
        std::cout << "count: " << cover << '\n';
        std::cout << "inner: " << inner << '\n';
        std::cout << "extra: " << extra << '\n';
        std::cout << "avgl: " << (cover > 0 ? (length / cover) : 0) << '\n';
    }
};
