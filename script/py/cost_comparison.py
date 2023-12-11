#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Copyright 2020-2021 Federica Filippini

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

     http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
"""

import sys
import os
import ast
import configparser as cp
import argparse
import csv
import logging
import collections


def recursivedict():
    return collections.defaultdict(recursivedict)


def main ():

    parser = argparse.ArgumentParser(description="Compute averages of random methods")

    parser.add_argument("results_directory", help="The directory containing the folders with the results to be processed")
    parser.add_argument('-c', "--config", help="Configuration file", 
                        default="config.ini")
    parser.add_argument('-d', "--debug", help="Enable debug messages", 
                        default=False, action="store_true")
    parser.add_argument('-o', "--output", help="String to be added to the name of the results files", 
                        default="")

    args = parser.parse_args()

    if args.debug:
        logging.basicConfig(level=logging.DEBUG, format='%(levelname)s: %(message)s')
    else:
        logging.basicConfig(level=logging.INFO, format='%(levelname)s: %(message)s')

    if not os.path.exists(args.results_directory):
        logging.error("%s does not exist", args.results_directory)
        sys.exit(1)

    ##########################################################################

    ## configuration parameters
    config = cp.ConfigParser()
    config.read(args.config)

    # methods
    baseline_method = ast.literal_eval(config['Methods']['baseline_method'])
    existing_heuristics = ast.literal_eval(config['Methods']['existing_heuristics'])
    existing_greedy = ast.literal_eval(config['Methods']['existing_greedy'])
    existing_random = ast.literal_eval(config['Methods']['existing_random'])

    ##########################################################################

    costs = recursivedict()
    
    for file in os.listdir(args.results_directory):
        combined_path = os.path.join(args.results_directory, file)
        if os.path.isdir(file):
            continue
        if not file.startswith("total_costs_averaged"):
            continue

        logging.debug("Examining %s", str(file))

        cost_data = csv.reader(open(combined_path, "r"))
        next(cost_data)
        
        for row in cost_data:
            method = row[0]
            nodes = row[1]
            jobs = row[2]
            lambdaa = row[3]
            delta = row[4]
            distribution = row[5]
            seed = row[6]
            cost = float(row[7])

            experiment = (nodes, jobs, lambdaa, delta, distribution, seed)

            costs[experiment][method] = cost

    results_file_name = "results" + args.output + ".csv"
    results_file = open(results_file_name, "w")
    results_file.write("Nodes, Jobs, Lambda, Delta, Distribution, Seed, Baseline, Best_FPM, ")
    results_file.write("Best_Greedy, Best_Random, Gain (baseline VS best fpm), ")
    results_file.write("Gain (baseline VS best greedy), Gain (baseline VS best random), ")
    results_file.write("Gain (best greedy VS best fpm), Gain (best random VS best fpm), ")
    results_file.write("Gain (best greedy VS best random), TC baseline, ")
    results_file.write("TC best_fpm, TC best_greedy, TC best_random\n")

    for experiment in sorted(costs):
        logging.debug("Examining %s", str(experiment))
        results_file.write(",".join(experiment))
        results = costs[experiment]

        best_fpm = ()
        best_greedy = ()
        best_random = ()
        baseline = ()

        for method in results:
            cost = results[method]
            if method in existing_random:
                if not best_random or cost < best_random[1]:
                    best_random = (method, cost)
            elif method in existing_greedy:
                if not best_greedy or cost < best_greedy[1]:
                    best_greedy = (method, cost)
            elif method in existing_heuristics:
                if not best_fpm or cost < best_fpm[1]:
                    best_fpm = (method, cost)
            if method == baseline_method:
                baseline = (method, cost)

        if baseline:
            results_file.write(", " + baseline[0])
        else:
            results_file.write(", - ")
        if best_fpm:
            results_file.write(", " + best_fpm[0])
        else:
            results_file.write(", - ")
        if best_greedy:
            results_file.write(", " + best_greedy[0])
        else:
            results_file.write(", - ")
        if best_random:
            results_file.write(", " + best_random[0])
        else:
            results_file.write(", - ")

        if baseline and best_fpm:
            gain_b_fpm = (best_fpm[1] - baseline[1]) / best_fpm[1]
            results_file.write(", {:.2f}%".format(gain_b_fpm * 100))
        else:
            gain_b_fpm = "-"
            results_file.write(", - ")
        if baseline and best_greedy:
            gain_b_gr = (baseline[1] - best_greedy[1]) / baseline[1]
            results_file.write(", {:.2f}%".format(gain_b_gr * 100))
        else:
            results_file.write(", - ")
        if baseline and best_random:
            gain_b_rand = (baseline[1] - best_random[1]) / baseline[1]
            results_file.write(", {:.2f}%".format(gain_b_rand * 100))
        else:
            results_file.write(", - ")
        if best_greedy and best_fpm:
            gain_gr_fpm = (best_fpm[1] - best_greedy[1]) / best_fpm[1]
            results_file.write(", {:.2f}%".format(gain_gr_fpm * 100))
        else:
            results_file.write(", - ")
        if best_random and best_fpm:
            gain_rand_fpm = (best_fpm[1] - best_random[1]) / best_fpm[1]
            results_file.write(", {:.2f}%".format(gain_rand_fpm * 100))
        else:
            results_file.write(", - ")
        if best_greedy and best_random:
            gain_gr_rand = (best_greedy[1] - best_random[1]) / best_greedy[1]
            results_file.write(", {:.2f}%".format(gain_gr_rand * 100))
        else:
            results_file.write(", - ")
        
        if baseline:
            results_file.write(", " + str(baseline[1]))
        else:
            results_file.write(", - ")
        if best_fpm:
            results_file.write(", " + str(best_fpm[1]))
        else:
            results_file.write(", - ")
        if best_greedy:
            results_file.write(", " + str(best_greedy[1]))
        else:
            results_file.write(", - ")
        if best_random:
            results_file.write(", " + str(best_random[1]))
        else:
            results_file.write(", - ")

        results_file.write("\n")

    results_file.close()


if __name__ == "__main__":
    main()
