#pragma once
#include <random>

#include "CoverCollector.h"
#include "DualizationBacktrackAlgorithmBase.h"
#include "Pruning.h"
#include "WeightedCover.h"
#include "Weighters.h"

// Collects the enumerated covers
class BitCoverCollectorCallback : public IDualizationCallback {
protected:
    bit_matrix _covers;

    Weights _weights;

public:
    virtual void Call(DualizationNode& node);

    bit_matrix const& GetCovers() const;

    Weights const& GetWeights() const;

    bit_matrix MoveCovers();

    Weights MoveWeights();

    void Reset();
};

struct MostCoverCollectorOptions {
    // Size starting from which the collection is truncated
    int TrimSize;

    // Number of covers to keep
    int MostCount;

    // Random order
    bool RandomShuffle;

    // Truncation order (less than zero --- keep the heaviest covers,
    // greater than zero --- keep the lightest covers, 0 --- keep random ones)
    int TrimOrder;

    // Seed of the random order, applied on the first truncation
    unsigned Seed;

    // Stop after the given number of covers
    //int StopAfter;

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

// Collector that keeps the N best covers
class MostCoverCollector : public CoverCollector {

    int _counter;

    // Random order engine, seeded from Options.Seed on the first truncation
    std::mt19937 _random;
    bool _seeded = false;

public:
    // Truncation options
    MostCoverCollectorOptions Options;

    // Weighter of search tree nodes
    WeighterBase* Weighter;

    // Initialize with an object that weighs search tree nodes (optional)
    MostCoverCollector(WeighterBase* weighter = 0);

    // Process a search tree node
    virtual void Call(DualizationNode& node);

    // Truncate the collection of covers to the size from the options
    void Trim();

    // Truncate the collection of covers
    void Trim(int trimSize);
};