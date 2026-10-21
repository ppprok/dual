/// @file
/// @brief Collectors that keep covers as a bit matrix or keep only the best N covers.

#pragma once
#include <random>

#include "CoverCollector.h"
#include "DualizationBacktrackAlgorithmBase.h"
#include "Pruning.h"
#include "WeightedCover.h"
#include "Weighters.h"

/// @brief Collects the enumerated covers as rows of a bit matrix; install it as `CoverCallback`.
class BitCoverCollectorCallback : public IDualizationCallback {
protected:
    /// Collected covers, one row per cover (bit `j` set for every column `j` of the cover).
    bit_matrix _covers;

    /// Weights of the collected covers, by row.
    Weights _weights;

public:
    /// @brief Append @p node as a row if it is a cover node (`NodeType::CoverNode`).
    /// @note Rows of empty covers are not stored (a cover adds a row only through its columns),
    ///       while their weights are.
    virtual void Call(DualizationNode& node);

    /// @brief The collected covers.
    bit_matrix const& GetCovers() const;

    /// @brief The weights of the collected covers.
    Weights const& GetWeights() const;

    /// @brief Move the collected covers out.
    bit_matrix MoveCovers();

    /// @brief Move the weights out.
    Weights MoveWeights();

    /// @brief Drop the collected covers and weights.
    void Reset();
};

/// @brief Options of `MostCoverCollector`.
struct MostCoverCollectorOptions {
    /// Size starting from which the collection is truncated: once it holds more covers, it is
    /// truncated to `MostCount`.
    int TrimSize;

    /// Number of covers to keep.
    int MostCount;

    /// Shuffle the covers before truncation, so that ties are broken randomly.
    bool RandomShuffle;

    /// Truncation order: less than zero --- keep the lightest covers, greater than zero --- keep the
    /// heaviest covers, 0 --- keep random ones. Ties are broken by fewer columns.
    int TrimOrder;

    /// Seed of the random order, applied on the first truncation.
    unsigned Seed;

    // Stop after the given number of covers
    //int StopAfter;

    /// @brief Keep @p mostCount covers (`> 0`), truncating when there are more than @p trimSize
    ///        (at least `2 * mostCount` if @p trimSize is smaller than @p mostCount).
    MostCoverCollectorOptions(int mostCount = 100, int trimSize = 0, int trimOrder = -1, bool randomShuffle = true) {
        assert(mostCount > 0);
        if (trimSize < mostCount)
            trimSize = mostCount * 2;

        MostCount = mostCount;
        TrimSize = trimSize;
        TrimOrder = trimOrder;
        RandomShuffle = randomShuffle;
        Seed = std::mt19937::default_seed;
        //StopAfter = -1;
    }
};

/// @brief Collector that keeps the N best covers (by weight, see `MostCoverCollectorOptions`).
///
/// Covers are collected until there are more than `Options.TrimSize`; then the collection is
/// truncated to `Options.MostCount`. With a `Weighter`, the weight of the worst kept cover becomes
/// the weighter's bound, so that branches that cannot beat it are pruned. Call `Trim()` after the
/// enumeration to get at most `MostCount` covers.
class MostCoverCollector : public CoverCollector {

    int _counter;

    // Random order engine, seeded from Options.Seed on the first truncation
    std::mt19937 _random;
    bool _seeded = false;

public:
    /// Truncation options.
    MostCoverCollectorOptions Options;

    /// Weighter of search tree nodes (not owned, may be null).
    WeighterBase* Weighter;

    /// @brief Initialize with an object that weighs search tree nodes (optional, not owned).
    MostCoverCollector(WeighterBase* weighter = 0);

    /// @brief Process a search tree node: store a cover that the weighter does not prune and
    ///        truncate the collection when it grows past `Options.TrimSize`.
    virtual void Call(DualizationNode& node);

    /// @brief Truncate the collection of covers to `Options.MostCount`.
    void Trim();

    /// @brief Truncate the collection of covers to the best @p trimSize ones (ordered by
    ///        `Options.TrimOrder`).
    void Trim(int trimSize);
};
