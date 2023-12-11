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
import argparse
import logging
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt


def createFolder(directory):
    try:
        if not os.path.exists(directory):
            os.makedirs(directory)
    except OSError:
        print ("Error: Creating directory. " +  directory)


def plot_times (all_distributions, times_all_distr, path = "", title = ""):
    # define colormap
    cmap = plt.get_cmap("Set1")
    # plot times for different distributions
    times_by_distr = times_all_distr.groupby(["Distribution"])
    current_figure = plt.figure()
    ax = current_figure.gca()
    n_lambda = 0
    for distribution in times_by_distr:
        times = distribution[1].copy()
        times["idxs"] = range(len(times))
        times["SubmissionTime"] = times["SubmissionTime"] / 3600
        times["FinishTime"] = times["FinishTime"] / 3600
        distr = distribution[0].split("_")
        col_idx = all_distributions[distr[0]]
        if distr[0] == "exponential":
          col_idx += n_lambda
          n_lambda += 1
        colorVal = cmap.colors[col_idx]
        times.plot(x = "idxs", y = "SubmissionTime", linestyle = "solid", 
                   ax = ax, label = ("submission - " + distribution[0]), 
                   color = colorVal)
        times.plot(x = "idxs", y = "FinishTime", linestyle = "dotted", 
                   ax = ax, label = ("completion - " + distribution[0]),
                   color = colorVal)
    handles,labels = ax.get_legend_handles_labels()
    ax.legend(handles, labels, loc="upper left")
    plt.xlabel("Intervals")
    plt.ylabel("Submissions and completions of jobs [h]")
    plt.title(title)
    if path != "":
        current_figure.savefig(path + "/" + title + ".pdf", format='pdf', 
                               dpi=1000)
        plt.close(current_figure)
    else:
        plt.show()


def plot_costs (all_methods, costs_all_methods, column, title = ""):
    # define colormap
    cmap = plt.get_cmap("Set1")
    # plot times for different methods
    costs_by_method = costs_all_methods.groupby(["Method"])
    ax = plt.gca()
    for method in costs_by_method:
        costs = method[1].copy()
        colorVal = cmap.colors[all_methods[method[0]]]
        costs.plot(x = "Nodes", y = column, kind = "scatter", 
                   ax = ax, label = (column + " - " + method[0]), 
                   color = colorVal)
    handles,labels = ax.get_legend_handles_labels()
    ax.legend(handles, labels, loc="upper left")
    plt.xlabel("Number of nodes")
    plt.ylabel(column)
    plt.title(title)
    plt.show()


def plot_avg_costs (all_methods, avg_costs, title = "", path = ""):
    # define colormap
    cmap = plt.get_cmap("Set1")
    # plot times for different distributions
    costs_by_distribution = avg_costs.groupby(["Distribution"])
    for distribution in costs_by_distribution:
        title1 = distribution[0] + title
        current_figure = plt.figure()
        ax = current_figure.gca()
        costs_by_method = distribution[1].groupby(["Method"])
        for method in costs_by_method:
            costs = method[1].copy()
            colorVal = cmap.colors[all_methods[method[0]]]
            costs.plot(x = "Nodes", y = "TotalCost", linestyle = "solid", 
                       ax = ax, label = ("cost - " + method[0]), 
                       color = colorVal)
            costs.plot(x = "Nodes", y = "TotalTardi", linestyle = "dotted", 
                       ax = ax, label = ("tardi - " + method[0]),
                       color = colorVal)
            handles,labels = ax.get_legend_handles_labels()
            ax.legend(handles, labels, loc="upper left")
            plt.xlabel("Number of nodes")
            plt.ylabel("Total cost [$] and total tardiness [h]")
            plt.title(title1)
        if path != "":
            current_figure.savefig(path + "/" + title1 + ".pdf", format='pdf', 
                                   dpi=1000)
            plt.close(current_figure)
        else:
            plt.show()


def add_to_df (combined_path, df_avg):
    temp = pd.read_csv(combined_path)
    exp_rows = (temp["Distribution"] == "exponential")
    func = lambda x : x["Distribution"] + "_" + str(x["Lambda"])
    temp.loc[exp_rows, "Distribution"] = temp[exp_rows].apply(func, axis = 1)
    temp.drop(["Lambda"], axis = 1)
    return df_avg.append(temp, ignore_index=True)


def get_all_costs (results_directory):
    tardi_avg = pd.DataFrame(columns = ["Method", "Nodes", "Jobs", "Delta", 
                                        "Distribution", "Seed", "TotalTardi"])
    costs_avg = pd.DataFrame(columns = ["Method", "Nodes", "Jobs", "Delta", 
                                        "Distribution", "Seed", "TotalCost"])
    for file in os.listdir(results_directory):
        combined_path = os.path.join(results_directory, file)
        if os.path.isdir(combined_path) or "average" not in file:
            continue
        tokens = file.split("_")
        if tokens[1] == "tardi":
            tardi_avg = add_to_df(combined_path, tardi_avg)
        elif tokens[1] == "costs":
            costs_avg = add_to_df(combined_path, costs_avg)
    all_costs = pd.merge(costs_avg, tardi_avg, on=["Method", "Nodes", "Jobs", 
                                                   "Delta", "Distribution", 
                                                   "Seed"])
    return all_costs


def main ():

    parser = argparse.ArgumentParser(description="Compute and plot averages of random methods")

    parser.add_argument("results_directory", help="The directory containing the folders with the results to be processed")
    parser.add_argument('-d', "--debug", help="Enable debug messages", 
                        default=False, action="store_true")
    parser.add_argument('-p', "--plot", 
                        help="Path to the directory where plots should be saved", 
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
    
    # all distributions
    all_distributions = {"high": 0, "mixed": 1, "low": 2, "exponential": 3}
    # all methods
    all_methods = {"Greedy": 0, "RandomGreedy": 1, "EDF": 2, "LocalSearch": 3,
                   "PathRelinking": 4}
    
    # folder for figures
    if args.plot != "":
        fig_folder1 = args.plot + "/times"
        fig_folder2 = args.plot + "/costs"
        createFolder(fig_folder1)
        createFolder(fig_folder2)
    else:
        fig_folder1 = ""
        fig_folder2 = ""
    
    # get times
    times_folder = os.path.join(args.results_directory, "times")
    if os.path.exists(times_folder):
        all_times = pd.DataFrame(columns = ["Method", "nN", "nJ", "Seed", 
                                            "delta", "Distribution", 
                                            "SubmissionTime", "FinishTime"])
        for file in os.listdir(times_folder):
            tokens = file.split("_")
            method = tokens[0]
            nodes = tokens[1]
            jobs = tokens[2]
            lambdaa = tokens[3]
            delta = tokens[4]
            distribution = tokens[5]
            seed = tokens[6].replace(".csv","")        
            experiment = (method, nodes, jobs, lambdaa, delta, seed)
            logging.debug("Examining %s -- %s", str(experiment), distribution)
            times = pd.read_csv(os.path.join(times_folder, file))
            temp = pd.DataFrame()
            temp["Method"] = [method] * len(times)
            temp["nN"] = [nodes] * len(times)
            temp["nJ"] = [jobs] * len(times)
            temp["Seed"] = [seed] * len(times)
            temp["delta"] = [delta] * len(times)
            temp["Distribution"] = [distribution + "_" + lambdaa] * len(times)
            temp["SubmissionTime"] = times["SubmissionTime"]
            temp["FinishTime"] = times["FinishTime"]
            all_times = all_times.append(temp, ignore_index=True)

            # # plot times (one plot for each seed)
            # all_times_by_experiment = all_times.groupby(["Method", "nN", 
            #                                              "nJ", "Seed", 
            #                                              "delta"])
            # for experiment in all_times_by_experiment:
            #     times_all_distr = experiment[1]
            #     title = "_".join(experiment[0])
            #     plot_times(all_distributions, times_all_distr, title)
            
            # plot times (average w.r.t. seed)
            all_times_by_experiment = all_times.groupby(["Method", "nN", "nJ", 
                                                         "delta"])
            for experiment in all_times_by_experiment:
                times_all_seeds = experiment[1]
                times_all_distr = pd.DataFrame()
                for distribution in times_all_seeds.groupby(["Distribution"]):
                    ST_avg = np.array([])
                    FT_avg = np.array([])
                    group_by_seed = distribution[1].groupby(["Seed"])
                    for seed in group_by_seed:
                        times = seed[1].copy()
                        times.index = range(len(times))
                        if len(ST_avg) == 0:
                            ST_avg = np.array([0.0] * len(times))
                            FT_avg = np.array([0.0] * len(times))
                        ST_avg += times["SubmissionTime"]
                        FT_avg += times["FinishTime"]
                    ST_avg /= len(group_by_seed)
                    FT_avg /= len(group_by_seed)
                    temp = pd.DataFrame()
                    temp["Distribution"] = [distribution[0]] * len(ST_avg)
                    temp["SubmissionTime"] = ST_avg
                    temp["FinishTime"] = FT_avg
                    times_all_distr = times_all_distr.append(temp, 
                                                             ignore_index=True)
                title = "_".join(experiment[0])
                plot_times(all_distributions, times_all_distr, fig_folder1, title)
    else:
        logging.info("No times folder found")
    
    # get costs and tardiness
    all_costs = get_all_costs(args.results_directory)
    
    if all_costs.size > 0:
        # # plot costs (one plot for each seed)
        # costs_by_distribution = all_costs.groupby(["Distribution"])
        # for distribution in costs_by_distribution:
        #     costs_all_methods = distribution[1]
        #     title = distribution[0]
        #     plot_costs(all_methods, costs_all_methods, "TotalCost", title)
        #     plot_costs(all_methods, costs_all_methods, "TotalTardi", title)    
        
        # plot costs and tardi (average w.r.t. seed)
        for delta in all_costs.groupby(["Delta"]):
            costs_by_experiment = delta[1].groupby(["Nodes", "Jobs", "Distribution"])
            avg_costs = pd.DataFrame()
            for experiment in costs_by_experiment:
                costs_all_seeds = experiment[1]
                ext_temp = pd.DataFrame()
                for method in costs_all_seeds.groupby(["Method"]):
                    C_avg = np.array([])
                    T_avg = np.array([])
                    group_by_seed = method[1].groupby(["Seed"])
                    for seed in group_by_seed:
                        costs = seed[1].copy()
                        costs.index = range(len(costs))
                        if len(C_avg) == 0:
                            C_avg = np.array([0.0] * len(costs))
                            T_avg = np.array([0.0] * len(costs))
                        C_avg += costs["TotalCost"]
                        T_avg += (costs["TotalTardi"] * 0.009)
                    C_avg /= len(group_by_seed)
                    T_avg /= len(group_by_seed)
                    temp = pd.DataFrame()
                    temp["Method"] = [method[0]] * len(C_avg)
                    temp["Nodes"] = [experiment[0][0]] * len(C_avg)
                    temp["Distribution"] = [experiment[0][-1]] * len(C_avg)
                    temp["TotalCost"] = C_avg
                    temp["TotalTardi"] = T_avg
                    ext_temp = ext_temp.append(temp, ignore_index=True)
                avg_costs = avg_costs.append(ext_temp, ignore_index=True)
            plot_avg_costs(all_methods, avg_costs, "_"+str(delta[0]), fig_folder2)
    else:
        logging.info("No results folder found")

if __name__ == "__main__":
    main()
