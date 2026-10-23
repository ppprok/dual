/// @file
/// @brief `DualizationCounters`: counters of results, inner states and delays.

#pragma once
#include <string>

namespace dual {

/// @brief Counters of dualization results, including the delay (number of steps) between results.
struct DualizationCounters {
    /// Counter type.
    typedef long long int Int;

    Int results;       ///< Number of solutions found
    Int inners;        ///< Number of inner states
    Int extras;        ///< Number of extra steps
    Int currentDelay;  ///< Current delay
    Int maxDelay;      ///< Maximum delay

    /// @brief Reset the counters.
    void Reset();

    /// @brief Constructor that zeroes the counters.
    DualizationCounters();

    /// @brief Convert the counters to text.
    std::string ToString() const;

    /// @brief Count a result: update `maxDelay` with the steps since the previous result and
    ///        restart `currentDelay`.
    void IncResult() {
        ++results;
        if (results == 1)
            maxDelay = currentDelay;
        else if (maxDelay < currentDelay)
            maxDelay = currentDelay;
        currentDelay = 0;
    }

    /// @brief Count an extra step.
    void IncExtra() {
        ++extras;
        ++currentDelay;
    }

    /// @brief Count an inner state.
    void IncInners() {
        ++inners;
        ++currentDelay;
    }
};

}  // namespace dual
