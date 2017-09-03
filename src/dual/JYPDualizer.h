#pragma once
#include "bit_matrix.h"
#include "bit_vector.h"
#include "bit_mask.h"


class JYPDualizer
{

    bit_matrix _intermediate;

    bit_matrix _input;

    bit_matrix _output;

    mutable bit_vector _mask;

    mutable bit_vector _buffer;

    mutable bit_vector _buffer2;
    
    void expand_to_maximal_indepenedent_set(bit_chunk columns) const
    {
        int m = _input.height();
        int n = _input.width();
        for (int j = 0; j != n; ++j)
        {
            if (columns.get(j))
                continue;

            columns.set(j);
            for (auto& r : _input.get_rows())
                if (r.is_subset_of(columns))
                {
                    columns.set(j, false);
                    break;
                }
        }
    }

    bool is_maximal_independent_set(bit_chunk columns, int width) const
    {
        int m = _input.height();
        int n = _input.width();
        assert(width > 0 && width <= n);
        _mask.zero(width);
        _buffer.zero(width);
        _buffer2.zero(width);

        for (int j = 0; j != width; ++j)
            _mask.set(j);

        for (int i = 0; i != m; ++i)
        {
            
            _buffer.assign_sub(_input.get_row(i), columns);
            int c = bit_mask(_mask).count(_buffer, 2);
            if (c == 0)
                return 0;
            if (c > 1)
                continue;
            _buffer.assign_and(_buffer, _mask);
            _buffer2.assign_or(_buffer2, _buffer);
        }

        return bit_mask(_mask).subset_cmp(columns, _buffer2) == 0;
    }

    void Dualize(bit_matrix input)
    {
        _input = std::move(input);
        int n = _input.width();

        _intermediate.zero(0, n);
        _output.zero(0, n)

    }

};