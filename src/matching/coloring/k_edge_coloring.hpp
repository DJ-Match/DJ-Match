//=======================================================================
// Copyright 2013 Maciej Piechotka, 2021 Kathrin Hanauer
// Original Authors: Maciej Piechotka
// Adapted by: Kathrin Hanauer <kathrin.hanauer@univie.ac.at>
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
//=======================================================================
#ifndef KEDGECOLORING_H
#define KEDGECOLORING_H

#include <algorithm>
#include <limits>
#include <vector>

#include "matching/matching_algorithm.h"

/* This algorithm is to find coloring of an edges

   Reference:

   Misra, J., & Gries, D. (1992). A constructive proof of Vizing's
   theorem. In Information Processing Letters.
*/

using namespace Algora;

class kEdgeColoring : public matching_algorithm
{
public:
    kEdgeColoring(MatchingConfig & config, bool common_color = true, bool lightest_color = false, bool max_rotate = false)
            : matching_algorithm(config), find_common_color(common_color), use_lightest_color(lightest_color), rotate_long(max_rotate) { }

     virtual bool prepare() override {
        if (!matching_algorithm::prepare()) {
            return false;
        }
        if (diGraph == nullptr) {
            return false;
        }
        reset();
        matched.resetAll(diGraph->getSize());
        if (use_lightest_color) {
            color_weights.resize(num_matchings, 0);
            colors_by_weight.reserve(num_matchings);
            colors_by_weight.clear();
            for (auto i = 0U; i < num_matchings; i++) {
                colors_by_weight.push_back(i);
            }
        }
        return true;
     }

     virtual void run() {
         num_colors = edge_coloring();
     }

	virtual std::string getName() const noexcept override {
        return rotate_long
            ?  (find_common_color
                ? (use_lightest_color ? "k-Edge Coloring (CC, LC, RL)"
                                      : "k-Edge Coloring (CC, RL)")
                : (use_lightest_color ? "k-Edge Coloring (LC, RL)"
                                      : "k-Edge Coloring (RL)"))
            : (find_common_color
                ? (use_lightest_color ? "k-Edge Coloring (CC, LC)"
                                      : "k-Edge Coloring (CC)")
                : (use_lightest_color ? "k-Edge Coloring (LC)"
                                      : "k-Edge Coloring"));
    }

	virtual std::string getShortName() const noexcept override {
        return rotate_long
            ? (find_common_color
                ? (use_lightest_color ? "k-EC+CC-LC-RL"
                                      : "k-EC+CC-RL")
                : (use_lightest_color ? "k-EC-LC-RL"
                                      : "k-EC-RL"))
            : (find_common_color
                ? (use_lightest_color ? "k-EC+CC-LC"
                                      : "k-EC+CC")
                : (use_lightest_color ? "k-EC-LC"
                                      : "k-EC"));
    }

    color_t getNumColors() const {
        return num_colors;
    }

private:
    color_t num_colors { 0 };
    FastPropertyMap<color_t> matched { 0 };

    std::vector<EdgeWeight> color_weights;
    std::vector<color_t> colors_by_weight;

    bool find_common_color { true };
    bool edge_centered { true };
    bool use_lightest_color { false };
    bool rotate_long { false };


    bool is_free_color(Vertex *u, const color_t &color) {
        if (color == UNCOLORED) {
            return false;
        }
        assert(color < arcToMate.size());

        return arcToMate[color][u] == nullptr;
    }

    std::vector<Arc*> maximal_fan(Vertex *x, Arc *xy)
    {
        std::vector<Arc*> fan;
        fan.push_back(xy);

        bool extended;
        auto extend = [&](Arc *a) {
                if (is_free_color(fan.back()->getOther(x), edge_color(a))
                    && std::find(fan.begin(), fan.end(), a) == fan.end()) {
                    fan.push_back(a);
                    extended = true;
                }
            };
        do {
            extended = false;
            diGraph->mapOutgoingArcs(x, extend);
            diGraph->mapIncomingArcs(x, extend);
        } while (extended);
        return fan;
    }

    std::vector<Arc*> large_fan(Vertex *x, Arc *xy)
    {
        std::vector<Arc*> fan;
        fan.push_back(xy);

        // collect colored arcs with >= 1 free color on other side -> deg(x)
        std::vector<Arc*> coloredArcsWithFreeColor;
        coloredArcsWithFreeColor.reserve(matched[x]);
        std::vector<Arc*> nonFreeArc(num_matchings, nullptr);

        diGraph->mapIncidentArcs(x, [&](Arc *a) {
            if (edge_color(a) != UNCOLORED) {
                if (matched[a->getOther(x)] < num_matchings) {
                    coloredArcsWithFreeColor.push_back(a);
                } else if (nonFreeArc[edge_color(a)] == nullptr) {
                    nonFreeArc[edge_color(a)]  = a;
                }
            }
        });

        while (!nonFreeArc.empty() && nonFreeArc.back() == nullptr) {
            nonFreeArc.pop_back();
        }
        {
        auto i = 0U;
        while (i < nonFreeArc.size()) {
            if (nonFreeArc[i] == nullptr) {
                if (i + 1 < nonFreeArc.size()) {
                    nonFreeArc[i] = nonFreeArc.back();
                }
                nonFreeArc.pop_back();
            } else {
                i++;
            }
        }
        }

        // build fan...
        bool extended;
        do {
            extended = false;
            auto i = 0U;
            while (i < coloredArcsWithFreeColor.size()) {
                auto &a = coloredArcsWithFreeColor[i];
                if (is_free_color(fan.back()->getOther(x), edge_color(a))) {
                    fan.push_back(a);
                    if (i + 1 < coloredArcsWithFreeColor.size()) {
                        coloredArcsWithFreeColor[i] = coloredArcsWithFreeColor.back();
                    } else {
                        i++;
                    }
                    coloredArcsWithFreeColor.pop_back();
                    extended = true;
                } else {
                    i++;
                }
            }
        } while (extended);

        auto lastNeighbor = fan.back()->getOther(x);
        for (auto &a : nonFreeArc) {
            if (is_free_color(lastNeighbor, edge_color(a))) {
                fan.push_back(a);
                break;
            }
        }
        return fan;
    }

    std::vector<Arc*> quicker_fan(Vertex *x, Arc *xy)
    {
        std::vector<Arc*> fan;
        fan.push_back(xy);

        // collect colored arcs with >= 1 free color on other side -> deg(x)
        std::vector<Arc*> coloredArcs;
        coloredArcs.reserve(matched[x]);

        diGraph->mapIncidentArcs(x, [&](Arc *a) {
            if (edge_color(a) != UNCOLORED) {
                coloredArcs.push_back(a);
            }
        });

        // build fan...
        bool extended;
        std::vector<Arc*> coloredArcsOther;
        coloredArcsOther.reserve(coloredArcs.size());
        do {
            extended = false;
            auto i = 0U;
            coloredArcsOther.clear();
            while (i < coloredArcs.size()) {
                auto &a = coloredArcs[i];
                if (is_free_color(fan.back()->getOther(x), edge_color(a))) {
                    fan.push_back(a);
                    if (matched[a->getOther(x)] == num_matchings) {
                        extended = false;
                        break;
                    } else {
                        i++;
                        extended = true;
                    }
                } else {
                    coloredArcsOther.push_back(a);
                    i++;
                }
            }
            if (extended) {
                std::swap(coloredArcs, coloredArcsOther);
            }
        } while (extended);

        return fan;
    }

    color_t find_free_color(Vertex *u)
    {
        if (use_lightest_color) {
            for (auto &c : colors_by_weight) {
                if (is_free_color(u, c)) {
                    return c;
                }
            }
            return UNCOLORED;
        } else {
            color_t c = 0;
            while (c < num_matchings && !is_free_color(u, c))
                c++;
            return c < num_matchings ? c : UNCOLORED;
        }
    }

    void invert_cd_path(Vertex *x,
        Arc *eold,
        const color_t &c,
        const color_t &d)
    {
        unset_edge_color(eold);
        set_edge_color(eold, d);
        bool stop = false;
        diGraph->mapOutgoingArcsUntil(x, [&](Arc *a) {
            if (edge_color(a) == d && a != eold) {
                invert_cd_path(a->getHead(), a, d, c);
                stop = true;
            }
        }, [&stop](const Arc*) { return stop; });
        if (stop) {
            return;
        }
        diGraph->mapIncomingArcsUntil(x, [&](Arc *a) {
            if (edge_color(a) == d && a != eold) {
                invert_cd_path(a->getTail(), a, d, c);
                stop = true;
            }
        }, [&stop](const Arc*) { return stop; });
    }

    void invert_cd_path(
        Vertex *x,
        const color_t &c,
        const color_t &d)
    {
        bool stop = false;
        diGraph->mapOutgoingArcsUntil(x, [&](Arc *a) {
            if (edge_color(a) == d) {
                invert_cd_path(a->getHead(), a, d, c);
                stop = true;
            }
        }, [&stop](const Arc*) { return stop; });
        if (stop) {
            return;
        }
        diGraph->mapIncomingArcsUntil(x, [&](Arc *a) {
            if (edge_color(a) == d) {
                invert_cd_path(a->getTail(), a, d, c);
                stop = true;
            }
        }, [&stop](const Arc*) { return stop; });
    }

    void invert_cd_path_it(Vertex *x, const color_t &c, const color_t &d)
    {
        auto arcToRecolor = arcToMate[d][x];
        auto nextColor = c;
        auto nextArc = arcToRecolor;

        while (arcToRecolor) {
            x = arcToRecolor->getOther(x);
            nextArc = arcToMate[nextColor][x];

            unset_edge_color(arcToRecolor);
            set_edge_color(arcToRecolor, nextColor);

            arcToRecolor = nextArc;
            nextColor = nextColor == c ? d : c;
        }
    }

    void rotate_fan(
        std::vector<Arc*>::iterator begin,
        std::vector<Arc*>::iterator end) {
        if (begin == end)
        {
            return;
        }
        auto previous = *begin;
        for (begin++; begin != end; begin++)
        {
            auto c = edge_color(*begin);
            unset_edge_color(*begin);
            set_edge_color(previous, c);
            previous = *begin;
        }
    }

    color_t color_edge(Arc *xy, Vertex *x) {
        if (find_common_color) {
            auto c_common = matching_free_color(xy->getTail(), xy->getHead());
            if (c_common < num_matchings) {
                set_edge_color(xy, c_common);
                return c_common;
            }
        }

        color_t c = find_free_color(x);
        if (c >= num_matchings) {
            return UNCOLORED;
        }

        //auto fan = maximal_fan(x, xy);
        //auto fan = small_fan(x, xy);
        auto fan = quicker_fan(x, xy);
        assert(fan.size() > 0);

        color_t d = find_free_color(fan.back()->getOther(x));

        if (d >= num_matchings) {
            return UNCOLORED - 1;
        }
        assert(!fan.empty());

        if (!rotate_long || !is_free_color(x, d)) {
            if (c != d) {
            //invert_cd_path(x, c, d);
                invert_cd_path_it(x, c, d);
            }

            auto w = std::find_if(fan.begin(), fan.end(),
                    [&](Arc *a) { return is_free_color(a->getOther(x), d); });
            assert(w != fan.end());
            rotate_fan(fan.begin(), w + 1);
            set_edge_color(*w, d);
        } else {
            rotate_fan(fan.begin(), fan.end());
            set_edge_color(fan.back(), d);
        }
        return (std::max)(c, d);
    }

    color_t edge_coloring()
    {
        color_t colors = 0;

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

            for (auto &a : edges) {
                if (matched[a->getTail()] < num_matchings
                        && matched[a->getHead()] < num_matchings) {
                    auto c = color_edge(a, a->getTail());
                    if (c == UNCOLORED - 1) {
                        assert(edge_color(a) == UNCOLORED);
                        c = color_edge(a, a->getHead());
                    }
                    if (c < num_matchings) {
                        colors = (std::max)(colors, c + 1);
                        total_weight += (*weights)(a);
                        matched[a->getTail()]++;
                        matched[a->getHead()]++;

                        if (use_lightest_color) {
                            color_weights[c] += (*weights)(a);
                            bool swapped = false;
                            for (auto i = 0U; i + 1 < num_matchings; i++) {
                                if (color_weights[colors_by_weight[i]] > color_weights[colors_by_weight[i+1]]) {
                                    auto tmp = colors_by_weight[i];
                                    colors_by_weight[i] = colors_by_weight[i+1];
                                    colors_by_weight[i+1] = tmp;
                                    swapped = true;
                                } else if (swapped) {
                                    break;
                                }
                            }
                        }
                    }
                }
            }
        return colors;
    }
};

#endif
