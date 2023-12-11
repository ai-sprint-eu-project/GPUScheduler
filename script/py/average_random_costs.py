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

import os
import ast
import configparser as cp
import argparse
import csv
import logging

import collections

import pandas as pd


def recursivedict():
    return collections.defaultdict(recursivedict)


def main ():

    parser = argparse.ArgumentParser(description="Compute averages of random methods")

    parser.add_argument("results_directory", help="The directory containing the folders with the results to be processed")
    parser.add_argument('-c', "--config", help="Configuration file", 
                        default="config.ini")

    args = parser.parse_args()

    if not os.path.exists(args.results_directory):
        sys.exit(1)

    ##########################################################################

    ## configuration parameters
    config = cp.ConfigParser()
    config.read(args.config)

    # methods
    methods         = ast.literal_eval(config['Methods']['methods'])
    existing_cpp    = ast.literal_eval(config['Methods']['existing_cpp'])

    ##########################################################################

    costs = recursivedict()

    found_methods = set()

    for content in os.listdir(args.results_directory):
        combined_path = os.path.join(args.results_directory, content)
        if os.path.isdir(combined_path):
            for file in os.listdir(combined_path):
                if os.path.isdir(file):
                    continue
                if not file.startswith("cost-"):
                    continue
                tokens = file.split("-")
                if tokens[0] != "cost":
                    continue
                method = tokens[1]
                if method not in existing_cpp or "_R" not in method:
                    continue
                number_initial_jobs = tokens[2]
                if number_initial_jobs != "1":
                    continue
                nodes = tokens[3]
                jobs = tokens[4]
                lambdaa = tokens[5]
                mu = tokens[6]
                seeds = tokens[7].replace(".csv", "").split("_")
                seed = seeds[0]
                cppseed = seeds[1]

                total_cost = 0.0

                cost_data = csv.reader(open(os.path.join(combined_path, file), 
                                            "r"))
                next(cost_data)
                for row in cost_data:
                    total_cost = total_cost + float(row[4])

                experiment = (method, nodes, jobs, lambdaa, mu, seed)

                found_methods.add(method)
                costs[experiment][cppseed] = total_cost
    logging.debug("Found methods %s", str(found_methods))

    if not found_methods:
        logging.error("No random result found")
        sys.exit(1)

    results_file_name = "average_costs.csv"
    results_file = open(results_file_name, "w")
    results_file.write("Method, Nodes, Jobs, Lambda, Mu, Seed, AverageCost")
    results_file.write("\n")

    for experiment in sorted(costs):
        logging.debug("Examining %s", str(experiment))

        results = costs[experiment]

        results_file.write(",".join(experiment))

        average = 0.0
        ncppseed = 0

        for cppseed in results:
            average += results[cppseed]
            ncppseed += 1

        average /= ncppseed
        results_file.write("," + str(average) + "\n")

    results_file.close()


if __name__ == "__main__":
    main()