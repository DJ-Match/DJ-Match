/**
 * Copyright (C) 2021, 2022 : Kathrin Hanauer, Jonathan Trummer
 *
 * This file is part of DJMatch and licensed under GPLv3.
 */

#pragma once
#include "matching_algorithm.h"

class greedy_b_matching : public matching_algorithm {

    public:
        greedy_b_matching(MatchingConfig &config, bool extend = false, bool global_swaps = false)
            : matching_algorithm(config), do_extend(extend), do_global_swaps(global_swaps) {}

        virtual void run() override final;

        void postprocess();

        virtual std::string getName() const noexcept override {
            return do_extend
                ? (do_global_swaps
                  ? "bGreedy&Extend + global swaps"
                  : "bGreedy&Extend")
                : (do_global_swaps
                  ? "bGreedy&Color + global swaps"
                  : "bGreedy&Color");
        }

        virtual std::string getShortName() const noexcept override {
            return do_extend
                ? (do_global_swaps
                  ? "bgreedy_extend-swaps-global"
                  : "bgreedy_extend")
                : (do_global_swaps
                  ? "bgreedy_color-swaps-global"
                  : "bgreedy_color");
        }

    private:
        bool do_extend { false };
        bool do_global_swaps { false };

        void greedy_extend(std::vector<Algora::Arc*> &);
};
