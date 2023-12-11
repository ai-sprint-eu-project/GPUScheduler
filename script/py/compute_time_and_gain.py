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

from datetime import datetime


def recursivedict():
    return collections.defaultdict(recursivedict)


def main ():

    parser = argparse.ArgumentParser(description="Compute elapsed time and gain")

    parser.add_argument("logs_directory", help="The directory containing the folders with the logs to be processed")
    parser.add_argument('-c', "--config", help="Configuration file", 
                        default="config.ini")
    parser.add_argument('-d', "--debug", help="Enable debug messages", 
                        default=False, action="store_true")
    parser.add_argument('-o', "--output", 
                        help="String to be added to the name of the results files", 
                        default="")
    parser.add_argument('-g', "--gain", 
                        help="Enable gain computation for random methods", 
                        default=False, action="store_true")

    args = parser.parse_args()

    if args.debug:
        logging.basicConfig(level=logging.DEBUG, format='%(levelname)s: %(message)s')
    else:
        logging.basicConfig(level=logging.INFO, format='%(levelname)s: %(message)s')

    if not os.path.exists(args.logs_directory):
        logging.error("%s does not exist", args.logs_directory)
        sys.exit(1)

    ##########################################################################

    ## configuration parameters
    config = cp.ConfigParser()
    config.read(args.config)

    # methods
    existing_heuristics = ast.literal_eval(config['Methods']['existing_heuristics'])
    existing_greedy = ast.literal_eval(config['Methods']['existing_greedy'])
    existing_random = ast.literal_eval(config['Methods']['existing_random'])

    ##########################################################################

    all_costs = recursivedict()

    all_gains = recursivedict()
    average_gains = recursivedict()

    all_times = recursivedict()

    found_methods = set()

    for content in os.listdir(args.logs_directory):
        combined_path = os.path.join(args.logs_directory, content)
        if os.path.isdir(combined_path):
            tokens = content.split("_")
            method = tokens[0]
            if method not in existing_heuristics and \
               method not in existing_greedy and \
               method not in existing_random:
                continue
            number_initial_jobs = tokens[1]
            if number_initial_jobs != "1":
                continue
            nodes = tokens[2]
            jobs = tokens[3]
            seed = tokens[4]
            distribution = tokens[5]
            lambdaa = tokens[6]
            delta = tokens[7]

            if method in existing_random:
                is_random = True
                cppseed = tokens[8]
            else:
                cppseed = seed

            logging.debug("Examining %s", str(method + "-" + nodes + "-" +\
                                              distribution + "-" + seed))

            for file in os.listdir(combined_path):
                if os.path.isdir(file):
                    continue
                if not file.startswith("CppTests_execution"):
                    continue

                log_data = open(os.path.join(combined_path, file),"r")
                
                experiment = (method, nodes, jobs, lambdaa, delta, 
                              distribution, seed)
                found_methods.add(method)

                avg_gains = [0.0, 0.0, 0.0]
                count = [0, 0, 0]
                n_iterations = 0

                for row in log_data:
                    if row.startswith("start time"):
                        start_time = datetime.strptime(row[11:-1], "%Y-%m-%d %H:%M:%S.%f")
                    elif row.startswith("finish time"):
                        finish_time = datetime.strptime(row[12:-1], "%Y-%m-%d %H:%M:%S.%f")
                    #
                    #   TEMP: we should have uniform outputs!
                    #
                    else:
                        if method == "PathRelinking":
                            elems = row.split(",")
                            if len(elems) > 1:
                                n_iterations = elems[0]
                        else:
                            if "ITER" in row:
                                n_iterations = row.split(" ")[2]
                                if args.gain:
                                    next_row = next(log_data)
                                    costs = [-1, -1, -1]
                                    gains = [-1, -1, -1]
                                    while not next_row.startswith("###"):
                                        next_row = next(log_data)
                                        if "best cost =" in next_row:
                                            next_row = next_row.split(" ")
                                            if is_random:
                                                costs[0] = float(next_row[4][:-1])
                                            else:
                                                costs[0] = float(next_row[3])
                                        if is_random:
                                            next_row = next_row.split(" ")
                                            if "greedy" in next_row:
                                                costs[1] = float(next_row[7][:-1])
                                            elif "random" in next_row:
                                                costs[2] = float(next_row[7][:-1])
                                    # if exists greedy cost...
                                    if costs[1] > 0:
                                        # greedy VS best cost
                                        gains[0] = (costs[1] - costs[0]) / costs[1]
                                        avg_gains[0] += gains[0]
                                        count[0] += 1
                                        # if exists random cost...
                                        if costs[2] > 0:
                                            # greedy VS random
                                            gains[2] = (costs[1] - costs[2]) / costs[1]
                                            avg_gains[2] += gains[2]
                                            count[2] += 1
                                    # if exists greedy cost...
                                    if costs[2] > 0:
                                        # random VS best cost
                                        gains[1] = (costs[2] - costs[0]) / costs[2]
                                        avg_gains[1] += gains[1]
                                        count[1] += 1

                                    # add costs and gains to the corresponding 
                                    # dictionaries
                                    all_costs[experiment][n_iterations] = costs
                                    all_gains[experiment][n_iterations] = gains

                # compute and add elapsed time
                elapsed_time = (finish_time - start_time).total_seconds()
                time_per_iter = elapsed_time / (int(n_iterations) - 1)
                all_times[experiment][cppseed] = (elapsed_time, time_per_iter)

                # add average gain to the corresponding dictonary
                if args.gain:
                    for idx in range(3):
                        if count[idx] > 0:
                            avg_gains[idx] /= count[idx]
                    average_gains[experiment] = avg_gains
    
    logging.debug("Found methods %s", str(found_methods))

    if not found_methods:
        logging.error("No log found")
        sys.exit(1)

    times_file_name = "elapsed_time" + args.output + ".csv"
    times_file = open(times_file_name, "w")
    times_file.write("Method,Nodes,Jobs,Lambda,Delta,Distribution,")
    times_file.write("Seed,CppSeed,Time,TimePerIter\n")

    times_file_name_avg = "elapsed_time_averaged" + args.output + ".csv"
    times_file_avg = open(times_file_name_avg, "w")
    times_file_avg.write("Method,Nodes,Jobs,Lambda,Delta,Distribution,")
    times_file_avg.write("Seed,AvgTime,AvgTimePerIter\n")

    for experiment in sorted(all_times):
        logging.debug("Examining %s", str(experiment))
        times = all_times[experiment]
        times_file.write(",".join(experiment))
        times_file_avg.write(",".join(experiment))

        average1 = 0.0
        average2 = 0.0
        ncppseed = 0

        for cppseed in times:
            if ncppseed == 0:
                times_file.write("," + str(cppseed) + "," + \
                                 str(times[cppseed][0])+ "," + \
                                 str(times[cppseed][1]) + "\n")
            else:
                times_file.write(",,,,,,," + str(cppseed) + "," + \
                                 str(times[cppseed][0])+ "," + \
                                 str(times[cppseed][1]) + "\n")
            average1 += times[cppseed][0]
            average2 += times[cppseed][1]
            ncppseed += 1

        average1 /= ncppseed
        average2 /= ncppseed
        times_file_avg.write("," + str(average1) + "," + str(average2) + "\n")

    times_file.close()
    times_file_avg.close()


    # results_file_name = "total_gain" + args.output + ".csv"
    # results_file = open(results_file_name, "w")
    # results_file.write("Method,Nodes,Jobs,Lambda,Seed,CppSeed,Delta,Gain")
    # results_file.write("\n")

    # averages_file_name = "total_gain_averaged" + args.output + ".csv"
    # averages_file = open(averages_file_name, "w")
    # averages_file.write("Method,Nodes,Jobs,Lambda,Seed,CppSeed,Delta,")
    # averages_file.write("AverageGain\n")

    # for experiment in sorted(average_gains):
    #     logging.debug("Examining %s", str(experiment))

    #     results = average_gains[experiment]

    #     results_file.write(",".join(experiment))
    #     averages_file.write(",".join(experiment))

    #     average = 0.0
    #     ncppseed = 0

    #     for cppseed in results:
    #         if ncppseed == 0:
    #             results_file.write("," + str(cppseed) +\
    #                               ", {:.2f}%".format(results[cppseed] * 100) +\
    #                               "\n")
    #         else:
    #             results_file.write(",,,,,," + str(cppseed) +\
    #                               ", {:.2f}%".format(results[cppseed] * 100) +\
    #                               "\n")
    #         average += results[cppseed]
    #         ncppseed += 1

    #     average /= ncppseed
    #     averages_file.write(", {:.2f}%".format(average * 100) + "\n")

    # results_file.close()
    # averages_file.close()

    # if args.debug:
    #     full_results_file_name = "gain.csv"

    #     for full_exp in sorted(gains):
    #         folder = args.logs_directory + "/" + "-".join(full_exp) + "/"
    #         full_results_file = open(folder+full_results_file_name, "w")
    #         full_results_file.write("iter, Gain\n")
    #         ggg = gains[full_exp]
    #         for n_iter in sorted(ggg):
    #             full_results_file.write(str(n_iter) +\
    #                                     ", {:.2f}%".format(ggg[n_iter]*100) +\
    #                                     "\n")
    #         full_results_file.close()


if __name__ == "__main__":
    main()
