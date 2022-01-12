/**
 * Copyright (C) 2021, 2022 : Kathrin Hanauer, Jonathan Trummer
 *
 * This file is part of DJMatch and licensed under GPLv3.
 */

#pragma once
#include <string>
#include <iostream>
#include <limits>
#include "matching_config.h"
#include "matching_defs.h"
#include "algorithm/dynamicweighteddigraphalgorithm.h"

#include "graph/digraph.h"
#include "property/fastpropertymap.h"

#define SHADOW

#ifdef SHADOW
#include "graph.incidencelist/incidencelistgraph.h"
#endif


using namespace Algora;

class matching_algorithm :
    public DynamicWeightedDiGraphAlgorithm<EdgeWeight>
{
    using Super = DynamicWeightedDiGraphAlgorithm<EdgeWeight>;

    public:
        using color_t = unsigned int;
        matching_algorithm(MatchingConfig & config,
                const DiGraph::size_type &maxVertexId = 0)
          : Super(),
            config(config),
            num_matchings(0U),
            max_vertex_id(maxVertexId)
        {
            edge_color.setName("edge_color_map");
        }
        virtual ~matching_algorithm() = default;

        void set_num_matchings(unsigned int b) {
            num_matchings = b;
            reset();
        }

        void set_max_vertex_id(const DiGraph::size_type &maxVertexId) {
            max_vertex_id = maxVertexId;
        }

        virtual EdgeWeight deliver() {
            return total_weight;
        }

        virtual double getThreshold() const noexcept {
            return -1.0;
        }

        Algora::FastPropertyMap<color_t> getEdgeColors() {
            return edge_color;
        }

        std::vector<Algora::FastPropertyMap<Algora::Vertex*>> getMates() {
            return mate;
        }

        void swapData(Algora::FastPropertyMap<color_t> &edge_colors,
                std::vector<Algora::FastPropertyMap<Algora::Vertex*>> &mates) {
            std::swap(edge_colors, this->edge_color);
            std::swap(mates, this->mate);
        }

    protected:
        MatchingConfig &config;
        color_t num_matchings {0};
        const color_t UNCOLORED { std::numeric_limits<color_t>::max() };
        DiGraph::size_type max_vertex_id;
        Algora::FastPropertyMap<color_t> edge_color;
        std::vector<Algora::FastPropertyMap<Algora::Vertex*>> mate;
        std::vector<Algora::FastPropertyMap<Algora::Arc*>> arcToMate;
        EdgeWeight total_weight{0ul};

#ifdef SHADOW
        FastPropertyMap<EdgeWeight> *weights;
        IncidenceListGraph *diGraph;

        virtual void onWeightsSet() override {
            Super::onWeightsSet();
            weights = dynamic_cast<FastPropertyMap<EdgeWeight>*>(Super::weights);
        }
#endif

        virtual void onDiGraphSet() override {
            Super::onDiGraphSet();
#ifdef SHADOW
            diGraph = dynamic_cast<IncidenceListGraph*>(Super::diGraph);
#endif
            if (max_vertex_id == 0) {
                max_vertex_id = diGraph->getSize();
            }
            reset();
        }

        void reset() {
            if (num_matchings == 0) {
                return;
            }
            edge_color.setDefaultValue(UNCOLORED);
            edge_color.resetAll(diGraph->getNumArcs(false));
            mate.resize(num_matchings, {nullptr});
            arcToMate.resize(num_matchings, {nullptr});
            for (auto bi = 0u; bi < num_matchings; bi++) {
                mate[bi].resetAll(max_vertex_id);
                arcToMate[bi].resetAll(max_vertex_id);
            }
            total_weight = 0;
        }

        void set_edge_color(Arc *a, const color_t &c) {
            assert(c < mate.size());

            edge_color[a] = c;
            mate[c][a->getTail()] = a->getHead();
            mate[c][a->getHead()] = a->getTail();
            arcToMate[c][a->getTail()] = a;
            arcToMate[c][a->getHead()] = a;
        }

        void unset_edge_color(Arc *a) {
            auto c = edge_color(a);
            if (c != UNCOLORED) {
                assert(c < mate.size());
                edge_color.resetToDefault(a);
                if (mate[c][a->getTail()] == a->getHead()) {
                    mate[c][a->getTail()] = nullptr;
                }
                if (mate[c][a->getHead()] == a->getTail()) {
                    mate[c][a->getHead()] = nullptr;
                }
                if (arcToMate[c][a->getTail()] == a) {
                    arcToMate[c][a->getTail()] = nullptr;
                }
                if (arcToMate[c][a->getHead()] == a) {
                    arcToMate[c][a->getHead()] = nullptr;
                }
            }
        }


        color_t matching_free_color(const Algora::Vertex *lop, const Algora::Vertex *rop) {
            color_t i = 0;
            for (; i < mate.size(); i++) {
                // they're both their own mate in any color => both free there
                if (mate[i][lop] == nullptr && mate[i][rop] == nullptr) {
                    break;
                }
            }
            return i;
        }


        void sanityCheck() {
            using namespace Algora;
            FastPropertyMap<unsigned> num_matchings_per_node(0, "", max_vertex_id);
            FastPropertyMap<std::vector<unsigned>> colors_per_node(std::vector<unsigned>(num_matchings, 0), "", max_vertex_id);
            FastPropertyMap<unsigned> arc_matchings(0);
            EdgeWeight weight_check = 0;
            diGraph->mapArcs([&] (Arc * arc) {
                if (edge_color[arc] < num_matchings) {
                    auto s = arc->getFirst();
                    auto t = arc->getSecond();
                    num_matchings_per_node[s]++;
                    colors_per_node[s][edge_color[arc]]++;
                    num_matchings_per_node[t]++;
                    colors_per_node[t][edge_color[arc]]++;

                    weight_check += weights->getValue(arc);
                }
            });

            for (auto bi = 0u; bi < num_matchings; bi++) {
                diGraph->mapVertices([&] (Vertex * v) {
                    // v's mate doesn't have v as mate
                    if (mate[bi][v] && mate[bi][mate[bi][v]] != v) {
                        std::cout << "Error! In matching #" << bi << " mate of " << v << " is " << mate[bi][v] << ", but mate of " << mate[bi][v] << " is " << mate[bi][mate[bi][v]]  << "\n";
                    }

                    if (mate[bi][v] == nullptr) {
                        return;
                    }

                    // to only count for one mate pair once,
                    // try to find the edge starting from v
                    auto arc = diGraph->findArc(v, mate[bi][v]);
                    if (arc != nullptr) {
                        arc_matchings[arc]++;
                    }
                });
            }

            diGraph->mapArcs([&] (Arc * arc) {
                if (arc_matchings[arc] > 2
                    || (arc_matchings[arc] == 2 && !diGraph->findArc(arc->getHead(), arc->getTail()))) {
                    std::cout << "ERROR: arc " << arc << " matched "
                        << arc_matchings[arc] << " times\n";
                }
            });

            diGraph->mapVertices([&] (Vertex * v) {
                if (num_matchings_per_node[v] > num_matchings) {
                    std::cout << "ERROR: Node " << v << " matched " << num_matchings_per_node[v] << " times!\n";
                }
                for (auto c = 0u; c < num_matchings; c++) {
                    if (colors_per_node[v][c] > 1) {
                        std::cout << "ERROR: Color " << c << " adjacent to node " << v << " " << colors_per_node[v][c] << " times!\n";
                    }
                }
            });
            if (weight_check != total_weight) {
                std::cout << "Warning: weight check differs from weight computed by algorithm: weight check = " << weight_check << "\n";
            }
        }

        // swapping subroutine, can be used for local or for global swaps
        // this routine simply tries to swap one single arc out and replace
        // it by two arcs such that their combined weight is larger
        bool swap_subroutine(const Algora::Arc * arc, const unsigned round) {
            using namespace Algora;
            const auto s = arc->getFirst();
            const auto t = arc->getSecond();
            EdgeWeight lop_weight = 0;
            EdgeWeight rop_weight = 0;
            Arc *lop, *rop;
            Vertex* lop_target;
            auto outgoing = true;
            auto is_lop = true;

            // one lambda for both s and t searches
            // from incoming and outgoing arcs
            auto vm = [&] (Arc* candidate) {
                auto t2 = outgoing
                    ? candidate->getHead()
                    : candidate->getTail();
                // uncolored edge and endpoint free in this round
                if (edge_color[candidate] == UNCOLORED && mate[round][t2] == nullptr) {
                    if (is_lop && (*weights)[candidate] > lop_weight) {
                        lop = candidate;
                        lop_weight = (*weights)[candidate];
                        lop_target = t2;
                    } else if (!is_lop && (*weights)[candidate] > rop_weight) {
                        // prevent triangle matching, ie unmatching
                        // one edge of triangle for the other two edges of the triangle
                        // => endpoints of lop and rop cant overlap
                        if (lop_weight > 0 && t2 == lop_target) {
                            return;
                        }
                        rop = candidate;
                        rop_weight = (*weights)[candidate];
                    }
                }
            };

            diGraph->mapOutgoingArcs(s,vm);
            outgoing = false;
            diGraph->mapIncomingArcs(s,vm);
            is_lop = false;
            outgoing = true;
            diGraph->mapOutgoingArcs(t,vm);
            outgoing = false;
            diGraph->mapIncomingArcs(t,vm);

            if (lop_weight > 0 && rop_weight > 0 && (lop_weight+rop_weight) > (*weights)[arc]) {
                // replace arc with lop and rop
                const auto s1 = lop->getFirst();
                const auto t1 = lop->getSecond();
                const auto s2 = rop->getFirst();
                const auto t2 = rop->getSecond();
                assert(s1 != s2);
                assert(s1 != t2);
                assert(t1 != s2);
                assert(t1 != t2);

                mate[round][s1] = t1;
                mate[round][t1] = s1;
                mate[round][s2] = t2;
                mate[round][t2] = s2;
                assert(edge_color[lop] == UNCOLORED);
                assert(edge_color[rop] == UNCOLORED);
                edge_color[lop] = round;
                edge_color[rop] = round;
                edge_color[arc] = UNCOLORED;

                total_weight = total_weight - (*weights)[arc] + (*weights)[lop] + (*weights)[rop];
                return true;
            }
            return false;
        }

        void global_swaps() {
        	using namespace Algora;
        	std::vector<Arc*> edges;
        
        	diGraph->mapArcs([this,&edges] (Arc * arc) {
        		if (edge_color[arc] < num_matchings) {
        			edges.push_back(arc);
        		}
        	});
        
            if (config.swaps_reverse_sort) {
                std::sort(edges.begin(), edges.end(), [this] (const Arc * lop, const Arc * rop) {
                        return weights->getValue(lop) < weights->getValue(rop);
                        });
            } else {
                std::sort(edges.begin(), edges.end(), [this] (const Arc * lop, const Arc * rop) {
                        return (*weights)[lop] > (*weights)[rop];
                        });
            }
        
        	for (const auto & arc : edges) {
        		auto round = matching_free_color(arc->getFirst(), arc->getSecond());
        		if (round < num_matchings) {
                        swap_subroutine(arc, round);
        		}
        	}
        }

};
