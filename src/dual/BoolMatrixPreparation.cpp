#include "BoolMatrixPreparation.h"
#include "containers.h"

void BoolMatrixPreparation::RemoveDependentRows(bit_chunks& rows) {
    int c = 0;

    for (int i = 0, s = rows.size(); i < s - 1; ++i) {
        if (!rows[i].begin)
            continue;

        for (int j = i + 1; j < s; ++j) {
            if (!rows[j].begin)
                continue;

            int b = bit_chunk::subset_cmp(rows[i], rows[j]);

            if (b < 0) {
                rows[j].begin = 0;
                ++c;
            }

            if (b > 0) {
                rows[i].begin = 0;
                ++c;
                break;
            }
        }
    }

    if (c > 0)
        pop_if(rows, [](bit_chunk r) { return !r.begin; });
}

void BoolMatrixPreparation::RemoveDependentRows(bit_chunks& rows, bit_mask const& mask) {
    int c = 0;

    for (int i = 0, s = rows.size(); i < s - 1; ++i) {
        if (!rows[i].begin)
            continue;

        for (int j = i + 1; j < s; ++j) {
            if (!rows[j].begin)
                continue;

            int b = mask.subset_cmp(rows[i], rows[j]);

            if (b < 0) {
                rows[j].begin = 0;
                ++c;
            }

            if (b > 0) {
                rows[i].begin = 0;
                ++c;
                break;
            }
        }
    }

    if (c > 0)
        pop_if(rows, [](bit_chunk r) { return !r.begin; });
}