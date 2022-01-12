/**
 * Copyright (C) 2021, 2022 : Kathrin Hanauer, Jonathan Trummer
 *
 * This file is part of DJMatch and licensed under GPLv3.
 */

#include <iostream>
#include <string>
#include <ostream>
#include <fstream>
#include <sys/stat.h>
#include <vector>
#include <memory>
#include <ratio>
#include <iomanip>
#include <random>

#include "djmatch_info.h"
#include "tools/chronotimer.h"
#include "matching/matching_config.h"
#include "parse_parameters.h"
#include "matching/matching_algorithm.h"
#include "matching/greedy_b_matching.h"
#include "matching/greedy_iterative.h"
#include "matching/node_centered.h"
#include "matching/gpa/gpa.h"
#include "matching/coloring/k_edge_coloring.hpp"
#include "graph.dyn/dynamicweighteddigraph.h"
#include "io/konectnetworkreader.h"


std::ofstream outfile;

bool file_exists(const std::string &filename) {
    struct stat buffer;
    return (stat(filename.c_str(), &buffer) == 0);
}

void write_result(MatchingConfig & config, const matching_algorithm &algorithm, double time, unsigned long weight) {
    if (config.writeOutputfile) {
        outfile << config.graph_filename << "," << config.b << ","
            << config.seed << "," << config.l << ","
            << algorithm.getThreshold() << ","
            << algorithm.getShortName() << "," << time << "," << weight

            << "\n";
    }
}

int main(int argc, char **argv) {
    MatchingConfig config;
    std::string graph_filename;
    bool do_return;

    auto code = parse_matching_parameters(argc, argv, config, graph_filename, do_return);
    if (code > 0) {
        return code;
    } else if (do_return) {
        return 0;
    }

    std::ifstream graph_file;
    graph_file.open(graph_filename);

    if (!graph_file.is_open()) {
        std::cout << "Error! Could not open file " << graph_filename << "\n";
        return 1;
    }


    std::cout << "GIT_DATE: " << DJMatchInfo::GIT_DATE << "\n"
            << "GIT_REVISION: " << DJMatchInfo::GIT_REVISION << "\n"
            << "GIT_TIMESTAMP: " << DJMatchInfo::GIT_TIMESTAMP << "\n"
            ;

    std::cout << "called with params: \n";
    for (auto i = 1; i < argc; i++) {
        std::cout << argv[i] << "\n";
    }

    ChronoTimer t;

    Algora::DynamicWeightedDiGraph<unsigned long> G(0);
    Algora::KonectNetworkReader reader;
    reader.setInputStream(&graph_file);
    reader.provideDynamicWeightedDiGraph(&G);
    graph_file.close();
    std::cout << "Input I/O took " << t.elapsed() << "s\n";
    std::cout << "%n,m " << G.getConstructedGraphSize() << "," << G.getConstructedArcSize() << "\n";

    if (config.writeOutputfile) {
        bool nfile = !file_exists(config.outputFile);

        outfile.open(config.outputFile, std::ios::app | std::ios::out);
        if (nfile) {
            outfile << "graph,b,seed,l,threshold_global,algorithm,time,weight\n";
        }
    }

    if (config.all_bs.empty()) {
        config.all_bs.push_back(1);
    }

    std::vector<std::unique_ptr<matching_algorithm>> algos;

    if (config.greedy) {
        const bool swaps = true;
        algos.emplace_back(new greedy_b_matching(config));

        if (config.swaps_and_normal) {
            algos.emplace_back(new greedy_iterative(config, swaps, true));
            algos.emplace_back(new greedy_iterative(config, swaps, false));
            algos.emplace_back(new greedy_iterative(config, !swaps));
        } else {
            if (config.swaps) {
                algos.emplace_back(new greedy_iterative(config, config.swaps, true));
            }
            algos.emplace_back(new greedy_iterative(config, config.swaps));
        }
    }

    if (config.node_centered) {
        const bool threshold = true;
        // node centered with threshold active
        algos.emplace_back(new node_centered(config, AggregateType::MAX, threshold));
        algos.emplace_back(new node_centered(config, AggregateType::SUM, threshold));
        algos.emplace_back(new node_centered(config, AggregateType::B_SUM, threshold));

        // node centered without threshold
        algos.emplace_back(new node_centered(config, AggregateType::MAX, !threshold));
        algos.emplace_back(new node_centered(config, AggregateType::SUM, !threshold));
        algos.emplace_back(new node_centered(config, AggregateType::B_SUM, !threshold));
    }

    if (config.gpa) {
        if (config.swaps_and_normal) {
            algos.emplace_back(new gpa(config, false, false, false));
            algos.emplace_back(new gpa(config, true, false, false));
            algos.emplace_back(new gpa(config, false, true, false));
            algos.emplace_back(new gpa(config, false, false, true));
        } else {
            if (config.swaps) {
                algos.emplace_back(new gpa(config, config.swaps, false, config.roma));
                algos.emplace_back(new gpa(config, config.swaps, true, config.roma));
            }
            algos.emplace_back(new gpa(config, config.swaps, config.swaps_global, config.roma));
        }
    }

    if (!config.algorithms.empty()) {
        algos.clear();
        for (const auto &algorithm : config.algorithms) {
            switch(algorithm) {
                case MatchingAlgorithm::BGREEDY_COLOR:
                    if (config.swaps_and_normal) {
                        algos.emplace_back(new greedy_b_matching(config, false, false));
                        algos.emplace_back(new greedy_b_matching(config, false, true));
                    } else {
                        algos.emplace_back(new greedy_b_matching(config, false, config.swaps_global));
                    }
                    break;
                case MatchingAlgorithm::BGREEDY_EXTEND:
                    if (config.swaps_and_normal) {
                        algos.emplace_back(new greedy_b_matching(config, true, false));
                        algos.emplace_back(new greedy_b_matching(config, true, true));
                    } else {
                        algos.emplace_back(new greedy_b_matching(config, true, config.swaps_global));
                    }
                    break;
                case MatchingAlgorithm::GREEDY_IT:
                    if (config.swaps_and_normal) {
                        algos.emplace_back(new greedy_iterative(config, false, config.swaps_global));
                        if (config.swaps_global) {
                            algos.emplace_back(new greedy_iterative(config, true, false));
                        }
                        algos.emplace_back(new greedy_iterative(config, true, config.swaps_global));
                    } else {
                        algos.emplace_back(new greedy_iterative(config, config.swaps, config.swaps_global));
                    }
                    break;
                case MatchingAlgorithm::NODE_CENTERED:
                    if (config.aggregation_types.empty()) {
                        std::cerr << "Error: trying to run node-centered without any aggregation type set\n";
                        return 1;
                    }
                    for (const auto &atype : config.aggregation_types) {
                        if (config.global_thresholds.empty()) {
                            algos.emplace_back(new node_centered(config, atype));
                        } else {
                            for (const auto &thresh : config.global_thresholds) {
                                algos.emplace_back(new node_centered(config, atype, thresh));
                            }
                        }
                    }
                    break;
                case MatchingAlgorithm::GPA_IT:
                    if (config.swaps_and_normal) {
                        // normal
                        algos.emplace_back(new gpa(config, false, config.swaps_global, 0));
                        if (config.swaps_global) {
                            // if global, add local
                            algos.emplace_back(new gpa(config, true, false, 0));
                        }
                        // global if enabled, else local
                        algos.emplace_back(new gpa(config, true, config.swaps_global, 0));
                        if (config.roma) {
                            algos.emplace_back(new gpa(config, false, config.swaps_global, config.l));
                        }
                    } else if (config.swaps) {
                        algos.emplace_back(new gpa(config, config.swaps, config.swaps_global, 0));
                        if (config.roma) {
                            algos.emplace_back(new gpa(config, false, false, config.l));
                        }
                    } else {
                        algos.emplace_back(new gpa(config, config.swaps, config.swaps_global, config.l));
                    }
                    break;
                case MatchingAlgorithm::K_EC:
                    if (config.swaps_and_normal) {
                        // normal
                        algos.emplace_back(new kEdgeColoring(config, false, false, false));
                        algos.emplace_back(new kEdgeColoring(config, true, false, false));
                        algos.emplace_back(new kEdgeColoring(config, false, false, true));
                        algos.emplace_back(new kEdgeColoring(config, true, false, true));
                        // global swaps
                        algos.emplace_back(new kEdgeColoring(config, false, true, false));
                        algos.emplace_back(new kEdgeColoring(config, true, true, false));
                        algos.emplace_back(new kEdgeColoring(config, false, true, true));
                        algos.emplace_back(new kEdgeColoring(config, true, true, true));
                    } else if (config.swaps) {
                        algos.emplace_back(new kEdgeColoring(config, false, true, false));
                        algos.emplace_back(new kEdgeColoring(config, true, true, false));
                        algos.emplace_back(new kEdgeColoring(config, false, true, true));
                        algos.emplace_back(new kEdgeColoring(config, true, true, true));
                    } else {
                        algos.emplace_back(new kEdgeColoring(config, false, false, false));
                        algos.emplace_back(new kEdgeColoring(config, true, false, false));
                        algos.emplace_back(new kEdgeColoring(config, false, false, true));
                        algos.emplace_back(new kEdgeColoring(config, true, false, true));
                    }
                    break;
                default:
                    std::cerr << "Error: invalid algorithm passed " << algorithm << "\n";
                    return 1;
            }
        }
    }

    if (config.algorithm_order_seed != 0) {
        std::mt19937 rng;
        rng.seed(config.algorithm_order_seed);
        std::shuffle(algos.begin(), algos.end(), rng);
    }

    auto *diGraph = G.getDiGraph();
    auto *weights = G.getArcWeights();
    for (auto & algo : algos) {
        algo->set_max_vertex_id(G.getMaxVertexId());
        algo->setGraph(diGraph);
        algo->setWeights(weights);
    }

    for (auto b : config.all_bs) {
        config.b = b;
        std::cout << "Running with b=" << b << ":\n"
            << "| "
            << std::left
            << std::setw(40)
            << "Algorithm"
            << " | "
            << std::right
            << std::setw(20)
            << "Weight"
            << " | "
            << std::setw(12)
            << "Time (s)"
            << " |\n";
        std::locale loc("");
        std::cout.imbue(loc);
        for (auto & algo : algos) {
            G.resetToBigBang();
            G.applyNextDelta();
            algo->set_num_matchings(config.b);
            std::cout << "| " << std::left << std::setw(40) << algo->getName() << std::flush;
            if (!algo->prepare()) {
                std::cout  << std::right << " | "
                    << std::setw(35)
                    << " FAILED TO PREPARE "
                    << " |\n";
                continue;
            }
            t.restart();
            algo->run();
            auto time = t.elapsed<>();
            auto res = algo->deliver();
            write_result(config, *algo, time, res);
            std::cout  << std::right << " | "
                << std::setw(20)
                << res
                << " | "
                << std::setw(12)
                << std::fixed << std::setprecision(6)
                << time
                << " |\n";
        }
    }

    if (config.writeOutputfile) {
        outfile.close();
    }

    return 0;
}
