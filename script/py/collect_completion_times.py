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
    if not os.path.exists(args.config):
        logging.error("%s does not exist", args.config)
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

    times = recursivedict()
    
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
                    
                    if not "schedule" in file:
                        continue
                    
                    tokens = file.split("_")
                    method = tokens[0]

                    if method not in existing_greedy and method not in \
                       existing_heuristics and method not in existing_random:
                        continue

                    if len(tokens) > 2:
                        cppseed = tokens[2].replace(".csv","")
                    else:
                        cppseed = seed
                    
                    logging.debug("Examining %s", str(method + "-" + cppseed))
                    
                    schedule = csv.reader(open(os.path.join(res_path, file), 
                                               "r"))
                    next(schedule)
                    
                    experiment = (method, nodes, jobs, lambdaa, delta, 
                                  distribution, seed)
                    
                    for row in schedule:
                        jobIDs = (row[6], row[7])
                        finish_time = float(row[16])
                        if finish_time > 0:
                            sub_time = float(row[8])
                            times[experiment][cppseed][jobIDs] = (sub_time,
                                                                  finish_time)
                            
                    found_methods.add(method)
    
    logging.debug("Found methods %s", str(found_methods))

    if not found_methods:
        logging.error("No cpp result found")
        sys.exit(1)
    
    for experiment in sorted(times):
        logging.debug("Examining %s", str(experiment))
        times_file_name = ("_".join(experiment)) + ".csv"
        times_file = open(times_file_name, "w")
        times_file.write("Jobs,UniqueJobsID,SubmissionTime,FinishTime\n")
        # lists required to compute averages
        avg_submissionTime = [0.0] * (int(experiment[2]) + 1)
        avg_finishTime = [0.0] * (int(experiment[2]) + 1)
        n_cppseeds = 0
        is_random = True
        # loop over all cppseeds
        for cppseed in times[experiment]:
            times_alljobs = times[experiment][cppseed]
            # for non-random methods
            if cppseed == experiment[-1]:
                is_random = False
                for jobIDs in times_alljobs:
                    times_file.write(",".join(jobIDs))
                    times_file.write("," + str(times_alljobs[jobIDs][0]) + "," + 
                                     str(times_alljobs[jobIDs][1]) + "\n")
            else:
                count = 0
                for jobIDs in times_alljobs:
                    avg_submissionTime[count] += times_alljobs[jobIDs][0]
                    avg_finishTime[count] += times_alljobs[jobIDs][1]
                    count += 1
            n_cppseeds += 1
        # compute averages
        if is_random:
            for count in range(len(avg_submissionTime)):
                avg_ST = avg_submissionTime[count] / n_cppseeds
                avg_FT = avg_finishTime[count] / n_cppseeds
                # write averages on file
                times_file.write("avg_ID,avg_u_ID, " + str(avg_ST) + ", " +
                                 str(avg_FT) + "\n")
        # close file
        times_file.close()
                    

if __name__ == "__main__":
    main()