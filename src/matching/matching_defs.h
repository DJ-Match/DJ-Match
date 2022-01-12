/**
 * Copyright (C) 2021, 2022 : Kathrin Hanauer, Jonathan Trummer
 *
 * This file is part of DJMatch and licensed under GPLv3.
 */

#pragma once

enum AggregateType {SUM,MAX,AVG,MEDIAN,B_SUM};
enum MatchingAlgorithm {BGREEDY_COLOR, BGREEDY_EXTEND, GREEDY_IT, GPA_IT, NODE_CENTERED, K_EC };

typedef unsigned long int EdgeWeight;
