/**
 * Copyright (C) 2021, 2022 : Kathrin Hanauer, Jonathan Trummer
 *
 * This file is part of DJMatch and licensed under GPLv3.
 */

#pragma once
#include "../matching_algorithm.h"
#include "../greedy_b_matching.h"

#include <iostream>
#include <vector>

class MisraGries: public matching_algorithm {
    friend greedy_b_matching;
    public:
        MisraGries(MatchingConfig & config)
            : matching_algorithm(config), fix_delta(0) { }

        void setMaxDegree(unsigned int max_degree) {
            fix_delta = max_degree;
        }

        virtual bool prepare() override {
            if (diGraph == nullptr) {
                return false;
            }
            if (fix_delta > 0) {
                delta = fix_delta;
            } else {
                delta = 0;
                // find maximum degree
                diGraph->mapVertices([&] (Algora::Vertex * v) {
                    const auto d = diGraph->getDegree(v, true);
                    if (d > delta) {
                        delta = d;
                    }
                });
            }
            delta++;
            free_color.assign(delta, true);
            num_matchings = delta;
            max_color = 0;
            reset();
            return true;
        }

        virtual void run() override;

        virtual std::string getName() const noexcept override {
            return "misra-gries";
        }

        virtual std::string getShortName() const noexcept override {
            return "misra-gries";
        }

        void sanityCheck();

        unsigned int getNumColors() {
            return max_color + 1;
        }

protected:
        virtual void onDiGraphSet() override {
            matching_algorithm::onDiGraphSet();
            fix_delta = 0;
        }

        virtual void onDiGraphUnset() override {
            matching_algorithm::onDiGraphUnset();
        }

    private:
        unsigned int fix_delta { 0U };
        unsigned int delta { 0U };
        unsigned int max_color { 0U };

        // helper vector whilst building fans
        std::vector<bool> free_color;

        // tmp arrays used throughout
        std::vector<unsigned> touched_free_color;
        std::vector<unsigned> touched_locally_free_color;
        std::vector<Algora::Vertex*> touched_path;

        std::vector<Algora::Arc*> fan;
        Algora::FastPropertyMap<char> fan_marked;

        Algora::FastPropertyMap<char> visited_path;

        void maximal_fan(Algora::Arc * arc);

        void shrink_fan(std::vector<Algora::Vertex*> cdpath, unsigned int c);

        unsigned int getFirstFreeColor(std::vector<bool> & colors) {
            auto i{0ul};
            while (i < colors.size() && !colors[i]) {
                i++;
            }
            return i;
        }

        void invertCdPath(unsigned int c, unsigned int d, Algora::Vertex * start);

        void rotateFan();

};
