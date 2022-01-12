/**
 * Copyright (C) 2021, 2022 : Kathrin Hanauer, Jonathan Trummer
 *
 * This file is part of DJMatch and licensed under GPLv3.
 */

#include <algorithm>
#include "greedy_b_matching.h"

#include "graph.incidencelist/incidencelistgraph.h"
#include "coloring/misra_gries.h"

void greedy_b_matching::run() {
    using namespace Algora;
    std::vector<Arc*> edges;
    edges.reserve(diGraph->getNumArcs(false));

    auto *iGraph = dynamic_cast<IncidenceListGraph*>(diGraph);
    iGraph->mapArcs([this,&edges,iGraph] (Arc * arc) {
        if (weights->getValue(arc) > 0) {
            edges.push_back(arc);
        } else  {
            iGraph->deactivateArc(arc);
        }
    });

    std::sort(edges.begin(), edges.end(), [this] (const Arc* lop, const Arc* rop) {
        return weights->getValue(lop) > weights->getValue(rop);
    });

    assert(num_matchings >= 1);
    unsigned b = do_extend ? num_matchings - 1 : num_matchings;

    FastPropertyMap<unsigned int> num_matched(0, "num_matched", max_vertex_id);

    std::vector<Arc*> unmatched_arcs;
    unmatched_arcs.reserve(edges.size());
    assert(iGraph);
    for (auto & arc : edges) {
        if (num_matched[arc->getHead()] < b && num_matched[arc->getTail()] < b) {
            num_matched[arc->getHead()]++;
            num_matched[arc->getTail()]++;
            total_weight += weights->getValue(arc);
        } else {
            unmatched_arcs.push_back(arc);
            iGraph->deactivateArc(arc);
        }
    }

    MisraGries mg(config);
    mg.setGraph(diGraph);
    mg.setMaxDegree(b);
    mg.prepare();
    mg.run();
    std::swap(mate, mg.mate);
    std::swap(edge_color, mg.edge_color);
    // misra gries adds one color, some color will be nullified

    if (!do_extend) {
        num_matchings++;
        if (num_matchings == mg.getNumColors()) {
            postprocess();
        }
        // discard last color
        mate.pop_back();
        num_matchings--;
    }

    for (auto & arc : unmatched_arcs) {
        diGraph->activateArc(arc);
    }

    if (do_extend) {
        greedy_extend(unmatched_arcs);
    }

    if (do_global_swaps) {
        global_swaps();
    }

    if (config.sanitycheck) {
        sanityCheck();
    }
}

// postprocess after coloring of b-matching
// will most definitely contain b+1 colors
// here we then nullify the lightest color (ie. with smallest weight)
void greedy_b_matching::postprocess() {
    using namespace Algora;

    // accumulate weight per color
    std::vector<unsigned long> color_weight(num_matchings, 0);
    diGraph->mapArcs([&color_weight, this] (Arc * arc) {
        assert(edge_color[arc] < num_matchings);
        color_weight[edge_color[arc]] += weights->getValue(arc);
    });

    auto it_min_color = std::min_element(color_weight.begin(), color_weight.end());
    assert(it_min_color - color_weight.begin() >= 0);
    unsigned int min_color = it_min_color - color_weight.begin();
    auto min_weight = *it_min_color;

    // adjust weight
    total_weight -= min_weight;

    // unset matching of min-weight color
    // re-color edges with "last" color if != min_color
    diGraph->mapArcs([&min_color, this] (Arc * arc) {
        if (edge_color[arc] == min_color) {
            edge_color[arc] = UNCOLORED;
        } else if (edge_color[arc] + 1 == num_matchings) {
            edge_color[arc] = min_color;
        }
    });

    if (min_color + 1 < num_matchings) {
        // and unset mates of the min-weight color
        std::swap(mate.back(), mate[min_color]);
    }
}

void greedy_b_matching::greedy_extend(std::vector<Algora::Arc *> &edges) {

    std::vector<Algora::Arc*> remaining_edges;
    remaining_edges.reserve(edges.size());
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
            }
		}
        std::swap(edges, remaining_edges);
        remaining_edges.clear();
	}
}
