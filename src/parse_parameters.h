/**
 * Copyright (C) 2021, 2022 : Kathrin Hanauer, Jonathan Trummer
 *
 * This file is part of DJMatch and licensed under GPLv3.
 */

#pragma once

#include <string>
#include <sstream>
#include "matching/matching_config.h"
#include "extern/argtable3-3.0.3/argtable3.h"
#include "matching/matching_defs.h"

std::string base_name(std::string const &path) {
    return path.substr(path.find_last_of("/\\") + 1);
}

int parse_matching_parameters(int argn, char **argv,  MatchingConfig &matching_config,
                          std::string &graph_filename, bool &ret) {

    const char *progname = argv[0];

    struct arg_lit *help = arg_lit0(NULL, "help", "Print help.");
    struct arg_lit *swaps = arg_lit0(NULL, "swaps", "use swaps heuristic on result.");
    struct arg_lit *swaps_and_normal = arg_lit0(NULL, "swaps-and-normal", "run with and without swap heuristic.");
    struct arg_lit *swaps_reverse_sort = arg_lit0(NULL, "swaps-reverse-sort", "for swaps sort edges in reverse, in ie ascending weight.");
    struct arg_lit *sanitycheck = arg_lit0(NULL, "sanity-check", "perform sanity checks on results.");
    struct arg_lit *greedy = arg_lit0(NULL, "greedy", "use greedy heuristic.");
    struct arg_lit *node_centered = arg_lit0(NULL, "node-centered", "run node centered.");
    struct arg_lit *greedycoloring = arg_lit0(NULL, "greedy-coloring", "run coloring algorithm on greedy b-matching result.");
    struct arg_lit *gpa = arg_lit0(NULL, "gpa", "run gpa.");
    struct arg_lit *console_log = arg_lit0(NULL, "console_log", "log more output to console");
    struct arg_int *b     = arg_intn(NULL, "b", NULL, 0, 10,  "do a b matching for B=b");
    struct arg_int *l     = arg_int0(NULL, "l", NULL, "l parameter for GPA-ROMA - number of iterations of improvements");
    struct arg_int *seed     = arg_int0(NULL, "seed", NULL, "set seed for RNG");
    struct arg_int *oseed     = arg_int0(NULL, "oseed", NULL, "set seed for RNG used for shuffling the order of algorithms");
    struct arg_dbl *global_threshold = arg_dbln("t", "threshold", NULL, 0, 100, "Node-Centered: threshold t of min edge weight t*MAX matched in first round");
    struct arg_str *algorithm = arg_strn("a", "algorithm", NULL, 0, 5, "single algorithm to run. any of: nodecentered, bmatching, biterative, gpa, bgreedy-extend, bgreedy-color, greedy-it, gpa-it, k-ec");
    struct arg_str *aggregation_type = arg_strn("g", "aggregation-type", NULL, 0, 5, "aggregation type to use for node centered. any of: sum, max, avg, median, bsum");
    struct arg_lit *global_swaps = arg_lit0(NULL, "global-swaps", "use global swaps instead of local (ie after end of iterations instead of after each it.)");


    struct arg_str *filename = arg_strn(NULL, NULL, "FILE", 1, 1, "Path to graph file to partition.");
    struct arg_str *outfile = arg_str0(NULL, "results-output", NULL, "Target file for result output");
    struct arg_end *end = arg_end(100);

    void *argtable[] = {
            help, filename, swaps, swaps_and_normal,
            swaps_reverse_sort,
            greedy, greedycoloring, gpa,
            node_centered,
            global_threshold,
            algorithm,
            aggregation_type,
            global_swaps,
            console_log, b, l,
            seed, oseed,
            sanitycheck, outfile,

            end
    };

    // Parse arguments.
    int nerrors = arg_parse(argn, argv, argtable);
    ret = false;

    // Catch case that help was requested.
    if (help->count > 0) {
        printf("Usage: %s", progname);
        arg_print_syntax(stdout, argtable, "\n");
        arg_print_glossary(stdout, argtable,"  %-40s %s\n");
        arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
        ret = true;
        return 0;
    }

    if (nerrors > 0) {
        arg_print_errors(stderr, end, progname);
        printf("Try '%s --help' for more information.\n",progname);
        arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
        return 1;
    }


    if (filename->count > 0) {
        graph_filename = filename->sval[0];
        matching_config.graph_filename = base_name(graph_filename);
    }

    if (gpa->count > 0) {
        matching_config.gpa = true;
    }

    if (console_log->count > 0) {
        matching_config.console_log = true;
    }

    if (swaps->count > 0) {
        matching_config.swaps = true;
    }

    if (swaps_and_normal->count > 0) {
        matching_config.swaps = true;
        matching_config.swaps_and_normal = true;
    }

    if (swaps_reverse_sort->count > 0) {
        matching_config.swaps_reverse_sort = true;
    }

    if (sanitycheck->count > 0) {
        matching_config.sanitycheck = true;
    }

    if (greedy->count > 0) {
        matching_config.greedy = true;
    }

    if (node_centered->count > 0) {
        matching_config.node_centered = true;
    }

    if (greedycoloring->count > 0) {
        matching_config.greedycoloring = true;
    }

    if (b->count > 0) {
        for (auto i = 0; i < b->count; i++) {
            matching_config.all_bs.push_back(b->ival[i]);
        }
    }

    if (l->count > 0) {
        matching_config.l = l->ival[0];
        matching_config.roma = true;
    }

    if (seed->count > 0) {
        matching_config.seed = seed->ival[0];
    }

    if (oseed->count > 0) {
        matching_config.algorithm_order_seed = oseed->ival[0];
    }

    if (global_threshold->count > 0) {
        for (auto i = 0; i < global_threshold->count; i++) {
            if (global_threshold->dval[i] > 0) {
                matching_config.global_thresholds.push_back(global_threshold->dval[i]);
            }
        }
    }

    if (algorithm->count > 0) {
        for (auto i = 0; i < algorithm->count; i++) {
            std::string alg = algorithm->sval[i];
            if (alg.compare("nodecentered") == 0) {
                matching_config.algorithms.push_back(MatchingAlgorithm::NODE_CENTERED);
            } else if (alg.compare("bmatching") == 0 || alg.compare("bgreedy-color") == 0) {
                matching_config.algorithms.push_back(MatchingAlgorithm::BGREEDY_COLOR);
            } else if (alg.compare("bgreedy-extend") == 0) {
                matching_config.algorithms.push_back(MatchingAlgorithm::BGREEDY_EXTEND);
            } else if (alg.compare("biterative") == 0 || alg.compare("greedy-it") == 0) {
                matching_config.algorithms.push_back(MatchingAlgorithm::GREEDY_IT);
            } else if (alg.compare("gpa") == 0 || alg.compare("gpa-it") == 0) {
                matching_config.algorithms.push_back(MatchingAlgorithm::GPA_IT);
            } else if (alg.compare("k-ec") == 0 || alg.compare("k-edgecoloring") == 0) {
                matching_config.algorithms.push_back(MatchingAlgorithm::K_EC);
            } else {
                printf("Invalid algorithm passed! %s\n", alg.c_str());
                arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
                return 1;
            }
        }
    }

    if (aggregation_type->count > 0) {
        for (auto i = 0; i < aggregation_type->count; i++) {
            std::string type = aggregation_type->sval[i];
            if (type.compare("sum") == 0) {
                matching_config.aggregation_types.push_back(AggregateType::SUM);
            } else if (type.compare("max") == 0) {
                matching_config.aggregation_types.push_back(AggregateType::MAX);
            } else if (type.compare("avg") == 0) {
                matching_config.aggregation_types.push_back(AggregateType::AVG);
            } else if (type.compare("median") == 0) {
                matching_config.aggregation_types.push_back(AggregateType::MEDIAN);
            } else if (type.compare("bsum") == 0) {
                matching_config.aggregation_types.push_back(AggregateType::B_SUM);
            } else {
                printf("Invalid aggregation type passed! %s\n", type.c_str());
                arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
                return 1;
            }
        }
    }

    if (global_swaps->count > 0) {
        matching_config.swaps_global = true;
    }


    if (outfile->count > 0) {
        matching_config.outputFile = outfile->sval[0];
        matching_config.writeOutputfile = true;
    }

    arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));

    return 0;
}
