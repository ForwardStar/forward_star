//
// Created by sjf on 9/2/2022.
//

#ifndef GRAPHINDEX_TC_H
#define GRAPHINDEX_TC_H

#include "benchmark.h"
#include "pvector.h"
#include "../radixgraph.h"

std::vector<double> OrderedCount(RadixGraph* g, uint32_t num_vertices);

#endif //GRAPHINDEX_TC_H
