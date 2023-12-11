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
            mu = row[4]
            seed = row[5]
            cost = float(row[6])

            experiment = (nodes, jobs, lambdaa, mu, seed)

            costs[experiment][method] = cost

    results_file_name = "results" + args.output + ".csv"
    results_file = open(results_file_name, "w")
    results_file.write("Nodes, Jobs, Lambda, Mu, Seed, Pure_Greedy, ")
    results_file.write("Randomized_Greedy, Local_Search, Gain (G vs RG), ")
    results_file.write("Gain (G vs LS), Gain (RG vs LS)\n")

    for experiment in sorted(costs):
        logging.debug("Examining %s", str(experiment))
        results_file.write(",".join(experiment))
        results = costs[experiment]

        greedy = ()
        random_greedy = ()
        local_search = ()

        for method in results:
            cost = results[method]
            if method == baseline_method:
                greedy = (method, cost)
            elif method == (baseline_method + "_R"):
                random_greedy = (method, cost)
            elif method == "LS":
                local_search = (method, cost)

        gain_g_rg = (greedy[1] - random_greedy[1]) / greedy[1]
        gain_g_ls = (greedy[1] - local_search[1]) / greedy[1]
        gain_rg_ls = (random_greedy[1] - local_search[1]) / random_greedy[1]
        results_file.write(", {:.2f}".format(greedy[1]) +\
                           ", {:.2f}".format(random_greedy[1]) +\
                           ", {:.2f}".format(local_search[1]) +\
                           ", {:.2f}%".format(gain_g_rg * 100) +\
                           ", {:.2f}%".format(gain_g_ls * 100) +\
                           ", {:.2f}%".format(gain_rg_ls * 100))
        results_file.write("\n")

    results_file.close()


if __name__ == "__main__":
    main()