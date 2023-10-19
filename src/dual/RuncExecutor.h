#pragma once

#include <stdint.h>

typedef void* DualizerHandler;

extern "C" {
DualizerHandler init_dualizer(int32_t m, int32_t n);
void release_dualizer(DualizerHandler dualizer);
void dualize(DualizerHandler dualizer);
void set_input_row(DualizerHandler dualizer, int32_t row_num, int32_t ones_num, int32_t* ones);

}