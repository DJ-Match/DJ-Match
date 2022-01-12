/**
 * Copyright (C) 2021, 2022 : Kathrin Hanauer, Jonathan Trummer
 *
 * This file is part of DJMatch and licensed under GPLv3.
 */

#include "matching/gpa/gpa.h"

#include <algorithm>
#include <deque>
#include <vector>

void gpa::run() {
    using namespace Algora;
    std::vector<Arc *> edges;
    edges.reserve(diGraph->getNumArcs(false));

    diGraph->mapArcs([this, &edges] (Arc * arc) {
        if (weights->getValue(arc) > 0) {
            edges.push_back(arc);
        }
    });

    // for roma: set of all vertices
    if (num_roma > 0) {
        diGraph->mapVertices([&] (Vertex* v) {
            all_vertices.push_back(v);
        });
    }

    rng.seed(config.seed);

    // shuffle vector for random tie breaking
    std::shuffle(edges.begin(), edges.end(), rng);
    // then sort by weight descending
    std::sort(edges.begin(), edges.end(), [this] (const Arc * lop, const Arc * rop) {
        return (*weights)[lop] > (*weights)[rop];
    });

    std::vector<Arc *> edges_remaining;
    for (auto bi = 0u; bi < config.b; bi++) {
        path_set pathset(diGraph, max_vertex_id);
        for (auto arc : edges) {
            if (edge_color[arc] < config.b) {
                continue;
            }
            pathset.add_if_applicable(arc);
            edges_remaining.push_back(arc);
        }
        extract_paths_apply_matching(pathset, bi);
        if (num_roma > 0) {
            roma(bi);
        } else if (swaps && !do_global_swaps) {
            local_swaps(bi);
        }
        matched_in_round.clear();
        std::swap(edges, edges_remaining);
        edges_remaining.clear();
    }

    if (config.sanitycheck) {
        sanityCheck();
    }
}

//  utilize swap subroutine from parent abstract class
bool gpa::local_swaps(const unsigned int round) {
	using namespace Algora;
	auto vector_size = matched_in_round.size();
	bool succ = false;
	for (unsigned i = 0; i < vector_size; i++) {
		// reverse sort? access vector from back, otherwise in normal order
		const auto index = config.swaps_reverse_sort ? vector_size-1-i : i;
		const auto arc = matched_in_round[index];

		succ |= swap_subroutine(arc, round);
	}
	return succ;
}

bool gpa::roma(const unsigned int round) {
    using namespace Algora;
    bool succ = false;
    std::shuffle(all_vertices.begin(), all_vertices.end(), rng);
    std::vector<Vertex*> current_vertices(all_vertices);
    std::vector<Vertex*> changed;
    int l = 0;
    while (!current_vertices.empty() && l++ < num_roma) {
        for (const auto v : current_vertices) {
            if (mate[round][v] != nullptr) {
                auto arc = diGraph->findArc(v, mate[round][v]);
                if (arc == nullptr) {
                    arc = diGraph->findArc(mate[round][v], v);
                }
                succ = swap_subroutine(arc, round);
                if (succ) {
                    // only vertices affected by swap need to be checked again
                    // for possible improvements
                    changed.push_back(arc->getFirst());
                    changed.push_back(arc->getSecond());
                    changed.push_back(mate[round][arc->getFirst()]);
                    changed.push_back(mate[round][arc->getSecond()]);
                }
            }
        }
        current_vertices = changed;
        changed.clear();
    }
    return succ;
}

void gpa::extract_paths_apply_matching(path_set & pathset, unsigned round) {
    EdgeWeight first_rating, second_rating;
    diGraph->mapVertices([&] (Vertex * v) {
        const path & p = pathset.get_path(v);

        if (!p.is_active() || p.get_tail() != v || p.get_length() == 0) {
            return;
        }

        if (p.get_head() == p.get_tail()) {
            // cycle
            std::vector<Arc*> first_matching, second_matching;
            std::deque<Arc*> unpacked_cycle;
            unpack_path(p, pathset, unpacked_cycle);

            // first split cycle by removing first edge
            auto first = unpacked_cycle.front();
            unpacked_cycle.pop_front();
            maximum_weight_matching(unpacked_cycle, first_matching, first_rating);

            // then by removing the last edge
            unpacked_cycle.push_front(first);
            auto last = unpacked_cycle.back();
            unpacked_cycle.pop_back();
            maximum_weight_matching(unpacked_cycle, second_matching, second_rating);

            unpacked_cycle.push_back(last);

            if (first_rating > second_rating) {
                apply_matching(first_matching, round);
            } else {
                apply_matching(second_matching, round);
            }
        } else {
            // a path, not a cycle
            if (p.get_length() == 1) {
                // match single edge directly
                Arc * arc;
                if (pathset.next_vertex(p.get_tail()) == p.get_head()) {
                    arc = pathset.edge_to_next(p.get_tail());
                } else {
                    arc = pathset.edge_to_prev(p.get_tail());
                }
                edge_color[arc] = round;
                auto s = arc->getFirst();
                auto t = arc->getSecond();
                mate[round][s] = t;
                mate[round][t] = s;
                total_weight += (*weights)[arc];
                matched_in_round.push_back(arc);
                return;
            }
            std::vector<Arc*> matching;
            std::deque<Arc*> unpacked_path;
            unpack_path(p, pathset, unpacked_path);

            EdgeWeight rating = 0;
            maximum_weight_matching(unpacked_path, matching, rating);
            apply_matching(matching, round);
            // total_weight += rating;
        }
    });
}

// todo: computation of ratings seems to contain some bug
// if ratings where used to accumulate total_weight, the resulting
// weight is a little too small
template <typename VectorOrDeque>
void gpa::maximum_weight_matching(VectorOrDeque & unpacked_path, std::vector<Algora::Arc*> & matched_edges, EdgeWeight & final_rating) {
    auto k = unpacked_path.size();
    final_rating = 0;
    if (k == 1) {
        matched_edges.push_back(unpacked_path[0]);
        return;
    }

    std::vector<EdgeWeight> ratings(k, 0);
    std::vector<bool> decision(k, false);
    decision[0] = true;
    ratings[0] = (*weights)[unpacked_path[0]];
    ratings[1] = (*weights)[unpacked_path[1]];

    if (ratings[0] < ratings[1]) {
        decision[1] = true;
    }
    // dynamic programing
    for (auto i = 2u; i < k; i++) {
        const auto arc = unpacked_path[i];
        EdgeWeight weight = (*weights)[arc];
        if (weight+ratings[i-2] > ratings[i-1]) {
            decision[i] = true;
            ratings[i] = weight+ratings[i-2];
        } else {
            decision[i] = false;
            ratings[i] = ratings[i-1];
        }
    }

    if (decision[k-1]) {
        final_rating = ratings[k-1];
    } else {
        final_rating = ratings[k-2];
    }
    // construct solution
    for (int i = k-1; i >= 0;) {
        if (decision[i]) {
            matched_edges.push_back(unpacked_path[i]);
            i -= 2;
        } else {
            i -= 1;
        }
    }
}

inline void gpa::apply_matching(std::vector<Arc*> & matched, unsigned round) {
    for (const auto & arc : matched) {
        edge_color[arc] = round;
        auto s = arc->getFirst();
        auto t = arc->getSecond();
        mate[round][s] = t;
        mate[round][t] = s;
        matched_in_round.push_back(arc);
        total_weight += weights->getValue(arc);
    }
}

template <typename VectorOrDeque>
void gpa::unpack_path(const path & p, path_set & pathset, VectorOrDeque & unpacked_path) {
    auto head = p.get_head();
    auto prev = p.get_tail();
    Vertex * next;
    auto current = prev;

    if (prev == head) {
        // path is a cycle
        current = pathset.next_vertex(prev);
        unpacked_path.push_back(pathset.edge_to_prev(current));
    }

    while (current != head) {
        if (pathset.next_vertex(current) == prev) {
            next =  pathset.prev_vertex(current);
            unpacked_path.push_back(pathset.edge_to_prev(current));
        } else {
            next =  pathset.next_vertex(current);
            unpacked_path.push_back(pathset.edge_to_next(current));
        }
        prev = current;
        current = next;
    }
}
