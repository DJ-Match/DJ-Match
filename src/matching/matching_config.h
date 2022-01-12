/**
 * Copyright (C) 2021, 2022 : Kathrin Hanauer, Jonathan Trummer
 *
 * This file is part of DJMatch and licensed under GPLv3.
 */

#pragma once
#include <vector>
#include <string>

#include "matching_defs.h"

struct MatchingConfig {
    bool gpa{false};
    bool random{false};
    bool swaps{false};
    bool swaps_and_normal{false};
    bool swaps_reverse_sort{false};
    bool swaps_global{false};
    
    std::vector<int> all_bs;
    unsigned b{1};
    bool greedy{false};
    bool greedycoloring{false};
    bool node_centered{false};
    bool sanitycheck{false};
    std::string graph_filename;

    unsigned int n;
    unsigned int m;

    std::string outputFile = "";
    bool writeOutputfile{false};

    bool console_log{false};

    std::vector<double> global_thresholds;

    // l for GPA-ROMA
    int l{-1};
    bool roma{false};

    int seed{123};
    unsigned algorithm_order_seed{0};

    std::vector<MatchingAlgorithm> algorithms;
    std::vector<AggregateType> aggregation_types;
};
