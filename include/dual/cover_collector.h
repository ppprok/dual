/// @file
/// @brief `CoverCollector`: a callback that stores the found covers.

#pragma once

#include <dual/dualization_backtrack_algorithm_base.h>
#include <dual/weighted_cover.h>

/// @brief Collects the enumerated covers; install it as `CoverCallback`.
class CoverCollector : public IDualizationCallback {
protected:
    /// Collected covers.
    WeightedCovers _covers;

public:
    /// @brief Empty collector without a limit.
    CoverCollector() : MaxCount(-1) {};

    /// Stop the enumeration (`DualizationNode::stoped`) once this many covers are collected;
    /// no limit if not positive (default -1).
    int MaxCount;

    /// @brief Store the columns and weight of @p node if `node.IsCover()`.
    virtual void Call(DualizationNode& node);

    /// @brief The collected covers, in the order they were found.
    WeightedCovers const& GetCovers() const;

    /// @brief Move the collected covers out, leaving the collector in a valid, unspecified state
    ///        (call `Reset` before reuse).
    WeightedCovers MoveCovers();

    /// @brief Drop the collected covers.
    void Reset();

    /// @brief Reserve memory for @p size covers.
    void Reserve(int size);
};
