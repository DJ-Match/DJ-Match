/**
 * Copyright (C) 2021, 2022 : Kathrin Hanauer, Jonathan Trummer
 *
 * This file is part of DJMatch and licensed under GPLv3.
 */

#pragma once
#include "matching_algorithm.h"
#include <algorithm>
#include <numeric>
#include "property/fastpropertymap.h"
#include "graph/arc.h"
#include <sstream>


const std::string aggregate_names[] = {"SUM", "MAX", "AVG", "MEDIAN", "B_SUM"};
class node_centered : public matching_algorithm {

	public:
		node_centered(MatchingConfig &config, AggregateType type, double thresh=-1.0)
            : matching_algorithm(config), type(type), threshold(thresh) {}

		virtual void run() override final;

		virtual std::string getName() const noexcept override {
            std::ostringstream out;
            out << "node_centered-" << aggregate_names[type];
            if (threshold > 0) {
                out << " + threshold " << threshold;
            }
            return out.str();
		}

		virtual std::string getShortName() const noexcept override {
            std::ostringstream out;
            out << "NC-" << aggregate_names[type];
            if (threshold > 0) {
                out << "+t" << threshold;
            }
            return out.str();
		}

        virtual double getThreshold() const noexcept override {
            return threshold;
        }

	private:
		AggregateType type;
		double threshold {-1.0};

		EdgeWeight aggregate(std::vector<EdgeWeight> &weights) {
			const unsigned b = num_matchings;
            const auto size = weights.size();
            assert(size > 0);
            if (size == 1) {
                return weights[0];
            }
            switch(type) {
                case AggregateType::AVG:
                    return (std::accumulate(weights.begin(), weights.end(), 0) / size);
                case AggregateType::MEDIAN: {
                    std::nth_element(weights.begin(), weights.begin()+size/2, weights.end(), std::greater<EdgeWeight>());
                    return size % 2 != 0
                        ?  weights[size/2]
                        : (weights[size/2] + *std::min_element(weights.begin(), weights.begin()+size/2)) / 2UL;
                }
                case AggregateType::MAX:
                    return *std::max_element(weights.begin(), weights.end());
                case AggregateType::B_SUM:
                    if (b < size) {
                        std::nth_element(weights.begin(), weights.begin() + b, weights.end(), std::greater<EdgeWeight>());
                        return std::accumulate(weights.begin(), weights.begin()+ b, 0);
                    }
                    return std::accumulate(weights.begin(), weights.end(), 0UL);
                case AggregateType::SUM:
                default:
                    return std::accumulate(weights.begin(), weights.end(), 0UL);
            }
        }

		unsigned long aggregateWeights(std::vector<Algora::Arc*> &edges, const Algora::ModifiableProperty<EdgeWeight> &weight) {
			const unsigned b = num_matchings;
            const auto size = edges.size();
            assert(size > 0);
            if (size == 1) {
                return weight(edges[0]);
            }
            auto weightSum = [&weight](EdgeWeight acc, const Algora::Arc *a) { return std::move(acc) + weight(a); };
            switch(type) {
                case AggregateType::AVG:
                    return (std::accumulate(edges.begin(), edges.end(), weight(edges[0]), weightSum) / size);
                case AggregateType::MEDIAN: {
                    return size % 2 != 0
                        ?  weight(edges[size/2])
                        : (weight(edges[size/2]) + weight(edges[size/2-1])) / 2UL;
                }
                case AggregateType::MAX:
                    return weight(edges.front());
                case AggregateType::B_SUM:
                    if (b < size) {
                        return std::accumulate(edges.begin(), edges.begin() + b, weight(edges[0]), weightSum);
                    }
                    return std::accumulate(edges.begin(), edges.end(), weight(edges[0]), weightSum);
                case AggregateType::SUM:
                default:
                    return std::accumulate(edges.begin(), edges.end(), weight(edges[0]), weightSum);
            }
        }

        unsigned first_free_matching_color(const std::vector<char> & lop, const std::vector<char> & rop) {
            return std::mismatch(
                    lop.begin(), lop.end(),
                    rop.begin(), rop.end(),
                    [](const char &l, const char &r){ return !(l && r);}).first - lop.begin();
        }

        unsigned first_free(const std::vector<char> & lop) {
            return std::find(lop.begin(), lop.end(), true) - lop.begin();
        }
};
