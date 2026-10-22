/// @file
/// @brief Small math helpers.

#pragma once

namespace dual {

/// @brief Square a value.
template<typename T>
T sqr(T const& v) {
    return v * v;
}

}  // namespace dual
