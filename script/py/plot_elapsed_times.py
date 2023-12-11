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

import os
import sys
import ast
import configparser as cp
import argparse
import logging
import pandas as pd
import matplotlib.pyplot as plt


def createFolder (directory):
    try:
        if not os.path.exists(directory):
            os.makedirs(directory)
    except OSError:
        print ("Error: Creating directory. " +  directory)


def compute_average_times (times):
    avg_times = pd.DataFrame()
    times_by_experiment = times.groupby(["Nodes","Jobs","Lambda"])
    for experiment in times_by_experiment:
        times_all_seeds = experiment[1].copy()
        avg_row = times_all_seeds.drop("Seed",axis=1).mean()
        avg_times = avg_times.append(avg_row, ignore_index=True)
    return (avg_times)



def plot_times (avg_times, prefix, all_distributions, distr, all_methods, 
                fig_folder):
    # define colormap
    cmap = plt.get_cmap("Set1")
    # plot times for different lambdas
    times_by_lambda = avg_times.groupby(["Lambda"])
    for lambdaa in times_by_lambda:
        times = lambdaa[1]
        title1 = prefix + distr + "_" + str(lambdaa[0])
        current_figure = plt.figure()
        ax = current_figure.gca()
        # loop over all methods
        for method in all_methods:
            #colorVal = cmap.colors[all_methods[method]]
            colname = prefix + method
            # plot total time
            if colname in times.columns:
                times.plot(x = "Nodes", y = colname, linestyle = "solid", 
                           ax = ax, label = (method), #color = colorVal,
                           linewidth = 3, fontsize = 14, logy = True)
        handles,labels = ax.get_legend_handles_labels()
        ax.legend(handles, labels, loc = "center right", fontsize = 14)
        plt.xlabel("Number of nodes", fontsize = 14)
        plt.ylabel("Execution time [s]", fontsize = 14)
        #plt.title(title1, fontsize = 14)
        if fig_folder != "":
            current_figure.savefig(fig_folder + "/" + title1 + ".pdf", 
                                   format="pdf", dpi=1000, 
                                   bbox_inches = "tight")
            plt.close(current_figure)
        else:
            plt.show()


def main ():

    parser = argparse.ArgumentParser(description="Plot elapsed times of all methods")

    parser.add_argument("results_directory", help="The directory containing the files with elapsed times")
    parser.add_argument("-c", "--config", help="Configuration file", 
                        default="config.ini")
    parser.add_argument("-d", "--debug", help="Enable debug messages", 
                        default=False, action="store_true")
    parser.add_argument("-p", "--plot", 
                        help="Path to the directory where plots should be saved", 
                        default="")
    
    args = parser.parse_args()

    if args.debug:
        logging.basicConfig(level=logging.DEBUG, format="%(levelname)s: %(message)s")
    else:
        logging.basicConfig(level=logging.INFO, format="%(levelname)s: %(message)s")

    if not os.path.exists(args.results_directory):
        logging.error("%s does not exist", args.results_directory)
        sys.exit(1)

    ##########################################################################

    ## configuration parameters
    config = cp.ConfigParser()
    config.read(args.config)

    # distributions
    all_distributions = ast.literal_eval(config["RandomParameters"]["distributions"])
    vals = list(range(len(all_distributions)))
    all_distributions = dict(zip(all_distributions, vals))

    # methods
    all_methods = ast.literal_eval(config["Methods"]["methods"])
    vals = list(range(len(all_methods)))
    all_methods = dict(zip(all_methods, vals))


    ##########################################################################
    
    # folder for figures
    if args.plot != "":
        fig_folder = args.plot + "/times"
        createFolder(fig_folder)
    else:
        fig_folder = ""
    
    # read times data
    for content in os.listdir(args.results_directory):
        combined_path = os.path.join(args.results_directory, content)
        if os.path.isdir(combined_path):
            continue
        if not content.startswith("times") or not content.endswith("csv"):
            continue
        distr = content.split("_")[1][:-4]
        if distr in all_distributions:
            times = pd.read_csv(combined_path, sep=";", decimal=",")
            # compute the average w.r.t. all seeds
            avg_times = compute_average_times(times)
            # plot total times
            plot_times(avg_times, "TT_", all_distributions, distr, 
                       all_methods, fig_folder)
            # plot times per iteration
            plot_times(avg_times, "TT_iter_", all_distributions, distr, 
                       all_methods, fig_folder)
        else:
            logging.error("unknown %s distribution", distr)

        

if __name__ == "__main__":
    main()