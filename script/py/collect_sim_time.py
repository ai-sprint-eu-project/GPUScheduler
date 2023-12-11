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
import argparse
import csv
import logging
import collections


def recursivedict():
    return collections.defaultdict(recursivedict)


def main ():

    parser = argparse.ArgumentParser(description="Compute averages of random methods")

    parser.add_argument("results_directory", help="The directory containing the folders with the results to be processed")
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

    sim_times = recursivedict()

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
                
                for file in os.listdir(res_path):
                    if os.path.isdir(file) or not file.startswith("cost-"):
                        continue
                    tokens = file.split("-")[1]
                    if "_" in tokens:
                        tokens = tokens.split("_")
                        method = tokens[0]
                        cppseed = tokens[1].replace(".csv", "")
                    else:
                        method = tokens.replace(".csv", "")
                        cppseed = seed
                        logging.debug("Examining %s", str(method + "-" + 
                                                          cppseed))
                        time_data = csv.reader(open(os.path.join(res_path, 
                                                                 file), "r"))
                        time_list = list(time_data)
                        sim_time = float(time_list[-1][1])
                        experiment = (method, nodes, jobs, lambdaa, delta, 
                                      distribution, seed)
                        found_methods.add(method)
                        sim_times[experiment][cppseed] = sim_time

    logging.debug("Found methods %s", str(found_methods))

    if not found_methods:
        logging.error("No cpp result found")
        sys.exit(1)

    results_file_name = "sim_times" + args.output + ".csv"
    results_file = open(results_file_name, "w")
    results_file.write("Method, Nodes, Jobs, Lambda, Delta, Distribution, ")
    results_file.write("Seed, CppSeed, sim_time\n")

    averages_file_name = "sim_times_averaged" + args.output + ".csv"
    averages_file = open(averages_file_name, "w")
    averages_file.write("Method, Nodes, Jobs, Lambda, Delta, Distribution, ")
    averages_file.write("Seed, sim_time\n")

    for experiment in sorted(sim_times):
        logging.debug("Examining %s", str(experiment))

        results = sim_times[experiment]

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
