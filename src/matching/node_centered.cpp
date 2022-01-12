/**
 * Copyright (C) 2021, 2022 : Kathrin Hanauer, Jonathan Trummer
 *
 * This file is part of DJMatch and licensed under GPLv3.
 */

#include <iostream>
#include "node_centered.h"

#include "graph/arc.h"

void node_centered::run() {
	using namespace Algora;
	auto global_max = 0ul;
	FastPropertyMap<std::vector<Arc*>> edges(std::vector<Arc*>(), "", max_vertex_id);
	FastPropertyMap<EdgeWeight> node_weights(0, "", max_vertex_id);
	std::vector<Vertex*> nodes;
    nodes.reserve(diGraph->getSize());
	diGraph->mapVertices([this,&nodes,&node_weights,&global_max,&edges] (Vertex* v) {
        edges[v].reserve(diGraph->getDegree(v,false));
	    auto am = [this,v,&edges] (Arc * arc) {

	    	if (weights->getValue(arc) > 0) {
	    		edges[v].push_back(arc);
	    	}
	    };
		diGraph->mapOutgoingArcs(v, am);
		diGraph->mapIncomingArcs(v, am);
		nodes.push_back(v);

		std::sort(edges[v].begin(), edges[v].end(), [this] (Arc * lop, Arc * rop) {
			return weights->getValue(lop) > weights->getValue(rop);
		});
        if (!edges[v].empty() && global_max < weights->getValue(edges[v].front())) {
            global_max = weights->getValue(edges[v].front());
        }
		node_weights[v] = edges[v].empty() ? 0 : aggregateWeights(edges[v], *weights);
	});

	std::sort(nodes.begin(), nodes.end(), [&node_weights](Vertex * lop, Vertex * rop) {
		return node_weights[lop] > node_weights[rop];
	});

	EdgeWeight global_threshold = threshold > 0 ? global_max * threshold : 0;

	FastPropertyMap<unsigned> num_matching(0, "", max_vertex_id);

	FastPropertyMap<std::vector<char>> free_colors(std::vector<char>(num_matchings, true), "", max_vertex_id);

	for (const auto &v : nodes) {
		for (const auto &arc : edges[v]) {
			if (num_matching[v] >= num_matchings || weights->getValue(arc) < global_threshold) {
				break;
			}

			if (edge_color[arc] == UNCOLORED) {
				const auto s = arc->getFirst();
				const auto t = arc->getSecond();
				const auto color = first_free_matching_color(free_colors[s], free_colors[t]);
				if (color < num_matchings) {
					edge_color[arc] = color;
					num_matching[s]++;
					free_colors[s][color] = false;
					num_matching[t]++;
					free_colors[t][color] = false;
					total_weight += weights->getValue(arc);

					mate[color][s] = t;
					mate[color][t] = s;
				}
			}
		}
	}

	if (threshold > 0) {
		std::vector<Arc*> left_edges;
		diGraph->mapArcs([this,&num_matching,&left_edges](Arc* arc) {
			if (edge_color[arc] < num_matchings) {
				return;
			}
			if (num_matching[arc->getFirst()] >= num_matchings
                    || num_matching[arc->getSecond()] >= num_matchings) {
				return;
			}
			left_edges.push_back(arc);
		});

		std::sort(left_edges.begin(), left_edges.end(), [this] (Arc * rop, Arc * lop) {
			return weights->getValue(rop) > weights->getValue(lop);
		});

		for (const auto & arc : left_edges) {
			const auto s = arc->getFirst();
			const auto t = arc->getSecond();
			if (num_matching[s] >= num_matchings
                    || num_matching[t] >= num_matchings
                    || edge_color[arc] < num_matchings) {
				continue;
			}
			auto color = first_free_matching_color(free_colors[s], free_colors[t]);

			if (color < num_matchings) {
				edge_color[arc] = color;
				free_colors[s][color] = false;
				free_colors[t][color] = false;
				num_matching[s]++;
				num_matching[t]++;

				total_weight += weights->getValue(arc);
			}
		}
	}

	if (config.sanitycheck) {
		sanityCheck();
	}
}
