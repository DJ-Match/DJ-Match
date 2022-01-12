/**
 * Copyright (C) 2021, 2022 : Kathrin Hanauer, Jonathan Trummer
 *
 * This file is part of DJMatch and licensed under GPLv3.
 */

#pragma once
#include "matching_algorithm.h"

class greedy_iterative : public matching_algorithm {

	public:
		greedy_iterative(MatchingConfig &config, bool swaps, bool do_global_swaps = false)
            : matching_algorithm(config), swaps(swaps), do_global_swaps(do_global_swaps) {}
        virtual ~greedy_iterative() = default;

		void run() override final;

		std::string getName() const noexcept override {
			std::string app = "";
			if (swaps) {
				app += "-swaps";
				if (do_global_swaps) {
					app+= "-global";
				} else {
					app+= "-local";
				}
			}
			return "greedy_iterative" + app;
		}

		std::string getShortName() const noexcept override {
            return getName();
		}

	private:
		bool swaps;
		bool do_global_swaps;
		std::vector<Algora::Arc*> matched_in_round;

		bool local_swaps(const unsigned int round);
};
