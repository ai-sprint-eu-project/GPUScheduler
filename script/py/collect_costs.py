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
    parser.add_argument("-LSYS", dest="LSYS", help="Use in case of results for large systems",
                        default=False, action="store_true")
    parser.add_argument('-c', "--config", help="Configuration file", 
                        default="config.ini")
    parser.add_argument('-d', "--debug", help="Enable debug messages", 
                        default=False, action="store_true")
    parser.add_argument('-o', "--output", 
                        help="String to be added to the name of the results files", 
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
    existing_greedy = ast.literal_eval(config['Methods']['existing_greedy'])
    existing_heuristics = ast.literal_eval(config['Methods']['existing_heuristics'])
    existing_random = ast.literal_eval(config['Methods']['existing_random'])

    ##########################################################################

    costs = recursivedict()
    
    found_methods = set()

    for content in os.listdir(args.results_directory):
        combined_path = os.path.join(args.results_directory, content)
        if os.path.isdir(combined_path):
            if not content.startswith("1-"):
                continue
            tokens = content.split("-")
            nodes = tokens[1]
            jobs = tokens[2]
            seed = tokens[3]
            distribution = tokens[4]
            lambdaa = tokens[5]
            for res_dir in os.listdir(combined_path):
                res_path = os.path.join(combined_path, res_dir)
                if not os.path.isdir(res_path) or not res_dir.startswith("result"):
                    continue
                tokens = res_dir.split("_")
                delta = tokens[1]
                
                logging.debug("Examining %s", str(res_dir))
                
                for file in os.listdir(res_path):
                    if os.path.isdir(file):
                        continue
                    
                    # LARGE SYSTEMS CASE
                    if args.LSYS:
                        if not file.startswith("all_costs"):
                            continue
                        cost_data = csv.reader(open(os.path.join(res_path, 
                                                                 file), "r"))
                        for row in cost_data:
                            # method and cpp_seed for random methods
                            info = row[0].split("_")
                            method = info[0]
                            if method not in existing_greedy and \
                                method not in existing_heuristics and \
                                method not in existing_random:
                                continue
                            if method in existing_random:
                                cppseed = info[-1].replace(".csv", "")
                            else:
                                cppseed = seed
                            logging.debug("Examining %s", str(method+"-"+cppseed))
                            total_cost = float(row[-1])
                            experiment = (method, nodes, jobs, lambdaa, delta, 
                                          distribution, seed)
                            found_methods.add(method)
                            costs[experiment][cppseed] = total_cost
                    
                    # SMALL SYSTEMS CASE
                    else:
                        if not file.startswith("cost-"):
                            continue
                        tokens = file.split("-")[1]
                        if "_" in tokens:
                            tokens = tokens.split("_")
                            method = tokens[0]
                            cppseed = tokens[1].replace(".csv", "")
                        else:
                            method = tokens.replace(".csv", "")
                            cppseed = seed
                        logging.debug("Examining %s", str(method + "-" + cppseed))
                        total_cost = 0.0
                        cost_data = csv.reader(open(os.path.join(res_path, 
                                                                 file), "r"))
                        next(cost_data)
                        for row in cost_data:
                            total_cost = total_cost + float(row[4])
                        experiment = (method, nodes, jobs, lambdaa, delta, 
                                      distribution, seed)
                        found_methods.add(method)
                        costs[experiment][cppseed] = total_cost
    
    logging.debug("Found methods %s", str(found_methods))

    if not found_methods:
        logging.error("No cpp result found")
        sys.exit(1)

    results_file_name = "total_costs" + args.output + ".csv"
    results_file = open(results_file_name, "w")
    results_file.write("Method,Nodes,Jobs,Lambda,Delta,Distribution,")
    results_file.write("Seed,CppSeed,TotalCost\n")

    averages_file_name = "total_costs_averaged" + args.output + ".csv"
    averages_file = open(averages_file_name, "w")
    averages_file.write("Method,Nodes,Jobs,Lambda,Delta,Distribution,")
    averages_file.write("Seed,TotalCost\n")

    for experiment in sorted(costs):
        logging.debug("Examining %s", str(experiment))

        results = costs[experiment]

        results_file.write(",".join(experiment))
        averages_file.write(",".join(experiment))

        average = 0.0
        ncppseed = 0

        for cppseed in results:
            if ncppseed == 0:
                results_file.write("," + str(cppseed) +\
                                   "," + str(results[cppseed]) + "\n")
            else:
                results_file.write(",,,,,,," + str(cppseed) +\
                                   "," + str(results[cppseed]) + "\n")
            average += results[cppseed]
            ncppseed += 1

        average /= ncppseed
        averages_file.write("," + str(average) + "\n")

    results_file.close()
    averages_file.close()


if __name__ == "__main__":
    main()
