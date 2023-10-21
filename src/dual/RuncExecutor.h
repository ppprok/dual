#pragma once

#include <stdint.h>

typedef void* DualizerHandler;

extern "C" {
DualizerHandler init_dualizer();
void release_dualizer(DualizerHandler dualizer);
int dualize(DualizerHandler dualizer);
void set_input_row(DualizerHandler dualizer, int32_t row_num, int32_t ones_num, int32_t* ones);
uint32_t enumerate_covers(DualizerHandler dualizer, uint32_t buffer_size, uint32_t* covers);

}