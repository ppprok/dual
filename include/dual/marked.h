/// @file
/// @brief `marked`: an object with an attached integer mark.

#pragma once

#include <limits.h>

namespace dual {

/// @brief Integer mark attached to an object of the base type.
/// @tparam base Type to derive from.
template<typename base>
struct marked : public base {
    /// Mark value meaning "no mark".
    static const int no_value = INT_MIN;

    /// Mark value.
    int _value;

public:
    /// @brief Copy of @p b with mark @p value.
    marked(base b = base(), int value = no_value) : base(b), _value(value) {}

    /// @brief Check whether a mark is set.
    bool has_value() const {
        return _value != no_value;
    }

    /// @brief Set the mark; by default remove it.
    void reset_value(int value = no_value) {
        _value = value;
    }

    /// @brief Set the mark.
    void set_value(int value) {
        _value = value;
    }

    /// @brief Get the mark (`no_value` if there is none).
    int get_value() const {
        return _value;
    }
};

}  // namespace dual
