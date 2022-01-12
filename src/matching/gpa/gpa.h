/**
 * Copyright (C) 2021, 2022 : Kathrin Hanauer, Jonathan Trummer
 *
 * This file is part of DJMatch and licensed under GPLv3.
 */

#pragma once
#include <random>

#include <sstream>

#include "matching/matching_algorithm.h"
#include "matching/matching_config.h"
#include "matching/matching_defs.h"

#include "matching/gpa/path.h"
#include "matching/gpa/path_set.h"

class gpa: public matching_algorithm {
    public:
        gpa(MatchingConfig &config, bool swaps, bool do_global_swaps, int num_roma)
            : matching_algorithm(config), swaps(swaps), do_global_swaps(do_global_swaps), num_roma(num_roma)
        {}

        virtual void run() override;

        virtual std::string getName() const noexcept override {
            std::ostringstream out;
            out << "GPA";
            if (num_roma > 0) {
                out << " + ROMA " << num_roma;
            } else if (swaps) {
                if (do_global_swaps) {
                    out << " + global swaps";
                } else {
                    out << " + local swaps";
                }
            }
            return out.str();
		}

        virtual std::string getShortName() const noexcept override {
            std::ostringstream out;
            out << "gpa";
            if (num_roma > 0) {
                out << "_roma-" << num_roma;
            } else if (swaps) {
                if (do_global_swaps) {
                    out << "_swaps-global";
                } else {
                    out << "_swaps-local";
                }
            }
            return out.str();
        }

    private:
        bool swaps;
		bool do_global_swaps;
        int num_roma;

        std::mt19937 rng;

        std::vector<Algora::Vertex*> all_vertices;
        std::vector<Algora::Arc*> matched_in_round;


        bool local_swaps(const unsigned int round);
        bool roma(const unsigned int round);

        void extract_paths_apply_matching(path_set & pathset, unsigned round); 

        template <typename VectorOrDeque> 
        void unpack_path(const path & the_path, path_set & pathset, VectorOrDeque & a_path);

        template <typename VectorOrDeque> 
        void maximum_weight_matching(VectorOrDeque & unpacked_path, std::vector<Algora::Arc*> & matched_edges, EdgeWeight & final_rating);

        void apply_matching(std::vector<Arc*> & matched, unsigned round);


        template <typename VectorOrDeque> 
        void dump_unpacked_path(VectorOrDeque & unpacked_path);
};
