#pragma once
#include <string>

// Counters of dualization results
struct DualizationCounters {
    typedef long long int Int;

    Int results;       ///< Number of solutions found
    Int inners;        ///< Number of inner states
    Int extras;        ///< Number of extra steps
    Int currentDelay;  ///< Current delay
    Int maxDelay;      ///< Maximum delay

    // Reset the counters
    void Reset();

    // Constructor that zeroes the counters
    DualizationCounters();

    // Convert the counters to text
    std::string ToString() const;

    void IncResult() {
        ++results;
        if (results == 1)
            maxDelay = currentDelay;
        else if (maxDelay < currentDelay)
            maxDelay = currentDelay;
        currentDelay = 0;
    }

    void IncExtra() {
        ++extras;
        ++currentDelay;
    }

    void IncInners() {
        ++inners;
        ++currentDelay;
    }
};
