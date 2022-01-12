/**
 * Copyright (C) 2021, 2022 : Kathrin Hanauer, Jonathan Trummer
 *
 * This file is part of DJMatch and licensed under GPLv3.
 */

#include "greedy_iterative.h"
#include <iostream>

void greedy_iterative::run() {
	using namespace Algora;

    std::vector<Arc*> edges;
    edges.reserve(diGraph->getNumArcs(false));
	diGraph->mapArcs([this, &edges] (Arc *arc) {
		if (weights->getValue(arc) > 0) {
			edges.push_back(arc);
		}
	});

	std::sort(edges.begin(), edges.end(), [this](const Arc *lop, const Arc *rop) {
		return weights->getValue(lop) > weights->getValue(rop);
	});

    std::vector<Algora::Arc*> remaining_edges;
    remaining_edges.reserve(edges.size());
    matched_in_round.reserve(edges.size());
	for (auto bi = 0u; bi < num_matchings; bi++) {
		for (const auto & arc : edges) {
			if (edge_color[arc] != UNCOLORED) {
				continue;
			}
			const auto s = arc->getFirst();
			const auto t = arc->getSecond();

			// can't match edge in this round, as at least one endpoint
			// is already not free anymore
			if (mate[bi][s] != nullptr || mate[bi][t] != nullptr) {
				remaining_edges.push_back(arc);
			} else {
                mate[bi][s] = t;
                mate[bi][t] = s;
                edge_color[arc] = bi;
                total_weight += (*weights)[arc];
                matched_in_round.push_back(arc);
            }
		}

		if (swaps && !do_global_swaps) {
			bool swapped = local_swaps(bi);
			// with local swaps instead of re-sorting the shortened edge
			// vector, keep the complete vector, unless nothing was swapped
			if (!swapped) {
                std::swap(edges, remaining_edges);
			}
		} else {
			// without local swaps we can shrink the vector down to those
			// edges, that weren't matched in that round
            std::swap(edges, remaining_edges);
		}
        matched_in_round.clear();
        remaining_edges.clear();
	}

	if (swaps && do_global_swaps) {
		global_swaps();
	}

	if (config.sanitycheck) {
		sanityCheck();
	}
}

bool greedy_iterative::local_swaps(const unsigned int round) {
	using namespace Algora;
	bool succ = false;
	if (config.swaps_reverse_sort) {
        for (auto it = matched_in_round.rbegin(); it != matched_in_round.rend(); it++) {
            succ |= swap_subroutine(*it, round);
        }
    } else {
        for (auto *arc : matched_in_round) {
            succ |= swap_subroutine(arc, round);
        }
    }
	return succ;
}

